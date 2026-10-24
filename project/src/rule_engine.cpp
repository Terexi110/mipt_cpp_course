#include "rule_engine.h"
#include <cstddef>

namespace nano_edr {

void RuleEngine::AddRule(std::unique_ptr<IRule> rule) {
    rules_.push_back(std::move(rule));
}

std::size_t RuleEngine::ProcessEvent(const Event& event, std::vector<const IRule*>* fired) {
    std::size_t counter_hist = 0;
    for (const auto& rule : rules_) {
        if (rule->Check(event)) {
            counter_hist++;
            if (fired != nullptr) {
                fired->push_back(rule.get());
            }
        }
    }
    return counter_hist;
}

}