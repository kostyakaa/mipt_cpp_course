#include "file_source.h"

#include <exception>
#include <stdexcept>
#include <string>

#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : input_(path) {
    if (!input_) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}

void FileSource::Run(IEventHandler* handler) {
    std::string line;
    while (std::getline(input_, line)) {
        if (IsBlankOrComment(line)) {
            handler->OnSkipped();
            continue;
        }

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            handler->OnBroken();
            continue;
        }
        try {
            handler->HandleEvent(Event(parts));
        } catch (const std::exception&) {
            handler->OnBroken();
        }
    }
}

}  // namespace nano_edr
