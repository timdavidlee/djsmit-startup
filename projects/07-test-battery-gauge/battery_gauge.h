#pragma once
#include <cstddef>
#include <cstdint>

enum class BatteryStatus : uint8_t { Normal, ReplaceSoon, EndOfLife };

inline uint16_t adc_to_millivolts(uint16_t counts) {
    return static_cast<uint16_t>(static_cast<uint32_t>(counts) * 3300u / 4095u);
}

inline uint16_t average_mv(const uint16_t* samples, std::size_t n) {
    uint16_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        sum = static_cast<uint16_t>(sum + samples[i]);
    }
    return static_cast<uint16_t>(sum / n);
}

inline BatteryStatus classify(uint16_t mv) {
    if (mv > 2600) return BatteryStatus::Normal;
    if (mv >= 2400) return BatteryStatus::ReplaceSoon;
    return BatteryStatus::EndOfLife;
}

class BatteryMonitor {
public:
    BatteryStatus status() const { return status_; }

    void update(uint16_t mv) {
        const BatteryStatus reading = classify(mv);
        if (reading > status_) {
            if (++worse_count_ >= 3) {
                status_ = reading;
                worse_count_ = 0;
            }
        }
    }

private:
    BatteryStatus status_ = BatteryStatus::Normal;
    uint8_t worse_count_ = 0;
};
