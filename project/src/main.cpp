// Каркас агента: читает журнал событий построчно и считает строки.
//
// Это заготовка занятия 1.1, а не решение. Детектов она не ищет — их вы
// добавите здесь же, в отмеченном месте ниже. Формат строки детекта, список
// признаков и правило про их порядок заданы в постановке занятия: по ним
// сравниваются эталоны.
//
// Весь код лежит в main, и на этом занятии так и надо: функции появятся
// на занятии 1.2, ссылки — на 1.3. Разбор аргументов, коды возврата и флаг
// --quiet — часть задания.
//
// Запуск:
//   nano-edr <журнал.log>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <unordered_map>
#include "parse.h"
#include "event_list.h"

using namespace nano_edr;

int main(int argc, char** argv) {
    bool quiet = false;
    std::size_t window_size = 64;
    std::string log_path;
    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return 2;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--quiet") {
            quiet = true;
        } else if (std::string(argv[i]) == "--window-size") {
            window_size = std::stoull(argv[i + 1]);
            ++i;
        } else if (log_path.empty()) {
            log_path = argv[i];
        }
    }

    EventList window;
    window.capacity = window_size;
    long long lines = 0;
    long long comments = 0;
    std::string line;
    std::vector<std::string> signs = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    std::unordered_map<std::string, int> types;
    std::ifstream log(log_path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", log_path);
        return 2;
    }

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

        bool detected = false;
        for (auto &cur_sign : signs) {
            if (line.find(cur_sign) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, cur_sign, line);
                detected = true;
            }
        }
        if (detected && !quiet) {
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

        ListPushBack(&window, &event);
    }

    if (!quiet) {
        std::print("строк {}, из них комментариев {}\n", lines, comments);
        std::print("всего сообытий: {}", lines - comments);
        std::print("типы событий:\n");
        for (const auto &pair : types) {
            std::print("  {}: {}\n", pair.first, pair.second);
        }
    }
    return 0;
}
