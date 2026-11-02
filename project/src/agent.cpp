#include <cstddef>
#include <print>
#include <string>

#include "agent_rules.h"
#include "event.h"
#include "agent.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(std::size_t window_size, bool quiet) : window_(window_size), engine_(BuildAgentRules()), quiet_(quiet) {}

void Agent::OnSkipped() {
    skipped_events_++;
}

void Agent::OnBroken() {
    broken_events_++;
}

void Agent::OnTick() {
    // 
}

void Agent::HandleEvent(const Event& event) {
    types_[event.type()]++;
    total_events_++;

    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);

    if (!fired.empty() && !quiet_) {
        PrintContext(previous_event_ ? &*previous_event_ : nullptr, window_);
    }
    
    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
            SeverityName(rule->severity()), rule->id(), event.raw_ts(), event.pid());
    }
    
    const EventNode* last = window_.back();
    if (last != nullptr) {
        previous_event_ = last->event;
    } else {
        previous_event_.reset();
    }
    window_.PushBack(event);
}

void Agent::PrintSummary() const {
    std::print("всего событий: {}\n", total_events_);
    std::print("пропущено (пустые/комментарии): {}\n", skipped_events_);
    std::print("ошибок/битых событий: {}\n", broken_events_);
    std::print("ошибок обработки: {}\n", broken_events_);
    std::print("типы событий:\n");
    for (const auto& [type, count] : types_) {
        std::print("  {}: {}\n", type, count);
    }
    std::print("правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const auto& rule = engine_.rule(i);
        std::print("  {}  {}\n", rule.id(), rule.hits());
    }
}

void Agent::PrintContext(const Event* previous, const EventList& window) {
    const EventNode* last = window.back();
    if (previous != nullptr && window.size() > 1) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", previous->raw_ts(), previous->type(), previous->pid());
    }
    if (last != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", last->event.raw_ts(), last->event.type(), last->event.pid());
    }
}


}  // namespace nano_edr