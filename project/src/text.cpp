#include "text.h"

#include <cctype>

namespace nano_edr {

namespace {

char LowerAscii(char character) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
}

}  // namespace

std::string LowerAscii(const std::string& text) {
    std::string result = text;
    for (char& character : result) {
        character = LowerAscii(character);
    }
    return result;
}

bool TextEquals(const std::string& text, const std::string& expected) {
    if (text.size() != expected.size()) {
        return false;
    }
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (LowerAscii(text[index]) != LowerAscii(expected[index])) {
            return false;
        }
    }
    return true;
}

bool TextContains(const std::string& text, const std::string& needle) {
    return LowerAscii(text).contains(LowerAscii(needle));
}

bool TextEndsWith(const std::string& text, const std::string& suffix) {
    return LowerAscii(text).ends_with(LowerAscii(suffix));
}

}  // namespace nano_edr
