#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <string>
#include <unordered_map>

#include "event.h"
#include "event_list.h"
#include "os.h"

namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);

    void HandleEvent(const Event& event);

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

    void PrintSummary() const;

 private:
    EventList window_;
    bool quiet_ = false;
    std::size_t total_events_ = 0;
    std::unordered_map<std::string, std::size_t> types_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H