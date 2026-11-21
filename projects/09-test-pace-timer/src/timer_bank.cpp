#include "timer_bank.h"

bool TimerBank::start(uint8_t slot, uint32_t now_ms, uint32_t interval_ms) {
    if (slot >= kSlots) return false;
    return timers_[slot].start(now_ms, interval_ms);
}

bool TimerBank::restart(uint8_t slot, uint32_t now_ms) {
    if (slot >= kSlots) return false;
    return timers_[slot].restart(now_ms);
}

bool TimerBank::stop(uint8_t slot) {
    if (slot >= kSlots) return false;
    timers_[slot].stop();
    return true;
}

void TimerBank::stop_all() {
    for (PaceTimer& timer : timers_) {
        timer.stop();
    }
}

bool TimerBank::expired(uint8_t slot, uint32_t now_ms) const {
    if (slot >= kSlots) return false;
    return timers_[slot].expired(now_ms);
}

uint32_t TimerBank::remaining(uint8_t slot, uint32_t now_ms) const {
    if (slot >= kSlots) return 0;
    return timers_[slot].remaining(now_ms);
}

bool TimerBank::next_expiry(uint32_t now_ms, uint8_t& slot) const {
    bool found = false;
    uint32_t least = 0;
    uint8_t winner = 0;
    for (uint8_t i = 0; i < kSlots; ++i) {
        if (!timers_[i].armed()) continue;
        const uint32_t left = timers_[i].remaining(now_ms);
        if (!found || left <= least) {
            least = left;
            winner = i;
            found = true;
        }
    }
    if (found) slot = winner;
    return found;
}
