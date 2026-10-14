#include "os_handle.h"

#include <stdexcept>

namespace nano_edr {

namespace {

std::runtime_error OsError(const std::string& action, os_status status) {
    return std::runtime_error(action + ": " + os_status_str(status));
}

}  // namespace

OsHandle::OsHandle(const std::string& config_path) {
    const os_status status = os_init(config_path.c_str(), &handle_);
    if (status != OS_OK) {
        throw OsError("не удалось открыть " + config_path, status);
    }
}

OsHandle::~OsHandle() {
    os_free(handle_);
}

void OsHandle::Subscribe(os_event_cb callback, void* context) {
    const os_status status = os_event_subscribe(handle_, callback, context);
    if (status != OS_OK) {
        throw OsError("не удалось подписаться на события", status);
    }
}

void OsHandle::Start() {
    const os_status status = os_start(handle_);
    if (status != OS_OK) {
        throw OsError("не удалось запустить источник событий", status);
    }
}

os_status OsHandle::Wait(uint32_t timeout_ms) {
    return os_wait(handle_, timeout_ms);
}

void OsHandle::Stop() {
    os_stop(handle_);
}

}  // namespace nano_edr
