#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <unordered_map>

#include "agent_rules.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"

namespace nano_edr {

struct Config {
    std::string log_path;
    bool quiet = false;
    std::size_t window_size = 64;
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
        } else if (config.log_path.empty()) {
            config.log_path = arg;
        }
    }

    if (config.log_path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return false;
    }
    return true;
}

void PrintContext(const EventList& window) {
    const EventNode* prev = nullptr;
    const EventNode* curr = nullptr;
    for (const EventNode* node = window.head; node != nullptr; node = node->next) {
        prev = curr;
        curr = node;
    }
    if (prev != nullptr) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev->event.ts, prev->event.type, prev->event.pid);
    }
    if (curr != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", curr->event.ts, curr->event.type, curr->event.pid);
    }
}

void PrintSummary(long long lines, long long comments, const std::unordered_map<std::string, int>& types) {
    std::print("строк {}, из них комментариев {}\n", lines, comments);
    std::print("всего событий: {}\n", lines - comments);
    std::print("типы событий:\n");
    for (const auto& [type, count] : types) {
        std::print("  {}: {}\n", type, count);
    }
}

void ProcessLog(std::ifstream& log, const Config& config) {
    EventList window;
    window.capacity = config.window_size;

    long long lines = 0;
    long long comments = 0;
    std::string line;
    std::unordered_map<std::string, int> types;

    const Rule* rules = AgentRules();
    const size_t rule_count = AgentRuleCount();

    while (std::getline(log, line)) {
        ++lines;

        if (IsBlankOrComment(&line)) {
            ++comments;
            continue;
        }

        Event event;
        if (!ParseEventLine(&line, &event)) {
            continue;
        }
        types[event.type]++;

        size_t detects = CheckRules(event, rules, rule_count);
        if (detects > 0 && !config.quiet) {
            PrintContext(window);
        }

        ListPushBack(&window, &event);
    }

    if (!config.quiet) {
        PrintSummary(lines, comments, types);
    }
}

}  // namespace

}  // namespace nano_edr

int main(int argc, char** argv) {
    nano_edr::Config config;
    if (!nano_edr::ParseArgs(argc, argv, config)) {
        return 2;
    }

    std::ifstream log(config.log_path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", config.log_path);
        return 2;
    }

    try {
        nano_edr::ProcessLog(log, config);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }

    return 0;
}
