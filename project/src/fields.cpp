#include "fields.h"

#include <cctype>
#include <charconv>
#include <stdexcept>

namespace nano_edr {

namespace {

const std::string temp_path = "\\appdata\\local\\temp";

const std::string variables[] = {"%temp%", "%tmp%"};

char lower_ascii(char character) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

std::string lower_ascii(const std::string& text) {
    std::string result = text;

    for (char& character : result) {
        character = lower_ascii(character);
    }

    return result;
}

}  // namespace

const std::string* FindField(const Event& event, const std::string& key) {
    // Возвращаем nullptr поскольку отсутствие необязательного поля не является ошибкой
    for (const Field& field : event.fields) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    // Выбрасываем исключение поскольку отсутствие обязательного является ошибкой
    const std::string* value = FindField(event, key);

    if (value == nullptr) {
        throw std::invalid_argument("Обязательное поле не найдено: " + key);
    }

    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    // Ошибок не выбрасываем, поле key не обязательно должно существовать
    // Ошибка парсинга данных тоже ожидаемая ошибка, не требует выброса исключений
    const std::string* field = FindField(event, key);

    if (field == nullptr) {
        return false;
    }

    uint64_t parsed = 0;

    const char* begin = field->data();
    const char* end = begin + field->size();

    const auto [position, error] = std::from_chars(begin, end, parsed);

    if (error != std::errc{} || position != end) {
        return false;
    }

    *out = parsed;
    return true;
}

uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    // Ошибок не выбрасываем, поле key не обязательно должно существовать
    uint64_t value = 0;

    if (!GetIntField(event, key, &value)) {
        return fallback;
    }

    return value;
}

bool IsProcessStart(const Event& event) {
    return event.type == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    // Ошибок не выбрасываем, поле path не обязательно должно существовать
    const std::string* path = FindField(event, "path");

    if (path == nullptr) {
        return false;
    }

    const std::string lower_path = lower_ascii(*path);
    const std::string lower_suffix = lower_ascii(suffix);

    return lower_path.ends_with(lower_suffix);
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    // Ошибок не выбрасываем, поле cmdline не обязательно должно существовать
    const std::string* command_line = FindField(event, "cmdline");

    if (command_line == nullptr) {
        return false;
    }

    const std::string lower_command_line = lower_ascii(*command_line);
    const std::string lower_needle = lower_ascii(needle);

    return lower_command_line.find(lower_needle) != std::string::npos;
}

std::string NormalizePath(const std::string& path) {
    // Ошибок не выбрасываем поскольку функция никак не проверяет путь на корректность
    // просто приводит его в нормализованный вид
    std::string expanded = lower_ascii(path);

    for (const std::string& variable : variables) {
        std::size_t position = 0;

        while ((position = expanded.find(variable, position)) != std::string::npos) {
            expanded.replace(position, variable.size(), temp_path);
            position += temp_path.size();
        }
    }

    std::string normalized;
    normalized.reserve(expanded.size());

    for (char character : expanded) {
        if (character == '/') {
            character = '\\';
        }

        if (character == '\\' && !normalized.empty() && normalized.back() == '\\') {
            continue;
        }

        normalized += character;
    }

    return normalized;
}
}  // namespace nano_edr
