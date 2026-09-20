#include <cstdio>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    bool is_quiet = false;
    std::string log_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quiet") {
            if (is_quiet) {
                std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
                return 2;
            }
            is_quiet = true;
        } else if (arg.starts_with('-') || !log_path.empty()) {
            std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
            return 2;
        } else {
            log_path = arg;
        }
    }

    if (log_path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
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

        ++events;

        const std::size_t type_begin = line.find("type=");
        if (type_begin != std::string::npos) {
            const std::size_t value_begin = type_begin + 5;
            const std::size_t value_end = line.find_first_of(" \t", value_begin);
            const std::string type = line.substr(value_begin, value_end - value_begin);
            ++events_by_type[type];
        }

        for (const std::string& flag : flags) {
            if (line.contains(flag)) {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, flag, line);
            }
        }
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
