#include "rule.h"

#include <algorithm>
#include <utility>

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity)
    : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

MatchRule::MatchRule(const std::string& id, Severity severity)
    : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event& event) {
    for (const std::unique_ptr<ICondition>& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

SequenceRule::SequenceRule(const std::string& id, Severity severity,
                           uint64_t window_ms)
    : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}

void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Check(const Event& event) {
    if (first_ == nullptr || second_ == nullptr) {
        return false;
    }

    const uint64_t event_time = event.ts().ms;
    std::erase_if(pending_, [event_time, this](const Pending& pending) {
        return event_time > pending.ts_ms &&
               event_time - pending.ts_ms > window_ms_;
    });

    if (second_->Matches(event)) {
        const auto match = std::find_if(
            pending_.begin(), pending_.end(),
            [&event, this](const Pending& pending) {
                return !same_pid_ || pending.pid == event.pid();
            });
        if (match != pending_.end()) {
            pending_.erase(match);
            CountHit();
            return true;
        }
    }

    if (first_->Matches(event)) {
        pending_.push_back({event_time, event.pid()});
    }
    return false;
}

}  // namespace nano_edr
