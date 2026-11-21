#ifndef NANO_EDR_WINDOW_ENTRY_H
#define NANO_EDR_WINDOW_ENTRY_H

#include <string>
#include <utility>

#include "copy_stats.h"
#include "event.h"

namespace nano_edr {

struct WindowEntry {
    Timestamp ts;
    std::string type;
    std::string pid;

    WindowEntry() = default;
    ~WindowEntry() = default;
    WindowEntry(const WindowEntry& other)
        : ts(other.ts), type(other.type), pid(other.pid) {
        ++copy_stats().window_copied;
    }
    WindowEntry& operator=(const WindowEntry& other) {
        ts = other.ts;
        type = other.type;
        pid = other.pid;
        ++copy_stats().window_copied;
        return *this;
    }
    WindowEntry(WindowEntry&& other) noexcept
        : ts(other.ts), type(std::move(other.type)), pid(std::move(other.pid)) {
        ++copy_stats().window_moved;
    }
    WindowEntry& operator=(WindowEntry&& other) noexcept {
        ts = other.ts;
        type = std::move(other.type);
        pid = std::move(other.pid);
        ++copy_stats().window_moved;
        return *this;
    }
};

}  // namespace nano_edr

#endif
