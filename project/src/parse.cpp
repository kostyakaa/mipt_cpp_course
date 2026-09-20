#include "../kit/include/l1.2/parse.h"

namespace {

void skip_spaces(const std::string* line, std::size_t* pos) {
    while (*pos < line->size() && ((*line)[*pos] == ' ' || (*line)[*pos] == '\t')) {
        ++(*pos);
    }
}

bool parse_key(const std::string* line, std::size_t* pos, std::string* key) {
    while (*pos < line->size()) {
        char cur_char = (*line)[*pos];
        if (cur_char == '=') break;
        if (cur_char == ' ' || cur_char == '\t' || cur_char == '"') return false;

        *key += cur_char;
        ++(*pos);
    }

    return !key->empty() && *pos < line->size() && (*line)[*pos] == '=';
}

bool parse_value(const std::string* line, std::size_t* pos, std::string* value) {
    if (*pos == line->size()) return true;

    if ((*line)[*pos] == '"') {
        ++(*pos);
        while (*pos < line->size() && (*line)[*pos] != '"') {
            *value += (*line)[*pos];
            ++(*pos);
        }

        if (*pos == line->size()) return false;
        ++(*pos);
        if (*pos < line->size() && (*line)[*pos] != ' ' && (*line)[*pos] != '\t') return false;
        return true;
    }

    while (*pos < line->size() && (*line)[*pos] != ' ' && (*line)[*pos] != '\t') {
        *value += (*line)[*pos];
        ++(*pos);
    }
    return true;
}

}  // namespace

bool nano_edr::ParseEventLine(const std::string* line, Event* out) {
    *out = Event{};
    if (IsBlankOrComment(line)) return false;

    std::size_t pos = 0;
    bool has_ts = false;
    bool has_type = false;
    bool has_pid = false;

    while (pos < line->size()) {
        skip_spaces(line, &pos);
        if (pos == line->size()) break;

        std::string key, value;
        if (!parse_key(line, &pos, &key)) return false;

        ++pos;
        if (!parse_value(line, &pos, &value)) return false;

        if (key == "ts" && !has_ts) {
            out->ts = value;
            has_ts = true;
        } else if (key == "type" && !has_type) {
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
    while (first < line->size() && (line->operator[](first) == ' ' || line->operator[](first) == '\t')) {
        ++first;
    }
    if (first == line->size()) return true;
    if (line->operator[](first) == '#') return true;
    if (line->operator[](first) == ';') return true;
    return false;
}
