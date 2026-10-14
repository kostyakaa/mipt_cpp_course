#include "os_source.h"

#include <exception>
#include <stdexcept>
#include <string>

#include "agent.h"
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

struct CallbackContext {
    nano_edr::Agent* agent = nullptr;
    std::exception_ptr error;
};

}  // namespace

namespace nano_edr {

OsSource::OsSource(const std::string& path) : source_(path) {}

void OsSource::Run(Agent* agent) {
    CallbackContext context{agent, nullptr};
    source_.Subscribe(Trampoline, &context);
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

void OsSource::Trampoline(const os_event* source,
                          void* raw_context) noexcept {
    CallbackContext* context = static_cast<CallbackContext*>(raw_context);
    if (context == nullptr || context->agent == nullptr ||
        context->error != nullptr) {
        return;
    }

    try {
        if (source == nullptr) {
            throw std::runtime_error("источник передал пустое событие");
        }
        context->agent->HandleEvent(Event(ConvertEvent(*source)));
    } catch (const std::exception&) {
        context->agent->HandleBrokenEvent();
    } catch (...) {
        context->error = std::current_exception();
    }
}

}  // namespace nano_edr
