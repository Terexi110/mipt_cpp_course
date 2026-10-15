#include "agent.h"

#include <iostream>
#include <print>
#include <string>

#include "agent_rules.h"
#include "event.h"
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

void PrintContext(const EventList& window) {
    const EventNode* prev = nullptr;
    const EventNode* curr = nullptr;
    for (const EventNode* node = window.head(); node != nullptr; node = node->next) {
        prev = curr;
        curr = node;
    }
    if (prev != nullptr) {
        std::print("[CTX] -2: ts={} type={} pid={}\n", prev->event.raw_ts(), prev->event.type(), prev->event.pid());
    }
    if (curr != nullptr) {
        std::print("[CTX] -1: ts={} type={} pid={}\n", curr->event.raw_ts(), curr->event.type(), curr->event.pid());
    }
}

Agent::Agent(std::size_t window_size, bool quiet) : window_(window_size), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    types_[event.type()]++;
    total_events_++;

    size_t detect_count = CheckRules(event, AgentRules(), AgentRuleCount());
    if (detect_count > 0 && !quiet_) {
        PrintContext(window_);
    }
    window_.PushBack(event);
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    try {
        auto* self = static_cast<Agent*>(ctx);
        Event event = MakeEventFromOs(ev);
        self->HandleEvent(event);
    } catch (const std::exception& e) {
        std::cerr << "ошибка обработки события: " << e.what() << std::endl;
    }
}

void Agent::PrintSummary() const {
    std::print("всего событий: {}\n", total_events_);
    std::print("типы событий:\n");
    for (const auto& [type, count] : types_) {
        std::print("  {}: {}\n", type, count);
    }
}

}  // namespace nano_edr