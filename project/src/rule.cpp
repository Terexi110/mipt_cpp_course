#include "rule.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "entity_model.h"

namespace nano_edr {

// --- RuleBase ---

RuleBase::RuleBase(const std::string& id, Severity severity)
    : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

// --- MatchRule ---

MatchRule::MatchRule(const std::string& id, Severity severity)
    : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event& event) {
    for (const auto& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

// --- SequenceRule ---

SequenceRule::SequenceRule(const std::string& id, Severity severity, uint64_t window_ms)
    : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}

void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Linked(const Pending& first, const Event& second) const {
    if (child_of_prev_) {
        if (model_ == nullptr) {
            return false;
        }
        const ProcessRecord* proc = model_->FindProcess(second.pid());
        return proc != nullptr && proc->ppid == first.pid;
    }
    if (same_pid_) {
        return first.pid == second.pid();
    }
    return true;
}

bool SequenceRule::Check(const Event& event) {
    if (!first_ || !second_) {
        return false;
    }

    const uint64_t current_ts = event.ts().ms;
    std::erase_if(pending_, [current_ts, this](const Pending& p) {
        return current_ts >= p.ts_ms && (current_ts - p.ts_ms > window_ms_);
    });

    if (second_->Matches(event)) {
        auto it = std::find_if(pending_.begin(), pending_.end(), [&](const Pending& p) {
            return Linked(p, event);
        });

        if (it != pending_.end()) {
            pending_.erase(it);
            CountHit();
            return true;
        }
    }

    if (first_->Matches(event)) {
        pending_.push_back({current_ts, event.pid()});
    }

    return false;
}

// --- LambdaRule ---

LambdaRule::LambdaRule(const std::string& id, Severity severity, Predicate predicate)
    : RuleBase(id, severity), predicate_(std::move(predicate)) {}

bool LambdaRule::Check(const Event& event) {
    if (predicate_ && predicate_(event)) {
        CountHit();
        return true;
    }
    return false;
}

}  // namespace nano_edr
