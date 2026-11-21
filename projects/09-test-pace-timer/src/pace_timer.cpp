#include "pace_timer.h"

namespace {

constexpr uint32_t kMsPerMinute = 60000;

constexpr uint32_t kMinIntervalMs = 300;
constexpr uint32_t kMaxIntervalMs = 2000;

constexpr uint16_t kMinRateBpm = 30;
constexpr uint16_t kMaxRateBpm = 220;

}  // namespace

// --- Rate and interval ---------------------------------------------------------------

uint32_t rate_to_interval_ms(uint16_t rate_bpm) {
    if (rate_bpm < kMinRateBpm || rate_bpm > kMaxRateBpm) return 0;
    return kMsPerMinute / rate_bpm;
}

uint16_t interval_to_rate_bpm(uint32_t interval_ms) {
    if (interval_ms < kMinIntervalMs || interval_ms > kMaxIntervalMs) return 0;
    return static_cast<uint16_t>(kMsPerMinute / interval_ms);
}

uint32_t escape_interval_ms(uint32_t base_ms, uint32_t hysteresis_ms, bool last_beat_sensed) {
    const uint32_t interval = base_ms > kMaxIntervalMs ? kMaxIntervalMs : base_ms;
    return last_beat_sensed ? interval + hysteresis_ms : interval;
}

// --- One-shot timer ------------------------------------------------------------------

bool PaceTimer::start(uint32_t now_ms, uint32_t interval_ms) {
    armed_ = false;
    if (interval_ms < kMinIntervalMs || interval_ms >= kMaxIntervalMs) return false;
    start_ = now_ms;
    interval_ = interval_ms;
    armed_ = true;
    return true;
}

bool PaceTimer::restart(uint32_t now_ms) {
    start_ = now_ms;
    armed_ = interval_ != 0;
    return armed_;
}

void PaceTimer::stop() { armed_ = false; }

bool PaceTimer::expired(uint32_t now_ms) const { return armed_ && now_ms >= start_ + interval_; }

uint32_t PaceTimer::remaining(uint32_t now_ms) const {
    if (!armed_) return 0;
    return interval_ - (now_ms - start_);
}

// --- Refractory window ---------------------------------------------------------------

void RefractoryWindow::begin(uint32_t now_ms, uint32_t duration_ms) {
    start_ = now_ms;
    duration_ = duration_ms;
    begun_ = true;
}

bool RefractoryWindow::active(uint32_t now_ms) const {
    return begun_ && now_ms - start_ < duration_;
}

// --- Upper rate limit ----------------------------------------------------------------

RateLimiter::RateLimiter(uint32_t min_interval_ms) : min_interval_(min_interval_ms) {}

bool RateLimiter::allow(uint32_t now_ms) {
    if (now_ms - last_ < min_interval_) return false;
    last_ = now_ms;
    return true;
}
