#include "detection.h"

#include <format>

namespace nano_edr {

std::string ToString(const Detection& detection) {
    std::string result = std::format("[DETECT] {}  {}  ts={} pid={}",
                                     SeverityName(detection.severity), detection.rule,
                                     detection.ts.ms, detection.pid);
    if (!detection.chain.empty()) {
        result += " chain=" + detection.chain;
    }
    return result;
}

}  // namespace nano_edr
