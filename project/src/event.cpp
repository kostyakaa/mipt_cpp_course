#include "event.h"

#include <charconv>
#include <sstream>
#include <stdexcept>

namespace nano_edr {

Event::Event(const EventParts& parts)
    : raw_ts_(parts.ts),
      type_(parts.type),
      pid_(parts.pid),
      fields_(parts.fields) {
    if (raw_ts_.empty()) {
        throw std::invalid_argument("пустое значение ts");
    }
    if (type_.empty()) {
        throw std::invalid_argument("пустое значение type");
    }

    const char* begin = raw_ts_.data();
    const char* end = begin + raw_ts_.size();
    const auto [position, error] = std::from_chars(begin, end, ts_.ms);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument("некорректное значение ts: " + raw_ts_);
    }
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
