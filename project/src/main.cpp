#include <charconv>
#include <cstdio>
#include <exception>
#include <fstream>
#include <map>
#include <print>
#include <stdexcept>
#include <string>

#include "agent_rules.h"
#include "event_list.h"
#include "os_handle.h"
#include "parse.h"
#include "rules.h"

namespace {

struct Options {
    bool quiet = false;
    bool file_source = false;
    std::size_t window_size = 64;
    std::string source_path;
};

bool ParseSize(const std::string& value, std::size_t* result) {
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto [position, error] = std::from_chars(begin, end, *result);
    return error == std::errc{} && position == end;
}

bool ParseArguments(int argc, char** argv, Options* options) {
    bool has_window_size = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--quiet") {
            if (options->quiet) {
                return false;
            }
            options->quiet = true;
        } else if (argument == "--window-size") {
            if (has_window_size || index + 1 == argc) {
                return false;
            }
            has_window_size = true;
            if (!ParseSize(argv[++index], &options->window_size)) {
                return false;
            }
        } else if (argument == "--file") {
            if (options->file_source || !options->source_path.empty() || index + 1 == argc) {
                return false;
            }
            options->file_source = true;
            options->source_path = argv[++index];
        } else if (argument.starts_with('-') || !options->source_path.empty()) {
            return false;
        } else {
            options->source_path = argument;
        }
    }

    return !options->source_path.empty();
}

void PrintContext(const nano_edr::EventList& window) {
    const std::size_t count = window.size() < 2 ? window.size() : 2;
    std::size_t skip = window.size() - count;
    const nano_edr::EventNode* node = window.head();

    while (skip > 0) {
        node = node->next;
        --skip;
    }

    long long position = -static_cast<long long>(count);
    while (node != nullptr) {
        const nano_edr::Event& event = node->event;
        std::print("[CTX] {}: ts={} type={}", position, event.raw_ts(),
                   event.type());
        if (!event.pid().empty()) {
            std::print(" pid={}", event.pid());
        }
        std::print("\n");
        node = node->next;
        ++position;
    }
}

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet) {}

    void HandleEvent(const nano_edr::Event& event) {
        const std::size_t detections = nano_edr::CheckRules(
            event, nano_edr::AgentRules(), nano_edr::AgentRuleCount());
        if (detections != 0 && !quiet_) {
            PrintContext(window_);
        }
        window_.PushBack(event);
        ++event_count_;
        ++events_by_type_[event.type()];
    }

    void PrintSummary() const {
        if (quiet_) {
            return;
        }
        std::print("событий {}\n", event_count_);
        for (const auto& [type, count] : events_by_type_) {
            std::print("{}: {}\n", type, count);
        }
    }

 private:
    nano_edr::EventList window_;
    bool quiet_;
    std::size_t event_count_ = 0;
    std::map<std::string, std::size_t> events_by_type_;
};

nano_edr::EventParts ConvertEvent(const os_event& source) {
    nano_edr::EventParts parts;
    parts.ts = std::to_string(source.ts);
    if (source.type != nullptr) {
        parts.type = source.type;
    }
    if (source.pid != 0) {
        parts.pid = std::to_string(source.pid);
    }
    parts.fields.reserve(source.field_count);
    for (std::size_t index = 0; index < source.field_count; ++index) {
        const os_kv& field = source.fields[index];
        parts.fields.push_back(
            {field.key == nullptr ? "" : field.key,
             field.value == nullptr ? "" : field.value});
    }
    return parts;
}

struct CallbackContext {
    Agent* agent = nullptr;
    std::exception_ptr error;
};

class FileSource {
 public:
    explicit FileSource(const std::string& path) : input_(path) {
        if (!input_) {
            throw std::runtime_error("не удалось открыть журнал: " + path);
        }
    }

    void Run(Agent* agent) {
        std::string line;
        while (std::getline(input_, line)) {
            if (nano_edr::IsBlankOrComment(line)) {
                continue;
            }

            nano_edr::EventParts parts;
            if (!nano_edr::ParseEventParts(line, &parts)) {
                continue;
            }
            try {
                agent->HandleEvent(nano_edr::Event(parts));
            } catch (const std::invalid_argument&) {
                // Битое событие - скип
            }
        }
    }

 private:
    std::ifstream input_;
};

class OsSource {
 public:
    explicit OsSource(const std::string& path) : source_(path) {}

    void Run(Agent* agent) {
        CallbackContext context{agent, nullptr};
        source_.Subscribe(OnEvent, &context);
        source_.Start();

        os_status status = OS_TIMEOUT;
        while ((status = source_.Wait(100)) == OS_TIMEOUT) {
        }
        source_.Stop();

        if (context.error != nullptr) {
            std::rethrow_exception(context.error);
        }
        if (status != OS_OK) {
            throw std::runtime_error(
                std::string("ошибка источника: ") + os_status_str(status));
        }
    }

 private:
    static void OnEvent(const os_event* source, void* raw_context) noexcept {
        CallbackContext* context = static_cast<CallbackContext*>(raw_context);
        if (context == nullptr || context->agent == nullptr ||
            context->error != nullptr) {
            return;
        }

        try {
            if (source == nullptr) {
                throw std::runtime_error(
                    "источник передал пустое событие");
            }
            context->agent->HandleEvent(
                nano_edr::Event(ConvertEvent(*source)));
        } catch (...) {
            context->error = std::current_exception();
        }
    }

    nano_edr::OsHandle source_;
};

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!ParseArguments(argc, argv, &options)) {
        std::print(stderr,
                   "использование: nano-edr [--file] <журнал.log> "
                   "[--window-size N] [--quiet]\n");
        return 2;
    }

    try {
        Agent agent(options.window_size, options.quiet);
        if (options.file_source) {
            FileSource source(options.source_path);
            source.Run(&agent);
        } else {
            OsSource source(options.source_path);
            source.Run(&agent);
        }
        agent.PrintSummary();
        return 0;
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}
