#include <print>
#include <stdexcept>
#include <utility>

#include "alert_sink.h"
#include "dedup_sink.h"

namespace nano_edr {

void ConsoleSink::operator()(const Detection& detection) const {
    std::print("{}\n", ToString(detection));
}

FileSink::FileSink(const std::string& path) : out_(path) {
    if (!out_.is_open()) {
        throw std::runtime_error("не удалось открыть файл: " + path);
    }
}

void FileSink::operator()(const Detection& detection) {
    out_ << ToString(detection) << '\n';
}

void CountingSink::operator()(const Detection& detection) const {
    ++counter_->total;
    counter_->seen.push_back(SeenAlert{detection.rule, detection.severity,
                                       detection.ts, detection.pid, detection.actions});
}

void FanOutSink::Add(AlertSink sink) {
    sinks_.push_back(std::move(sink));
}

void FanOutSink::operator()(const Detection& detection) const {
    for (const AlertSink& sink : sinks_) {
        sink(detection);
    }
}

DedupSink::DedupSink(AlertSink inner, uint64_t period_ms,
                     std::size_t* suppressed)
    : inner_(std::move(inner)), period_ms_(period_ms), suppressed_(suppressed) {}

void DedupSink::operator()(const Detection& detection) {
    if (period_ms_ == 0) {
        inner_(detection);
        return;
    }
    for (Entry& entry : entries_) {
        if (entry.rule != detection.rule || entry.pid != detection.pid) {
            continue;
        }
        if (detection.ts.ms - entry.first_ms <= period_ms_) {
            if (suppressed_ != nullptr) {
                ++*suppressed_;
            }
            return;
        }
        entry.first_ms = detection.ts.ms;
        inner_(detection);
        return;
    }
    entries_.push_back({detection.rule, detection.pid, detection.ts.ms});
    inner_(detection);
}

}  // namespace nano_edr
