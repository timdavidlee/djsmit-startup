#pragma once
#include <cstdint>

// Timing service for the pacing channel. See the project README for the specification.
// All times are in milliseconds, read from a free-running 32-bit counter.

// --- Rate and interval ---------------------------------------------------------------

// Pacing interval for a rate in beats per minute, or 0 if the rate is not allowed.
uint32_t rate_to_interval_ms(uint16_t rate_bpm);

// Rate in beats per minute for a pacing interval, or 0 if the interval is not allowed.
uint16_t interval_to_rate_bpm(uint32_t interval_ms);

// How long to wait for the heart's own beat before pacing.
uint32_t escape_interval_ms(uint32_t base_ms, uint32_t hysteresis_ms, bool last_beat_sensed);

// --- One-shot timer ------------------------------------------------------------------

class PaceTimer {
   public:
    bool start(uint32_t now_ms, uint32_t interval_ms);

    // Runs the same interval again, counted from now_ms.
    bool restart(uint32_t now_ms);

    void stop();

    bool armed() const { return armed_; }
    bool expired(uint32_t now_ms) const;
    uint32_t remaining(uint32_t now_ms) const;

   private:
    uint32_t start_ = 0;
    uint32_t interval_ = 0;
    bool armed_ = false;
};

// --- Refractory window ---------------------------------------------------------------

// The period after a beat during which sensed signals are ignored.
class RefractoryWindow {
   public:
    void begin(uint32_t now_ms, uint32_t duration_ms);
    bool active(uint32_t now_ms) const;

   private:
    uint32_t start_ = 0;
    uint32_t duration_ = 0;
    bool begun_ = false;
};

// --- Upper rate limit ----------------------------------------------------------------

// Refuses pacing pulses that would follow the previous one too closely.
class RateLimiter {
   public:
    explicit RateLimiter(uint32_t min_interval_ms);

    // Asks permission to deliver a pacing pulse now.
    bool allow(uint32_t now_ms);

   private:
    uint32_t min_interval_;
    uint32_t last_ = 0;
};
