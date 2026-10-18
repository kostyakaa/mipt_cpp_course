#include "rule.h"

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity)
    : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

MatchRule::MatchRule(const std::string& id, Severity severity)
    : RuleBase(id, severity) {}

void MatchRule::AddCondition(const Condition& condition) {
    conditions_.push_back(condition);
}

bool MatchRule::Check(const Event& event) {
    for (const Condition& condition : conditions_) {
        if (!condition(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

}  // namespace nano_edr
