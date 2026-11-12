#include <cstdio>
#include <exception>
#include <memory>
#include <print>
#include <string>

#include "agent.h"
#include "event_source.h"
#include "file_source.h"
#include "os_source.h"

namespace {

struct Options {
    bool quiet = false;
    bool file_source = false;
    std::string source_path;
};

bool ParseArguments(int argc, char** argv, Options* options) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--quiet") {
            if (options->quiet) {
                return false;
            }
            options->quiet = true;
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
                   "[--quiet]\n");
        return 2;
    }

    try {
        nano_edr::Agent agent(options.quiet);
        std::unique_ptr<nano_edr::IEventSource> source;
        if (options.file_source) {
            source =
                std::make_unique<nano_edr::FileSource>(options.source_path);
        } else {
            source =
                std::make_unique<nano_edr::OsSource>(options.source_path);
        }
        source->Run(&agent);
        agent.PrintSummary();
        return 0;
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}
