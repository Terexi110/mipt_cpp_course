#include "fields.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace nano_edr {

// ---------------------------------------------------------------------------
// Способы сообщения об ошибках в функциях доступа к полям:
//
// 1. FindField: возвращает nullptr, если поля нет в событии.
//    Отсутствие поля — нормальный и ожидаемый исход (например, у file_write нет
//    поля domain), а не поломка программы.
//
// 2. GetRequiredField: бросает std::invalid_argument, если поля нет.
//    Отсутствие обязательного поля нарушает контракт формата журнала (например,
//    process_start без image), продолжать обработку события бессмысленно.
//
// 3. GetIntField (с указателем out): возвращает bool (false при ошибке).
//    Битые данные или отсутствие поля во входном журнале — штатная ситуация
//    внешних данных, а не исключительная ситуация в коде.
//
// 4. GetIntField (со значением fallback): возвращает fallback по умолчанию.
//    Удобно для необязательных числовых полей (например, ppid).
//
// 5. Предикаты (Is*, PathEndsWith, CommandLineContains): возвращают bool.
//    Вопрос о событии предполагает ответ да/нет; при отсутствии нужных полей
//    возвращается false без падений и исключений.
// ---------------------------------------------------------------------------

const std::string* FindField(const Event& event, const std::string& key) {
    for (const auto& field : event.fields) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* value = FindField(event, key);
    if (!value) {
        throw std::invalid_argument("обязательное поле отсутствует: " + key);
    }
    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    if (!out) {
        return false;
    }

    const std::string* value = FindField(event, key);
    if (!value || value->empty()) {
        return false;
    }

    uint64_t tmp;
    const auto& s = *value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), tmp);

    if (ec == std::errc{} && ptr == s.data() + s.size()) {
        *out = tmp;
        return true;
    }
    return false;
}

uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t value = 0;
    if (GetIntField(event, key, &value)) {
        return value;
    }
    return fallback;
}

bool IsProcessStart(const Event& event) {
    return event.type == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}

std::string NormalizePath(const std::string& path) {
    std::string res = path;
    std::ranges::transform(res, res.begin(), [](unsigned char c) -> char {
        return static_cast<char>(std::tolower(c));
    });

    std::ranges::replace(res, '/', '\\');
    if (auto pos = res.find("%temp%"); pos != std::string::npos) {
        res.replace(pos, 6, "\\appdata\\local\\temp");
    }
    if (auto pos = res.find("%tmp%"); pos != std::string::npos) {
        res.replace(pos, 5, "\\appdata\\local\\temp");
    }

    auto [ret_begin, ret_end] = std::ranges::unique(res, [](char a, char b) {
        return a == '\\' && b == '\\';
    });
    res.erase(ret_begin, ret_end);

    return res;
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (!path) {
        return false;
    }

    std::string norm_path = NormalizePath(*path);
    std::string norm_suffix = NormalizePath(suffix);
    return norm_path.ends_with(norm_suffix);
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmdline = FindField(event, "cmdline");
    if (!cmdline) {
        return false;
    }

    std::string norm_cmdline = NormalizePath(*cmdline);
    std::string norm_needle = NormalizePath(needle);
    return norm_cmdline.find(norm_needle) != std::string::npos;
}

}  // namespace nano_edr