#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <string>
#include <unordered_map>

#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"

namespace nano_edr {

struct WindowEntry {
    Timestamp ts;
    std::string type;
    std::string pid;
};

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;
    explicit Agent(bool quiet);

    void HandleEvent(const Event& event) override;

    void OnSkipped() override;
    void OnBroken() override;

    void OnTick() override;

    void PrintSummary() const;

 private:
    void PrintContext();

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    RuleEngine engine_;
    bool quiet_ = false;
    std::size_t total_events_ = 0;
    std::unordered_map<std::string, std::size_t> types_;
    std::size_t skipped_events_ = 0;
    std::size_t broken_events_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
