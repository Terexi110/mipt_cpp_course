#include <print>
#include <string>

#include "agent_rules.h"
#include "event.h"
#include "agent.h"
#include "rule.h"
#include "rules.h"

namespace nano_edr {

namespace {

Event MakeEventFromOs(const os_event* ev) {
    EventParts parst;
    if (ev == nullptr) {
        throw std::invalid_argument("os_event is null");
    }
    parst.ts = std::to_string(ev->ts);
    parst.type = ev->type ? ev->type : "";
    parst.pid = (ev->pid == 0) ? "" : std::to_string(ev->pid);
    for (size_t i = 0; i < ev->field_count; ++i) {
        parst.fields.push_back(Field{ev->fields[i].key, ev->fields[i].value});
    }
    return Event(parst);
}

}  // namespace

Agent::Agent(std::size_t window_size, bool quiet) : window_(window_size), rules_(BuildAgentRules()), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    types_[event.type()]++;
    total_events_++;

    size_t detect_count = 0;
    for (auto& rule : rules_) {
        if (rule.Check(event)) {
            std::print("[DETECT] {}  {}  ts={} pid={}\n",
                        SeverityName(rule.severity()), rule.id(), event.raw_ts(), event.pid());
            ++detect_count;
        }
    }
    
    if (detect_count > 0 && !quiet_) {
        PrintContext(previous_event_ ? &*previous_event_ : nullptr, window_);
    }
    const EventNode* last = window_.back();
    if (last != nullptr) {
        previous_event_ = last->event;
    } else {
        previous_event_.reset();
    }
    window_.PushBack(event);
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    auto* self = static_cast<Agent*>(ctx);
    try {
        Event event = MakeEventFromOs(ev);
        self->HandleEvent(event);
    } catch (const std::exception& e) {
        std::print(stderr, "ошибка обработки события: {}\n", e.what());
        if (ctx != nullptr) {
            self->RecordError();
        }
    }
}

void Agent::PrintSummary() const {
    std::print("всего событий: {}\n", total_events_);
    std::print("ошибок обработки: {}\n", error_events_);
    std::print("типы событий:\n");
    for (const auto& [type, count] : types_) {
        std::print("  {}: {}\n", type, count);
    }
    std::print("правила, детектов:\n");
    for (const auto& rule : rules_) {
        std::print("  {}  {}\n", rule.id(), rule.hits());
    }
}

void Agent::RecordError() {
    error_events_++;
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