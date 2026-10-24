#include <string>

#include "rule.h"

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity) : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string &action) {
    actions_.push_back(action);
}


MatchRule::MatchRule(const std::string& id, Severity severity) : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event &event) {
    for (const auto& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}


SequenceRule::SequenceRule(const std::string& id, Severity severity, uint64_t window_ms) : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}


void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Check(const Event& event) {
    if (!first_ || !second_) {
        return false;
    }

    const uint64_t current_ts = event.ts().ms;

    std::vector<Pending> fresh;
    for (const auto& p : pending_) {
        if (current_ts < p.ts_ms || (current_ts - p.ts_ms <= window_ms_)) {
            fresh.push_back(p);
        }
    }
    pending_ = std::move(fresh);

    if (second_->Matches(event)) {
        auto it = std::find_if(pending_.begin(), pending_.end(), [&](const Pending& p) {
            return !same_pid_ || p.pid == event.pid();
        });

        if (it != pending_.end()) {
            pending_.erase(it);
            CountHit();
            return true;
        }
    }

    if (first_->Matches(event)) {
        pending_.push_back(Pending{current_ts, event.pid()});
    }

    return false;
}

}