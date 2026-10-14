#include "file_source.h"

#include <exception>
#include <stdexcept>
#include <string>

#include "agent.h"
#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : input_(path) {
    if (!input_) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}

void FileSource::Run(Agent* agent) {
    std::string line;
    while (std::getline(input_, line)) {
        if (IsBlankOrComment(line)) {
            continue;
        }

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            agent->HandleBrokenEvent();
            continue;
        }
        try {
            agent->HandleEvent(Event(parts));
        } catch (const std::exception&) {
            agent->HandleBrokenEvent();
        }
    }
}

}  // namespace nano_edr
