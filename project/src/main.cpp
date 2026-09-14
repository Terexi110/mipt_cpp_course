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
// 2.0
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>


int main(int argc, char** argv) {
    // Аргументы разбираются грубо: путь к журналу и ничего больше. Остальное,
    // включая --quiet, добавляется по заданию.
    bool quiet = false;
    std::string log_path;
    if (argc < 2) {
        std::print(stderr, "использование: nano-edr <журнал.log>\n");
        return 2;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--quiet") {
            quiet = true;
        } else if (log_path.empty()) {
            log_path = argv[i];
        }
    }

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
        // Счётчик увеличивается до всех проверок: он считает строки файла,
        // а не события. Номер, посчитанный по событиям, бесполезен — по нему
        // нельзя открыть файл и посмотреть.
        ++lines;

        // Строки-комментарии в журнале начинаются с '#'. Они не события,
        // и детекта по ним быть не должно.
        if (!line.empty() && line[0] == '#') {
            ++comments;
            continue;
        }

        // >>> Здесь начинается занятие 1.1.
        //
        // Проверка признаков и печать детекта. Номер строки, который нужен
        // в выводе, — это lines.

        for (auto &cur_sign : signs){
            if (line.find(cur_sign) != std::string::npos){
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, cur_sign, line);
            }
        }
        std::string type = "-";
        auto pos = line.find("type=");
        size_t start = pos + 5;
        size_t end = line.find(" ", start);
        type = line.substr(start, end - start);
        types[type]++;
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
