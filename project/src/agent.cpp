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

void Agent::EnableResponse(OsHandle* os, bool dry_run,
                           const std::string& quarantine_dir) {
    planner_ = std::make_unique<ResponsePlanner>(&model_, quarantine_dir);
    responder_ = std::make_unique<Responder>(os, dry_run);
    engine_.Subscribe([this](const Detection& detection) {
        responder_->Execute(planner_->Plan(detection));
    });
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

    if (responder_ != nullptr && responder_->Recognize(event)) {
        PushToWindow(event);
        return;
    }

    context_printed_for_event_ = false;

    engine_.ProcessEvent(event, nullptr);

    PushToWindow(event);
}

void Agent::PushToWindow(const Event& event) {
    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
    window_.PushBack(std::move(entry));
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
    if (responder_ != nullptr) {
        const ResponseStats& stats = responder_->stats();
        std::print("[STAT] действий запланировано {}, повторов отброшено {}\n",
                   stats.planned, stats.skipped_repeat);
        std::print("[STAT] завершено процессов {}, изолировано файлов {}, удалено файлов {}\n",
                   stats.killed, stats.quarantined, stats.deleted);
        if (stats.own_events > 0) {
            std::print("[STAT] своих событий {}\n", stats.own_events);
        }
    }
    if (suppressed_ > 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n", suppressed_, dedup_period_ms_);
    }
}

}  // namespace nano_edr
