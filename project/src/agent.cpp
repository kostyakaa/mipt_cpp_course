#include "agent.h"

#include <print>

#include "agent_rules.h"
#include "rules.h"

namespace {

void PrintContext(const nano_edr::EventList& context) {
    const nano_edr::EventNode* node = context.head();
    long long position = -static_cast<long long>(context.size());
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
    : window_(window_size),
      context_(window_size == 0 || window_size > 2 ? 2 : window_size),
      quiet_(quiet),
      rules_(BuildAgentRules()) {}

void Agent::HandleEvent(const Event& event) {
    std::size_t detections = 0;
    for (MatchRule& rule : rules_) {
        if (!rule.Check(event)) {
            continue;
        }
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule.severity()), rule.id(),
                   event.raw_ts(), event.pid());
        ++detections;
    }
    if (detections != 0 && !quiet_) {
        PrintContext(context_);
    }
    window_.PushBack(event);
    context_.PushBack(event);
    ++event_count_;
    ++events_by_type_[event.type()];
}

void Agent::HandleBrokenEvent() noexcept {
    ++broken_event_count_;
}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {}\n", event_count_);
    std::print("битых событий {}\n", broken_event_count_);
    for (const auto& [type, count] : events_by_type_) {
        std::print("{}: {}\n", type, count);
    }
    std::print("правила, детектов:\n");
    for (const MatchRule& rule : rules_) {
        std::print("  {}  {}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
