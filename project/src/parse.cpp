#include <parse.h>
#include <string>
#include "event.h"
#include <unordered_set>

namespace nano_edr {

bool ParseEventLine(const std::string* line, Event* out){
    if (IsBlankOrComment(line))
        return false;

    std::unordered_set<std::string> signs = {"ts", "type", "pid"};
    std::string ts = "", type = "", pid = "";
    std::vector<Field> fields;
    std::string cur, key;
    bool in_quotes = false;
    bool quote_closed = false;
    for (auto& i : *line + ' '){
        if (quote_closed && i != ' '){
            return false;
        }
        quote_closed = false;

        if (i == '"'){
            in_quotes = !in_quotes;
            if (!in_quotes)
                quote_closed = true;
        } else if (i == ' ' && !in_quotes){
            if (!key.empty()){
                if (ts.empty() && key == "ts"){
                    ts = cur;
                } else if (type.empty() && key == "type"){
                    type = cur;
                } else if (pid.empty() && key == "pid"){
                    pid = cur;
                } else {
                    fields.push_back({key, cur});
                }    
                key = "";
                cur = "";
            } else if (!cur.empty()){
                return false;
            }
        } else if (i == '=' && !in_quotes && key.empty()){
            if (cur.empty())
                return false;
            key = cur;
            cur = "";
        } else {
            cur += i;
        }
    }
    
    if (in_quotes || ts.empty() || type.empty()){
        return false;
    }

    out->ts = ts;
    out->type = type;
    out->pid = pid;
    out->fields = fields;

    return true;
}


bool IsBlankOrComment(const std::string* line){
    size_t fisrt = line->find_first_not_of(" \t");
    if (fisrt == std::string::npos)
        return true;
    return line->at(fisrt) == '#' || line->at(fisrt) == ';';
}

}  // namespace nano_edr
