#ifndef NANO_EDR_TEXT_H
#define NANO_EDR_TEXT_H

#include <string>

namespace nano_edr {

bool TextEquals(const std::string& text, const std::string& expected);
bool TextContains(const std::string& text, const std::string& needle);
bool TextEndsWith(const std::string& text, const std::string& suffix);
std::string LowerAscii(const std::string& text);

}  // namespace nano_edr

#endif  // NANO_EDR_TEXT_H
