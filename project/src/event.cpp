#include "event.h"

#include <ostream>
#include <string>

namespace nano_edr {

Event::Event(const EventParts& parts) : raw_ts_(parts.ts), type_(parts.type), pid_(parts.pid), fields_(parts.fields) {
    if (raw_ts_.empty()) {
        throw std::invalid_argument("ts пусто");
    }
    if (type_.empty()) {
        throw std::invalid_argument("type пусто");
    }
    try {
        uint64_t ms = 0;
        auto [ptr, ec] = std::from_chars(raw_ts_.data(), raw_ts_.data() + raw_ts_.size(), ms);
        if (ec != std::errc() || ptr != raw_ts_.data() + raw_ts_.size()) {
            throw std::invalid_argument("ts неправильный формат: " + raw_ts_);
        }
        ts_ = Timestamp{ms};
    } catch (const std::exception&) {
        throw std::invalid_argument("ошибка разбора ts: " + raw_ts_);
    }
}

std::string ToString(const Event& event) {
    std::string result = "ts=" + event.raw_ts() + " type=" + event.type() + " pid=" + event.pid();
    for (const auto& field : event.fields()) {
        result += " " + field.key + "=" + field.value;
    }
    return result;
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
    return os << ToString(event);
}

}  // namespace nano_edr