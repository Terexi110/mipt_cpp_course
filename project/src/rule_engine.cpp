#include "rule_engine.h"

#include <cstddef>

namespace nano_edr {

void RuleEngine::AddRule(std::unique_ptr<IRule> rule) {
    rules_.push_back(std::move(rule));
}

void RuleEngine::Subscribe(AlertSink sink) {
    sinks_.push_back(std::move(sink));
}

std::size_t RuleEngine::ProcessEvent(const Event& event, std::vector<const IRule*>* fired) {
    std::size_t hits = 0;
    for (const auto& rule : rules_) {
        if (rule->Check(event)) {
            hits++;
            if (fired != nullptr) {
                fired->push_back(rule.get());
            }
            if (!sinks_.empty()) {
                Detection d = MakeDetection(*rule, event);
                for (const auto& sink : sinks_) {
                    sink(d);
                }
            }
        }
    }
    return hits;
}

Detection RuleEngine::MakeDetection(const IRule& rule, const Event& event) const {
    return Detection{
        .rule = rule.id(),
        .severity = rule.severity(),
        .ts = event.ts(),
        .pid = event.pid(),
        .chain = (model_ != nullptr) ? model_->DescribeChain(event.pid()) : "",
        .actions = rule.actions(),
    };
}

}  // namespace nano_edr
