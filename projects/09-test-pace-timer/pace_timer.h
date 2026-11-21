#pragma once
#include <cstdint>

class PaceTimer {
public:
    bool start(uint32_t now_ms, uint32_t interval_ms) {
        armed_ = false;
        if (interval_ms < 300 || interval_ms >= 2000) return false;
        start_ = now_ms;
        interval_ = interval_ms;
        armed_ = true;
        return true;
    }

    void stop() { armed_ = false; }

    bool armed() const { return armed_; }

    bool expired(uint32_t now_ms) const {
        return armed_ && now_ms >= start_ + interval_;
    }

    uint32_t remaining(uint32_t now_ms) const {
        if (!armed_) return 0;
        return interval_ - (now_ms - start_);
    }

private:
    uint32_t start_ = 0;
    uint32_t interval_ = 0;
    bool     armed_ = false;
};
