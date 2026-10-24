#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <optional>
#include <string>
#include <unordered_map>

#include "os.h"
#include "event.h"
#include "event_list.h"
#include "rule_engine.h"
#include "event_source.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    void HandleEvent(const Event& event) override;

    void OnSkipped() override;
    void OnBroken() override;

    void OnTick() override;

    Agent(std::size_t window_size, bool quiet);

    void PrintSummary() const;

 private:
    void PrintContext(const Event* previous, const EventList& window);

    EventList window_;
    std::optional<Event> previous_event_;
    RuleEngine engine_;
    bool quiet_ = false;
    std::size_t total_events_ = 0;
    std::unordered_map<std::string, std::size_t> types_;
    std::size_t skipped_events_ = 0;
    std::size_t broken_events_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H