#include "rule_engine.h"

#include <utility>

#include "copy_stats.h"

namespace nano_edr {

void RuleEngine::Subscribe(AlertSink sink) {
    sinks_.push_back(std::move(sink));
}

Detection RuleEngine::MakeDetection(const IRule& rule, const Event& event) const {
    Detection detection;
    detection.evidence.push_back(std::make_unique<Event>(event));
    ++copy_stats().evidence_copied;
    detection.rule = rule.id();
    detection.severity = rule.severity();
    detection.ts = event.ts();
    detection.pid = event.pid();
    detection.actions = rule.actions();
    if (model_ != nullptr) {
        detection.chain = model_->DescribeChain(event.pid());
    }
    return detection;
}

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
        const Detection detection = MakeDetection(*rule, event);
        for (const AlertSink& sink : sinks_) {
            sink(detection);
        }
        if (fired != nullptr) {
            fired->push_back(rule.get());
        }
    }
    return detection_count;
}

}  // namespace nano_edr
