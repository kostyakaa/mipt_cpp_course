#include "event.h"

#include <charconv>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "copy_stats.h"

namespace nano_edr {

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") {
        return EventType::kProcessStart;
    }
    if (type == "process_end") {
        return EventType::kProcessEnd;
    }
    if (type == "file_create") {
        return EventType::kFileCreate;
    }
    if (type == "file_write") {
        return EventType::kFileWrite;
    }
    if (type == "file_delete") {
        return EventType::kFileDelete;
    }
    if (type == "file_move") {
        return EventType::kFileMove;
    }
    if (type == "net_connect") {
        return EventType::kNetConnect;
    }
    return EventType::kOther;
}

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    if (raw_ts.empty()) {
        throw std::invalid_argument("пустое значение ts");
    }
    if (type.empty()) {
        throw std::invalid_argument("пустое значение type");
    }

    Timestamp ts;
    const char* begin = raw_ts.data();
    const char* end = begin + raw_ts.size();
    const auto [position, error] = std::from_chars(begin, end, ts.ms);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument("некорректное значение ts: " + raw_ts);
    }
    return ts;
}

Event::Event(const EventParts& parts) {
    ts_ = Validate(parts.ts, parts.type);
    event_type_ = EventTypeFromString(parts.type);
    raw_ts_ = parts.ts;
    type_ = parts.type;
    pid_ = parts.pid;
    fields_ = parts.fields;
    copy_stats().event_strings_copied += 3 + 2 * parts.fields.size();
}

Event::Event(EventParts&& parts) {
    ts_ = Validate(parts.ts, parts.type);
    event_type_ = EventTypeFromString(parts.type);
    copy_stats().event_strings_moved += 3 + 2 * parts.fields.size();
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    pid_ = std::move(parts.pid);
    fields_ = std::move(parts.fields);
}

std::string ToString(const Event& event) {
    std::ostringstream out;
    out << "ts=" << event.raw_ts() << " type=" << event.type();
    if (!event.pid().empty()) {
        out << " pid=" << event.pid();
    }
    for (const Field& field : event.fields()) {
        out << ' ' << field.key << '=' << field.value;
    }
    return out.str();
}

std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr
