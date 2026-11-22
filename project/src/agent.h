#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "alert_sink.h"
#include "entity_model.h"
#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "response.h"
#include "rule_engine.h"
#include "window_entry.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    explicit Agent(bool quiet, uint64_t dedup_period_ms = 10000);

    void HandleEvent(const Event& event) override;

    void OnSkipped() override;
    void OnBroken() override;

    void OnTick() override;

    void PrintSummary() const;

    void Subscribe(AlertSink sink);

    void EnableResponse(OsHandle* os, bool dry_run,
                        const std::string& quarantine_dir);

 private:
    void PrintContextOnce();
    void PushToWindow(const Event& event);

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    EntityModel model_;
    RuleEngine engine_;
    bool quiet_ = false;
    uint64_t dedup_period_ms_ = 10000;
    std::size_t suppressed_ = 0;
    bool context_printed_for_event_ = false;
    std::unique_ptr<ResponsePlanner> planner_{};
    std::unique_ptr<Responder> responder_{};

    std::size_t total_events_ = 0;
    std::unordered_map<std::string, std::size_t> types_;
    std::size_t skipped_events_ = 0;
    std::size_t broken_events_ = 0;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
