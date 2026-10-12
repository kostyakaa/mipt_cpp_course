#ifndef NANO_EDR_SOURCES_H
#define NANO_EDR_SOURCES_H

#include <fstream>
#include <string>

#include "os_handle.h"

namespace nano_edr {

class Agent;

class FileSource {
 public:
    explicit FileSource(const std::string& path);

    void Run(Agent* agent);

 private:
    std::ifstream input_;
};

class OsSource {
 public:
    explicit OsSource(const std::string& path);

    void Run(Agent* agent);

 private:
    static void Trampoline(const os_event* source,
                           void* raw_context) noexcept;

    OsHandle source_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_SOURCES_H
