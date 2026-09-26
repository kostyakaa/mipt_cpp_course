#include <charconv>
#include <cstdio>
#include <exception>
#include <fstream>
#include <map>
#include <print>
#include <string>

#include "agent_rules.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"

bool parse_window_size(const std::string* value, std::size_t* result) {
    const char* begin = value->data();
    const char* end = begin + value->size();

    const auto [position, error] =
        std::from_chars(begin, end, *result);

    return error == std::errc{} && position == end;
}

void print_context(const nano_edr::EventList* window) {
    const std::size_t context_size = (window->size < 2 ? window->size : 2);

    std::size_t skip = window->size - context_size;
    const nano_edr::EventNode* node = window->head;

    while (skip > 0) {
        node = node->next;
        --skip;
    }

    long long position = -static_cast<long long>(context_size);

    while (node != nullptr) {
        const nano_edr::Event* event = &node->event;

        std::print(
            "[CTX] {}: ts={} type={}",
            position,
            event->ts,
            event->type);

        if (!event->pid.empty()) {
            std::print(" pid={}", event->pid);
        }

        std::print("\n");

        node = node->next;
        ++position;
    }
}

int main(int argc, char** argv) {
    try {
        bool is_quiet = false;
        bool has_window_size = false;
        bool invalid_args = false;
        std::size_t window_size = 64;
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
            if (!nano_edr::ParseEventLine(&line, &event)) {
                continue;
            }

            ++events;
            ++events_by_type[event.type];

            const std::size_t detection_count = nano_edr::CheckRules(event, nano_edr::AgentRules(), nano_edr::AgentRuleCount());

            if (detection_count > 0 && !is_quiet) {
                print_context(&window);
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
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}