#include "field_traits.h"

#include <charconv>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}

bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    uint64_t value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [position, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || position != end) {
        return false;
    }
    *out = value;
    return true;
}

bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    if (text == "1" || TextEquals(text, "true") || TextEquals(text, "yes")) {
        *out = true;
        return true;
    }
    if (text == "0" || TextEquals(text, "false") || TextEquals(text, "no")) {
        *out = false;
        return true;
    }
    return false;
}

}  // namespace nano_edr
