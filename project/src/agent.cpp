#include "agent.h"

#include <print>
#include <vector>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

void Agent::PrintContext() const {
    if (window_.size() >= 2) {
        const WindowEntry& entry = window_[window_.size() - 2];
        std::print("[CTX] -2: {} {}", entry.ts.ms, entry.type);
        if (!entry.pid.empty()) {
            std::print(" pid={}", entry.pid);
        }
        std::print("\n");
    }
    if (window_.size() >= 1) {
        const WindowEntry& entry = window_.back();
        std::print("[CTX] -1: {} {}", entry.ts.ms, entry.type);
        if (!entry.pid.empty()) {
            std::print(" pid={}", entry.pid);
        }
        std::print("\n");
    }
}

Agent::Agent(bool quiet)
    : engine_(BuildAgentRules()),
      quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    std::vector<const IRule*> fired;
    const std::size_t detections = engine_.ProcessEvent(event, &fired);
    if (detections != 0 && !quiet_) {
        PrintContext();
    }
    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule->severity()), rule->id(),
                   event.raw_ts(), event.pid());
    }
    window_.PushBack({event.ts(), event.type(), event.pid()});
    ++event_count_;
    ++events_by_type_[event.type()];
}

void Agent::OnSkipped() {
    ++skipped_event_count_;
}

void Agent::OnBroken() {
    ++broken_event_count_;
}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {}\n", event_count_);
    std::print("пропущенных строк {}\n", skipped_event_count_);
    std::print("битых событий {}\n", broken_event_count_);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), window_.capacity());
    for (const auto& [type, count] : events_by_type_) {
        std::print("{}: {}\n", type, count);
    }
    std::print("правила, детектов:\n");
    for (std::size_t index = 0; index < engine_.size(); ++index) {
        const IRule& rule = engine_.rule(index);
        std::print("  {}  {}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
