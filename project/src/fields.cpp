#include "fields.h"

#include <charconv>
#include <stdexcept>

#include "text.h"

namespace nano_edr {

namespace {

const std::string kTempPath = "\\appdata\\local\\temp";

const std::string kVariables[] = {"%temp%", "%tmp%"};

}  // namespace

const std::string* FindField(const Event& event, const std::string& key) {
    if (key == "ts") {
        return &event.raw_ts();
    }
    if (key == "type") {
        return &event.type();
    }
    if (key == "pid") {
        return &event.pid();
    }

    // Отсутствие необязательного поля - ожидаемый исход, поэтому nullptr.
    for (const Field& field : event.fields()) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    // Отсутствие обязательного поля нарушает контракт формата.
    const std::string* value = FindField(event, key);

    if (value == nullptr) {
        throw std::invalid_argument("Обязательное поле не найдено: " + key);
    }

    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    // Отсутствующее или битое число - ожидаемая ошибка внешних данных.
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
    // Подстановка покрывает и отсутствующее, и неразбираемое значение.
    uint64_t value = 0;

    if (!GetIntField(event, key, &value)) {
        return fallback;
    }

    return value;
}

bool IsProcessStart(const Event& event) {
    return event.type() == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type() == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type() == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    // Ошибок не выбрасываем, поле path не обязательно должно существовать
    const std::string* path = FindField(event, "path");

    if (path == nullptr) {
        return false;
    }

    return TextEndsWith(*path, suffix);
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    // Ошибок не выбрасываем, поле cmdline не обязательно должно существовать
    const std::string* command_line = FindField(event, "cmdline");

    if (command_line == nullptr) {
        return false;
    }

    return TextContains(*command_line, needle);
}

std::string NormalizePath(const std::string& path) {
    // Ошибок не выбрасываем поскольку функция никак не проверяет путь на корректность
    // просто приводит его в нормализованный вид
    std::string expanded = LowerAscii(path);

    for (const std::string& variable : kVariables) {
        std::size_t position = 0;

        while ((position = expanded.find(variable, position)) != std::string::npos) {
            expanded.replace(position, variable.size(), kTempPath);
            position += kTempPath.size();
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
