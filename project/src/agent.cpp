#include "agent.h"

#include <cstddef>
#include <print>
#include <string>
#include <utility>

#include "agent_rules.h"
#include "alert_sink.h"
#include "dedup_sink.h"
#include "event.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

Agent::Agent(bool quiet, uint64_t dedup_period_ms)
    : engine_(BuildAgentRules(&model_)),
      quiet_(quiet),
      dedup_period_ms_(dedup_period_ms) {
    engine_.SetModel(&model_);
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_period_ms_, &suppressed_));
}

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
    model_.Observe(event);

    types_[event.type()]++;
    total_events_++;

    context_printed_for_event_ = false;

    engine_.ProcessEvent(event, nullptr);

    window_.PushBack({
        .ts = event.ts(),
        .type = event.type(),
        .pid = event.pid(),
    });
}

void Agent::PrintContextOnce() {
    if (context_printed_for_event_ || quiet_) {
        return;
    }
    context_printed_for_event_ = true;
    if (window_.size() >= 2) {
        const auto& prev = window_[window_.size() - 2];
        std::print("[CTX] -2: {} {} pid={}\n", prev.ts.ms, prev.type, prev.pid);
    }
    if (!window_.empty()) {
        const auto& last = window_.back();
        std::print("[CTX] -1: {} {} pid={}\n", last.ts.ms, last.type, last.pid);
    }
}

void Agent::PrintSummary() const {
    std::print("всего событий: {}\n", total_events_);
    std::print("пропущено (пустые/комментарии): {}\n", skipped_events_);
    std::print("ошибок/битых событий: {}\n", broken_events_);
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
    std::print("[STAT] в модели процессов {}, файлов {}\n", model_.process_count(), model_.file_count());
    if (suppressed_ > 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n", suppressed_, dedup_period_ms_);
    }
}

}  // namespace nano_edr
