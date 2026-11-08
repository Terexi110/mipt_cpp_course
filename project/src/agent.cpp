#include <cstddef>
#include <print>
#include <string>

#include "agent_rules.h"
#include "event.h"
#include "agent.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(bool quiet)
    : engine_(BuildAgentRules()), quiet_(quiet) {}

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
        PrintContext();
    }

    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
            SeverityName(rule->severity()), rule->id(), event.raw_ts(), event.pid());
    }

    window_.PushBack(WindowEntry{
        .ts = event.ts(),
        .type = std::string(event.type()),
        .pid = std::string(event.pid()),
    });
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
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), kWindowCapacity);
}

void Agent::PrintContext() {
    if (window_.size() >= 2) {
        const auto& prev = window_[window_.size() - 2];
        std::print("[CTX] -2: {} {} pid={}\n", prev.ts.ms, prev.type, prev.pid);
    }
    if (!window_.empty()) {
        const auto& last = window_.back();
        std::print("[CTX] -1: {} {} pid={}\n", last.ts.ms, last.type, last.pid);
    }
}

}  // namespace nano_edr
