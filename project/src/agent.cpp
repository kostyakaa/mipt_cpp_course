#include "agent.h"

#include <print>
#include <vector>

#include "agent_rules.h"
#include "rules.h"

namespace {

void PrintContext(const nano_edr::RingBuffer<nano_edr::WindowEntry,
                                             nano_edr::Agent::kWindowCapacity>& window) {
    const std::size_t count = window.size() < 2 ? window.size() : 2;
    for (std::size_t index = window.size() - count; index < window.size(); ++index) {
        const nano_edr::WindowEntry& entry = window[index];
        const long long position = static_cast<long long>(index) -
                                   static_cast<long long>(window.size());
        std::print("[CTX] {}: {} {}", position, entry.ts.ms, entry.type);
        if (!entry.pid.empty()) {
            std::print(" pid={}", entry.pid);
        }
        std::print("\n");
    }
}

}  // namespace

namespace nano_edr {

Agent::Agent(bool quiet)
    : engine_(BuildAgentRules()),
      quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    std::vector<const IRule*> fired;
    const std::size_t detections = engine_.ProcessEvent(event, &fired);
    if (detections != 0 && !quiet_) {
        PrintContext(window_);
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
