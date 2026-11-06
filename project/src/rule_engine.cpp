#include "rule_engine.h"

#include <utility>

namespace nano_edr {

void RuleEngine::AddRule(std::unique_ptr<IRule> rule) {
    rules_.push_back(std::move(rule));
}

std::size_t RuleEngine::ProcessEvent(
    const Event& event, std::vector<const IRule*>* fired) {
    std::size_t detection_count = 0;
    for (const std::unique_ptr<IRule>& rule : rules_) {
        if (!rule->Check(event)) {
            continue;
        }
        ++detection_count;
        if (fired != nullptr) {
            fired->push_back(rule.get());
        }
    }
    return detection_count;
}

}  // namespace nano_edr
