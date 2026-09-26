#include <parse.h>
#include <string>
#include "event.h"

namespace nano_edr {

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line)) {
        return false;
    }

    std::string ts;
    std::string type;
    std::string pid;
    std::vector<Field> fields;
    std::string cur;
    std::string key;
    bool has_key = false;
    bool in_quotes = false;
    bool quote_closed = false;

    auto save_pair = [&](const std::string& k, const std::string& v) {
        if (ts.empty() && k == "ts") {
            ts = v;
        } else if (type.empty() && k == "type") {
            type = v;
        } else if (pid.empty() && k == "pid") {
            pid = v;
        } else {
            fields.push_back({k, v});
        }
    };

    for (char c : *line) {
        if (quote_closed) {
            if (c != ' ') {
                return false;
            }
            quote_closed = false;
        }

        if (in_quotes) {
            if (c == '"') {
                in_quotes = false;
                quote_closed = true;
            } else {
                cur += c;
            }
        } else if (c == ' ') {
            if (has_key) {
                save_pair(key, cur);
                key.clear();
                cur.clear();
                has_key = false;
            } else if (!cur.empty()) {
                return false;
            }
        } else if (c == '=') {
            if (!has_key) {
                if (cur.empty()) {
                    return false;
                }
                key = cur;
                cur.clear();
                has_key = true;
            } else {
                cur += c;
            }
        } else if (c == '"') {
            if (has_key && cur.empty()) {
                in_quotes = true;
            } else {
                return false;
            }
        } else {
            cur += c;
        }
    }

    if (in_quotes) {
        return false;
    }

    if (has_key) {
        save_pair(key, cur);
    } else if (!cur.empty()) {
        return false;
    }

    if (ts.empty() || type.empty()) {
        return false;
    }

    out->ts = std::move(ts);
    out->type = std::move(type);
    out->pid = std::move(pid);
    out->fields = std::move(fields);

    return true;
}

bool IsBlankOrComment(const std::string* line) {
    size_t first = line->find_first_not_of(" \t");
    if (first == std::string::npos) {
        return true;
    }
    return (*line)[first] == '#' || (*line)[first] == ';';
}

}  // namespace nano_edr