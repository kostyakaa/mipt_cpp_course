#include <charconv>
#include <cstdio>
#include <exception>
#include <print>
#include <string>

#include "agent.h"
#include "file_source.h"
#include "os_source.h"

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
        nano_edr::Agent agent(options.window_size, options.quiet);
        if (options.file_source) {
            nano_edr::FileSource source(options.source_path);
            source.Run(&agent);
        } else {
            nano_edr::OsSource source(options.source_path);
            source.Run(&agent);
        }
        agent.PrintSummary();
        return 0;
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}
