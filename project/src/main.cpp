#include <cstdio>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

const std::vector<std::string> flags = {
    "wscript.exe",
    ".locked",
    "certutil.exe",
    "\\Startup\\"};

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--quiet")) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
        return 2;
    }

    const bool is_quiet = argc == 3;

    std::ifstream log(argv[1]);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
        return 2;
    }

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
