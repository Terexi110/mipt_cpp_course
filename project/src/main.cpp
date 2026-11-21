#include <charconv>
#include <cstdio>
#include <memory>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <utility>

#include "agent.h"
#include "alert_sink.h"
#include "edr_handle.h"
#include "event_source.h"
#include "field_traits.h"
#include "file_source.h"
#include "limit_handler.h"
#include "os_source.h"

namespace nano_edr {

struct Config {
    std::string log_path;
    std::string out;
    bool quiet = false;
    bool os_source = true;
    std::optional<std::size_t> max_events;
};

namespace {

constexpr std::string_view kUsage =
    "использование: nano-edr [--quiet] [--file] [--out <файл>] <журнал.log>\n";

bool SetLogPath(Config& config, std::string path) {
    if (!config.log_path.empty()) {
        std::print(stderr, "ошибка: путь к файлу уже указан: {}\n", config.log_path);
        return false;
    }
    config.log_path = std::move(path);
    return true;
}

bool ParseCount(std::string_view text, std::size_t* count) {
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, *count);
    return ec == std::errc() && ptr == end;
}

bool ParseArgs(int argc, char** argv, Config& config) {
    if (argc < 2) {
        std::print(stderr, "{}", kUsage);
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        auto next_value = [&](std::string_view what) -> const char* {
            if (i + 1 >= argc) {
                std::print(stderr, "ошибка: отсутствует {} для {}\n", what, arg);
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--quiet") {
            config.quiet = true;
        } else if (arg == "--file") {
            const char* value = next_value("путь к файлу");
            if (value == nullptr || !SetLogPath(config, value)) {
                return false;
            }
            config.os_source = false;
        } else if (arg == "--out") {
            const char* value = next_value("путь к файлу");
            if (value == nullptr) {
                return false;
            }
            if (!config.out.empty()) {
                std::print(stderr, "ошибка: --out указан повторно\n");
                return false;
            }
            config.out = value;
        } else if (arg == "--max-events") {
            const char* value = next_value("значение");
            if (value == nullptr) {
                return false;
            }
            std::size_t count = 0;
            if (!ParseCount(value, &count)) {
                std::print(stderr, "ошибка: неверное значение для --max-events: {}\n", value);
                return false;
            }
            config.max_events = count;
        } else if (!arg.starts_with('-')) {
            if (!SetLogPath(config, std::string(arg))) {
                return false;
            }
        } else {
            std::print(stderr, "ошибка: неизвестный аргумент {}\n", arg);
            return false;
        }
    }

    if (config.log_path.empty()) {
        std::print(stderr, "{}", kUsage);
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
        nano_edr::EdrHandle edr(config.log_path);

        const std::string dedup_text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(dedup_text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", dedup_text);
            dedup_period_ms = 10000;
        }

        nano_edr::Agent agent(config.quiet, dedup_period_ms);

        nano_edr::FanOutSink targets;
        targets.Add(nano_edr::ConsoleSink());
        if (!config.out.empty()) {
            targets.Add(nano_edr::FileSink(config.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));

        std::unique_ptr<nano_edr::IEventSource> source;
        std::unique_ptr<nano_edr::LimitHandler> limit_handler;
        nano_edr::IEventHandler* handler = &agent;

        if (config.max_events.has_value()) {
            limit_handler = std::make_unique<nano_edr::LimitHandler>(handler, *config.max_events);
            handler = limit_handler.get();
        }

        if (config.os_source) {
            source = std::make_unique<nano_edr::OsSource>(config.log_path);
        } else {
            source = std::make_unique<nano_edr::FileSource>(config.log_path);
        }

        source->Run(handler);

        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }

        if (!config.quiet) {
            agent.PrintSummary();
            if (limit_handler) {
                std::print("отброшено событий: {}\n", limit_handler->dropped());
            }
        }
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка: {}\n", e.what());
        return 1;
    }

    return 0;
}
