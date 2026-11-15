#include <cstdio>
#include <exception>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include "agent.h"
#include "alert_sink.h"
#include "edr_handle.h"
#include "event_source.h"
#include "field_traits.h"
#include "file_source.h"
#include "os_source.h"

namespace {

struct Options {
    bool quiet = false;
    bool file_source = false;
    std::string source_path;
    std::string out;
};

bool ParseArguments(int argc, char** argv, Options* options) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--quiet") {
            if (options->quiet) {
                return false;
            }
            options->quiet = true;
        } else if (argument == "--out") {
            if (!options->out.empty() || index + 1 == argc) {
                return false;
            }
            options->out = argv[++index];
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
                   "[--quiet] [--out файл]\n");
        return 2;
    }

    try {
        nano_edr::EdrHandle edr(options.source_path);
        const std::string text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
            dedup_period_ms = 10000;
        }
        nano_edr::Agent agent(options.quiet, dedup_period_ms);
        nano_edr::FanOutSink targets;
        targets.Add(nano_edr::ConsoleSink());
        if (!options.out.empty()) {
            targets.Add(nano_edr::FileSink(options.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));
        std::unique_ptr<nano_edr::IEventSource> source;
        if (options.file_source) {
            source =
                std::make_unique<nano_edr::FileSource>(options.source_path);
        } else {
            source =
                std::make_unique<nano_edr::OsSource>(options.source_path);
        }
        source->Run(&agent);
        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();
        return 0;
    } catch (const std::exception& error) {
        std::print(stderr, "ошибка: {}\n", error.what());
        return 1;
    }
}
