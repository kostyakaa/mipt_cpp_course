#include "agent.h"

#include <print>
#include <utility>

#include "agent_rules.h"
#include "dedup_sink.h"
#include "rules.h"

namespace nano_edr {

void Agent::PrintContext() const {
    if (window_.size() >= 2) {
        const WindowEntry& entry = window_[window_.size() - 2];
        std::print("[CTX] -2: {} {}", entry.ts.ms, entry.type);
        if (!entry.pid.empty()) {
            std::print(" pid={}", entry.pid);
        }
        std::print("\n");
    }
    if (window_.size() >= 1) {
        const WindowEntry& entry = window_.back();
        std::print("[CTX] -1: {} {}", entry.ts.ms, entry.type);
        if (!entry.pid.empty()) {
            std::print(" pid={}", entry.pid);
        }
        std::print("\n");
    }
}

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

void Agent::PrintContextOnce() {
    if (quiet_ || context_printed_) {
        return;
    }
    PrintContext();
    context_printed_ = true;
}

void Agent::HandleEvent(const Event& event) {
    model_.Observe(event);
    const bool own = responder_ != nullptr && responder_->Recognize(event);
    context_printed_ = false;
    if (!own) {
        engine_.ProcessEvent(event, nullptr);
    }
    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
    window_.PushBack(std::move(entry));
    ++event_count_;
    ++events_by_type_[event.type()];
}

void Agent::OnSkipped() {
    ++skipped_event_count_;
}

void Agent::EnableResponse(OsHandle* os, bool dry_run,
                           const std::string& quarantine_dir) {
    planner_ = std::make_unique<ResponsePlanner>(&model_, quarantine_dir);
    responder_ = std::make_unique<Responder>(os, dry_run);
    engine_.Subscribe([this](const Detection& detection) {
        responder_->Execute(planner_->Plan(detection));
    });
}

void Agent::OnBroken() {
    ++broken_event_count_;
}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("событий {}\n", event_count_);
    if (responder_ != nullptr) {
        const ResponseStats& stats = responder_->stats();
        std::print("[STAT] действий запланировано {}, повторов отброшено {}\n",
                   stats.planned, stats.skipped_repeat);
        std::print("[STAT] завершено процессов {}, изолировано файлов {}, удалено файлов {}\n",
                   stats.killed, stats.quarantined, stats.deleted);
        if (stats.own_events != 0) {
            std::print("[STAT] своих событий {}\n", stats.own_events);
        }
    }
    std::print("пропущенных строк {}\n", skipped_event_count_);
    std::print("битых событий {}\n", broken_event_count_);
    std::print("[STAT] в модели процессов {}, файлов {}\n",
               model_.process_count(), model_.file_count());
    if (suppressed_ != 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n",
                   suppressed_, dedup_period_ms_);
    }
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), window_.capacity());
    for (const auto& [type, count] : events_by_type_) {
        std::print("{}: {}\n", type, count);
    }
    std::print("правила, детектов:\n");
    for (std::size_t index = 0; index < engine_.size(); ++index) {
        const IRule& rule = engine_.rule(index);
        std::print("  {}  {}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
