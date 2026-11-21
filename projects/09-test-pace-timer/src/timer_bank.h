#pragma once
#include <array>
#include <cstdint>

#include "pace_timer.h"

// A fixed set of timers, one per timing job the device runs at the same time.
// See the project README for the specification.
class TimerBank {
   public:
    static constexpr uint8_t kSlots = 4;

    bool start(uint8_t slot, uint32_t now_ms, uint32_t interval_ms);
    bool restart(uint8_t slot, uint32_t now_ms);
    bool stop(uint8_t slot);
    void stop_all();

    bool expired(uint8_t slot, uint32_t now_ms) const;
    uint32_t remaining(uint8_t slot, uint32_t now_ms) const;

    // Finds the armed timer that is due first.
    bool next_expiry(uint32_t now_ms, uint8_t& slot) const;

   private:
    std::array<PaceTimer, kSlots> timers_{};
};
