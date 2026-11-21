#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event_source.h"
#include "response.h"
#include "ring_buffer.h"
#include "rule_engine.h"
#include "window_entry.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    Agent(bool quiet, uint64_t dedup_period_ms);

    void Subscribe(AlertSink sink);
    void EnableResponse(OsHandle* os, bool dry_run,
                        const std::string& quarantine_dir);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;
    void PrintSummary() const;

 private:
    void PrintContext() const;
    void PrintContextOnce();

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    EntityModel model_;
    RuleEngine engine_;
    std::unique_ptr<ResponsePlanner> planner_;
    std::unique_ptr<Responder> responder_;
    bool quiet_ = false;
    bool context_printed_ = false;
    uint64_t dedup_period_ms_ = 0;
    std::size_t suppressed_ = 0;
    std::size_t event_count_ = 0;
    std::size_t skipped_event_count_ = 0;
    std::size_t broken_event_count_ = 0;
    std::map<std::string, std::size_t> events_by_type_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
