#ifndef NANO_EDR_DEDUP_SINK_H
#define NANO_EDR_DEDUP_SINK_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "alert_sink.h"

namespace nano_edr {

class DedupSink {
 public:
    DedupSink(AlertSink inner, uint64_t period_ms, std::size_t* suppressed);
    void operator()(const Detection& detection);

 private:
    struct Entry {
        std::string rule;
        std::string pid;
        uint64_t first_ms = 0;
    };
    AlertSink inner_;
    uint64_t period_ms_ = 0;
    std::size_t* suppressed_ = nullptr;
    std::vector<Entry> entries_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_DEDUP_SINK_H
