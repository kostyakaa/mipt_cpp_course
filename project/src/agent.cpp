#include "agent.h"

#include <print>

#include "agent_rules.h"
#include "rules.h"

namespace {

void PrintContext(const nano_edr::EventList& window) {
    const std::size_t count = window.size() < 2 ? window.size() : 2;
    std::size_t skip = window.size() - count;
    const nano_edr::EventNode* node = window.head();

    while (skip > 0) {
        node = node->next;
        --skip;
    }

    long long position = -static_cast<long long>(count);
    while (node != nullptr) {
        const nano_edr::Event& event = node->event;
        std::print("[CTX] {}: ts={} type={}", position, event.raw_ts(),
                   event.type());
        if (!event.pid().empty()) {
            std::print(" pid={}", event.pid());
        }
        std::print("\n");
        node = node->next;
        ++position;
    }
}

}  // namespace

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet)
    : window_(window_size), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    const std::size_t detections =
        CheckRules(event, AgentRules(), AgentRuleCount());
    if (detections != 0 && !quiet_) {
        PrintContext(window_);
    }
    window_.PushBack(event);
    ++event_count_;
    ++events_by_type_[event.type()];
}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {}\n", event_count_);
    for (const auto& [type, count] : events_by_type_) {
        std::print("{}: {}\n", type, count);
    }
}

}  // namespace nano_edr
