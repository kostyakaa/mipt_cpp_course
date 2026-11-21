#include "os_handle.h"

#include <stdexcept>

namespace nano_edr {

os_status OsHandle::Kill(uint32_t pid, uint64_t expected_start_time) {
    return os_process_kill(handle_, pid, expected_start_time);
}

os_status OsHandle::MoveFile(const std::string& from, const std::string& to) {
    return os_file_move(handle_, from.c_str(), to.c_str());
}

os_status OsHandle::DeleteFile(const std::string& path) {
    return os_file_delete(handle_, path.c_str());
}

os_status OsHandle::ProcessInfo(uint32_t pid, os_process_info* out) {
    return os_process_get_info(handle_, pid, out);
}

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
