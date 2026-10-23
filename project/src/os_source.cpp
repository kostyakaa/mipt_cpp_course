#include "os_source.h"

#include <stdexcept>
#include <string>

#include "event.h"

namespace {

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

}  // namespace

namespace nano_edr {

OsSource::OsSource(const std::string& path) : source_(path) {}

void OsSource::Run(IEventHandler* handler) {
    source_.Subscribe(Trampoline, handler);
    source_.Start();

    os_status status = source_.Wait(100);
    while (status == OS_TIMEOUT) {
        handler->OnTick();
        status = source_.Wait(100);
    }

    if (status != OS_OK) {
        throw std::runtime_error(
            std::string("ошибка источника: ") + os_status_str(status));
    }
}

void OsSource::Trampoline(const os_event* source,
                          void* raw_context) noexcept {
    IEventHandler* handler = static_cast<IEventHandler*>(raw_context);
    if (handler == nullptr) {
        return;
    }

    try {
        if (source == nullptr) {
            throw std::runtime_error("источник передал пустое событие");
        }
        handler->HandleEvent(Event(ConvertEvent(*source)));
    } catch (...) {
        handler->OnBroken();
    }
}

}  // namespace nano_edr
