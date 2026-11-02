#include "limit_handler.h"

namespace nano_edr {

LimitHandler::LimitHandler(IEventHandler* next, std::size_t max_events) : next_(next), max_events_(max_events) {}

void LimitHandler::OnSkipped() {
    next_->OnSkipped();
}

void LimitHandler::OnBroken() {
    next_->OnBroken();
}

void LimitHandler::OnTick() {
    next_->OnTick();
}

void LimitHandler::HandleEvent(const Event& event) {
    if (passed_ < max_events_) {
        passed_++;
        next_->HandleEvent(event);
    }
    else {
        dropped_++;
    }
}

std::size_t LimitHandler::dropped() const {
    return dropped_;
}

}

