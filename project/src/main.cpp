#include <cstdio>
    #include <cstring>
    #include <memory>   
    #include <print>
    #include <string>
    
    #include "agent.h"
    #include "event_source.h" 
    #include "file_source.h" 
    #include "os_source.h" 
    
    namespace nano_edr {
    
    struct Config {
        std::string log_path;
        bool quiet = false;
        bool os_source = true;
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
                const char* value = argv[++i];
                auto [ptr, ec] = std::from_chars(value, value + std::strlen(value), config.window_size);
                if (ec != std::errc()) {
                    std::print(stderr, "ошибка: неверное значение для --window-size: {}\n", argv[i]);
                    return false;
                }
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

            std::unique_ptr<nano_edr::IEventSource> source;
            if (config.os_source) {
                source = std::make_unique<nano_edr::OsSource>(config.log_path);
            } else {
                source = std::make_unique<nano_edr::FileSource>(config.log_path);
            }

            source->Run(&agent);

            if (!config.quiet) {
                agent.PrintSummary();
            }
        } catch (const std::exception& e) {
            std::print(stderr, "ошибка: {}\n", e.what());
            return 1;
        }

        return 0;
    }