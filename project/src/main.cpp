#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "../kit/include/l1.2/event_list.h"
#include "../kit/include/l1.2/parse.h"

bool parse_window_size(const std::string* value, std::size_t* result) {
    if (value->empty()) return false;

    std::size_t number = 0;
    for (char cur_char : *value) {
        if (cur_char < '0' || cur_char > '9') return false;

        const std::size_t digit = static_cast<std::size_t>(cur_char - '0');
        if (number > (std::numeric_limits<std::size_t>::max() - digit) / 10) return false;
        number = number * 10 + digit;
    }

    *result = number;
    return true;
}

void print_context(const nano_edr::EventList* window) {
    long long position = -static_cast<long long>(window->size);
    for (const nano_edr::EventNode* node = window->head; node != nullptr; node = node->next) {
        const nano_edr::Event* event = &node->event;
        std::print("[CTX] {}: ts={} type={}", position, event->ts, event->type);
        if (!event->pid.empty()) std::print(" pid={}", event->pid);
        std::print("\n");
        ++position;
    }
}

int main(int argc, char** argv) {
    bool is_quiet = false;
    bool has_window_size = false;
    bool invalid_args = false;
    std::size_t window_size = 2;
    std::string log_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quiet") {
            if (is_quiet) {
                invalid_args = true;
                break;
            }
            is_quiet = true;
            continue;
        }
        if (arg == "--window-size") {
            if (has_window_size || i + 1 == argc) {
                invalid_args = true;
                break;
            }
            const std::string value = argv[i + 1];
            if (!parse_window_size(&value, &window_size)) {
                invalid_args = true;
                break;
            }
            has_window_size = true;
            ++i;
            continue;
        }
        if (arg.starts_with('-') || !log_path.empty()) {
            invalid_args = true;
            break;
        }
        log_path = arg;
    }

    if (invalid_args || log_path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--window-size N] [--quiet]\n");
        return 2;
    }

    std::ifstream log(log_path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", log_path);
        return 2;
    }

    const std::vector<std::string> flags = {
        "wscript.exe",
        ".locked",
        "certutil.exe",
        "\\Startup\\"};

    long long lines = 0;
    long long comments = 0;
    long long events = 0;
    std::map<std::string, long long> events_by_type;
    nano_edr::EventList window;
    window.capacity = window_size;
    std::string line;

    while (std::getline(log, line)) {
        ++lines;

        std::size_t first = 0;
        while (first < line.size() && (line[first] == ' ' || line[first] == '\t')) {
            ++first;
        }

        const bool is_blank = first == line.size();
        const bool is_comment = !is_blank && (line[first] == '#' || line[first] == ';');
        if (is_comment) {
            ++comments;
            continue;
        }
        if (is_blank) {
            continue;
        }

        nano_edr::Event event;
        if (!nano_edr::ParseEventLine(&line, &event)) continue;

        ++events;
        ++events_by_type[event.type];

        bool has_detect = false;
        for (const std::string& flag : flags) {
            if (line.contains(flag)) {
                if (!is_quiet && !has_detect) print_context(&window);
                has_detect = true;
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, flag, line);
            }
        }

        nano_edr::ListPushBack(&window, &event);
    }

    if (!is_quiet) {
        std::print("строк {}, из них комментариев {}\n", lines, comments);
        std::print("событий {}\n", events);
        for (const auto& [type, count] : events_by_type) {
            std::print("{}: {}\n", type, count);
        }
    }

    return 0;
}
