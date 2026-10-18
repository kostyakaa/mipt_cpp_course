#include "parse.h"

namespace {

bool IsSpace(char character) {
    return character == ' ' || character == '\t';
}

void SkipSpaces(const std::string& line, std::size_t* position) {
    while (*position < line.size() && IsSpace(line[*position])) {
        ++(*position);
    }
}

bool ParseKey(const std::string& line, std::size_t* position,
              std::string* key) {
    while (*position < line.size() && line[*position] != '=') {
        const char character = line[*position];
        if (IsSpace(character) || character == '"') {
            return false;
        }
        *key += character;
        ++(*position);
    }
    return !key->empty() && *position < line.size();
}

bool ParseValue(const std::string& line, std::size_t* position,
                std::string* value) {
    if (*position == line.size()) {
        return true;
    }

    if (line[*position] == '"') {
        ++(*position);
        while (*position < line.size() && line[*position] != '"') {
            *value += line[*position];
            ++(*position);
        }
        if (*position == line.size()) {
            return false;
        }
        ++(*position);
        return *position == line.size() || IsSpace(line[*position]);
    }

    while (*position < line.size() && !IsSpace(line[*position])) {
        *value += line[*position];
        ++(*position);
    }
    return true;
}

}  // namespace

namespace nano_edr {

bool ParseEventParts(const std::string& line, EventParts* out) {
    if (out == nullptr || IsBlankOrComment(line)) {
        return false;
    }

    EventParts parsed;
    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;
    std::size_t position = 0;

    while (position < line.size()) {
        SkipSpaces(line, &position);
        if (position == line.size()) {
            break;
        }

        std::string key;
        std::string value;
        if (!ParseKey(line, &position, &key)) {
            return false;
        }
        ++position;
        if (!ParseValue(line, &position, &value)) {
            return false;
        }

        if (key == "ts" && !has_ts) {
            parsed.ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            parsed.type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            parsed.pid = value;
            has_pid = true;
        } else {
            parsed.fields.push_back({key, value});
        }
    }

    *out = parsed;
    return true;
}

bool IsBlankOrComment(const std::string& line) {
    std::size_t first = 0;
    SkipSpaces(line, &first);
    return first == line.size() || line[first] == '#' || line[first] == ';';
}

}  // namespace nano_edr
