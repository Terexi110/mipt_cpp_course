#ifndef NANO_EDR_LIMIT_HANDLER_H
#define NANO_EDR_LIMIT_HANDLER_H

#include <cstddef>

#include "event_source.h"

namespace nano_edr {

class LimitHandler : public IEventHandler {
 public:
    LimitHandler(IEventHandler* next, std::size_t max_events);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    std::size_t dropped() const;

 private:
    IEventHandler* next_ = nullptr;
    std::size_t max_events_ = 0;
    std::size_t passed_ = 0;
    std::size_t dropped_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_LIMIT_HANDLER_H
