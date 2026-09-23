#include "parse.h"

namespace {

bool is_space(char character) {
    return character == ' ' || character == '\t';
}

void skip_spaces(const std::string* line, std::size_t* pos) {
    while (*pos < line->size() && is_space((*line)[*pos])) {
        ++(*pos);
    }
}

bool parse_key(const std::string* line, std::size_t* pos, std::string* key) {
    while (*pos < line->size()) {
        char cur_char = (*line)[*pos];
        if (cur_char == '=') {
            break;
        }
        if (is_space(cur_char) || cur_char == '"') {
            return false;
        }

        *key += cur_char;
        ++(*pos);
    }

    return !key->empty() && *pos < line->size() && (*line)[*pos] == '=';
}

bool parse_value(const std::string* line, std::size_t* pos, std::string* value) {
    if (*pos == line->size()) {
        return true;
    }

    if ((*line)[*pos] == '"') {
        ++(*pos);
        while (*pos < line->size() && (*line)[*pos] != '"') {
            *value += (*line)[*pos];
            ++(*pos);
        }

        if (*pos == line->size()) {
            return false;
        }
        ++(*pos);
        if (*pos < line->size() && !is_space((*line)[*pos])) {
            return false;
        }
        return true;
    }

    while (*pos < line->size() && !is_space((*line)[*pos])) {
        *value += (*line)[*pos];
        ++(*pos);
    }
    return true;
}

}  // namespace

bool nano_edr::ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line)) {
        return false;
    }

    std::size_t pos = 0;
    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;

    while (pos < line->size()) {
        skip_spaces(line, &pos);
        if (pos == line->size()) {
            break;
        }

        std::string key;
        std::string value;

        if (!parse_key(line, &pos, &key)) {
            return false;
        }

        ++pos;
        if (!parse_value(line, &pos, &value)) {
            return false;
        }

        if (key == "ts" && !has_ts) {
            if (value.empty()) {
                return false;
            }
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
            if (value.empty()) {
                return false;
            }
            out->type = value;
            has_type = true;
        } else if (key == "pid" && !has_pid) {
            out->pid = value;
            has_pid = true;
        } else {
            out->fields.push_back({key, value});
        }
    }

    return has_ts && has_type;
}

bool nano_edr::IsBlankOrComment(const std::string* line) {
    std::size_t first = 0;
    while (first < line->size() && is_space(line->operator[](first))) {
        ++first;
    }
    if (first == line->size()) {
        return true;
    }
    if (line->operator[](first) == '#') {
        return true;
    }
    if (line->operator[](first) == ';') {
        return true;
    }
    return false;
}
