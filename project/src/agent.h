#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event_list.h"

namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);

    void HandleEvent(const Event& event);
    void HandleBrokenEvent() noexcept;
    void PrintSummary() const;

 private:
    EventList window_;
    EventList context_;
    bool quiet_ = false;
    std::size_t event_count_ = 0;
    std::size_t broken_event_count_ = 0;
    std::map<std::string, std::size_t> events_by_type_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
