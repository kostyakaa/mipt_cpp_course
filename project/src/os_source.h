#ifndef NANO_EDR_OS_SOURCE_H
#define NANO_EDR_OS_SOURCE_H

#include <string>

#include "event_source.h"
#include "os_handle.h"

namespace nano_edr {

class OsSource : public IEventSource {
 public:
    explicit OsSource(const std::string& path);

    void Run(IEventHandler* handler) override;

 private:
    static void Trampoline(const os_event* source,
                           void* raw_context) noexcept;

    OsHandle source_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_OS_SOURCE_H
