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
    if (needle.size() > text.size()) {
        return false;
    }
    for (std::size_t start = 0; start <= text.size() - needle.size(); ++start) {
        std::size_t index = 0;
        while (index < needle.size() &&
               LowerAscii(text[start + index]) == LowerAscii(needle[index])) {
            ++index;
        }
        if (index == needle.size()) {
            return true;
        }
    }
    return false;
}

bool TextEndsWith(const std::string& text, const std::string& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    const std::size_t start = text.size() - suffix.size();
    for (std::size_t index = 0; index < suffix.size(); ++index) {
        if (LowerAscii(text[start + index]) != LowerAscii(suffix[index])) {
            return false;
        }
    }
    return true;
}

}  // namespace nano_edr
