#include "event.h"

#include <ostream>
#include <string>
#include <utility>

#include "copy_stats.h"

namespace nano_edr {

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    if (raw_ts.empty()) {
        throw std::invalid_argument("ts пусто");
    }
    if (type.empty()) {
        throw std::invalid_argument("type пусто");
    }
    uint64_t ms = 0;
    auto [ptr, ec] = std::from_chars(raw_ts.data(), raw_ts.data() + raw_ts.size(), ms);
    if (ec != std::errc() || ptr != raw_ts.data() + raw_ts.size()) {
        throw std::invalid_argument("ts неправильный формат: " + raw_ts);
    }
    return Timestamp{ms};
}

Event::Event(const EventParts& parts)
    : ts_(Validate(parts.ts, parts.type)),
      raw_ts_(parts.ts),
      type_(parts.type),
      event_type_(EventTypeFromString(parts.type)),
      pid_(parts.pid),
      fields_(parts.fields) {
    copy_stats().event_strings_copied += 3 + 2 * parts.fields.size();
}

Event::Event(EventParts&& parts) : ts_(Validate(parts.ts, parts.type)) {
    event_type_ = EventTypeFromString(parts.type);
    copy_stats().event_strings_moved += 3 + 2 * parts.fields.size();
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    pid_ = std::move(parts.pid);
    fields_ = std::move(parts.fields);
}

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") return EventType::kProcessStart;
    if (type == "process_end") return EventType::kProcessEnd;
    if (type == "file_create") return EventType::kFileCreate;
    if (type == "file_write") return EventType::kFileWrite;
    if (type == "file_delete") return EventType::kFileDelete;
    if (type == "file_move") return EventType::kFileMove;
    if (type == "net_connect") return EventType::kNetConnect;
    return EventType::kOther;
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