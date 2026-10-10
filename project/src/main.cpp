#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <unordered_map>

#include "agent_rules.h"
#include "event.h"
#include "event_list.h"
#include "os.h"
#include "os_handle.h"
#include "parse.h"
#include "rules.h"

namespace nano_edr {

struct Config {
    std::string log_path;
    bool quiet = false;
    bool os_source = true;
    std::size_t window_size = 64;
};

void PrintContext(const EventList& window) {
    const EventNode* prev = nullptr;
    const EventNode* curr = nullptr;
    for (const EventNode* node = window.head(); node != nullptr; node = node->next) {
        prev = curr;
        curr = node;
    }
    if (prev != nullptr) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev->event.raw_ts(), prev->event.type(), prev->event.pid());
    }
    if (curr != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", curr->event.raw_ts(), curr->event.type(), curr->event.pid());
    }
}

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet) {
    }

    void HandleEvent(const Event& event) {
        types_[event.type()]++;
        total_events_++;

        size_t detect_count = CheckRules(event, AgentRules(), AgentRuleCount());
        if (detect_count > 0 && !quiet_) {
            PrintContext(window_);
        }
        window_.PushBack(event);
    }

    static void Trampoline(const os_event* ev, void* ctx) noexcept {
        try {
            auto* self = static_cast<Agent*>(ctx);
            EventParts parst;
            parst.ts = std::to_string(ev->ts);
            parst.type = ev->type ? ev->type : "";
            parst.pid = (ev->pid == 0) ? "" : std::to_string(ev->pid);
            for (size_t i = 0; i < ev->field_count; ++i) {
                parst.fields.push_back(Field{ev->fields[i].key, ev->fields[i].value});
            }
            Event event(parst);
            self->HandleEvent(event);
        } catch (...) {
            // ...
        }
    }

    void PrintSummary() const {
        std::print("всего событий: {}\n", total_events_);
        std::print("типы событий:\n");
        for (const auto& [type, count] : types_) {
            std::print("  {}: {}\n", type, count);
        }
    }

 private:
    EventList window_;
    bool quiet_;
    long long total_events_ = 0;
    std::unordered_map<std::string, int> types_;
};

class OsSource {
 public:
    explicit OsSource(const std::string& config_path) : handle_(config_path) {}

    void Run(Agent* agent) {
        handle_.Subscribe(Agent::Trampoline, agent);
        handle_.Start();

        os_status status = OS_OK;
        while ((status = handle_.Wait(1000)) == OS_TIMEOUT) {
        }
        if (status != OS_OK) {
            throw std::runtime_error("ошибка источника: " + std::string(os_status_str(status)));
        }
    }

 private:
    OsHandle handle_;
};

class FileSource {
 public:
    explicit FileSource(const std::string& path) : path_(path) {}

    void Run(Agent* agent) {
        std::ifstream log(path_);
        if (!log) {
            throw std::runtime_error("не удалось открыть журнал: " + path_);
        }

        std::string line;
        while (std::getline(log, line)) {
            if (IsBlankOrComment(line)) {
                continue;
            }
            EventParts parts;
            if (!ParseEventParts(line, &parts)) {
                continue;
            }
            try {
                Event event(parts);
                agent->HandleEvent(event);
            } catch (const std::exception& e) {
            }
        }

    }  // строка → EventParts → Event → HandleEvent
 private:
    std::string path_;
};

namespace {

bool ParseArgs(int argc, char** argv, Config& config) {
    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quiet") {
            config.quiet = true;
        } else if (arg == "--window-size") {
            if (i + 1 >= argc) {
                std::print(stderr, "ошибка: отсутствует значение для --window-size\n");
                return false;
            }
            config.window_size = std::stoull(argv[++i]);
        } else if (arg == "--file") {
            if (i + 1 >= argc) {
                std::print(stderr, "ошибка: отсутствует путь к файлу для --file\n");
                return false;
            }
            if (!config.log_path.empty()) {
                std::print(stderr, "ошибка: путь к файлу уже указан: {}\n", config.log_path);
                return false;
            }
            config.log_path = argv[++i];
            config.os_source = false;
        } else if (!arg.starts_with('-')) {
            if (!config.log_path.empty()) {
                std::print(stderr, "ошибка: путь к файлу уже указан: {}\n", config.log_path);
                return false;
            }
            config.log_path = arg;
        } else {
            std::print(stderr, "ошибка: неизвестный аргумент {}\n", arg);
            return false;
        }
    }

    if (config.log_path.empty()) {
        std::print(stderr, "использование: nano-edr [--quiet] [--window-size N] [--file] <журнал.log>\n");
        return false;
    }
    return true;
}

}  // namespace

}  // namespace nano_edr

int main(int argc, char** argv) {
    nano_edr::Config config;
    if (!nano_edr::ParseArgs(argc, argv, config)) {
        return 2;
    }

    try {
        nano_edr::Agent agent(config.window_size, config.quiet);
        if (config.os_source) {
            nano_edr::OsSource source(config.log_path);
            source.Run(&agent);
        } else {
            nano_edr::FileSource source(config.log_path);
            source.Run(&agent);
        }
        if (!config.quiet) {
            agent.PrintSummary();
        }
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }

    return 0;
}
