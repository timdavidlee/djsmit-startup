#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Battery gauge for an implantable device. See the project README for the specification.

enum class BatteryStatus : uint8_t { Normal, ReplaceSoon, EndOfLife };

// --- Voltage measurement -------------------------------------------------------------

// Converts a 12-bit ADC reading to millivolts.
uint16_t adc_to_millivolts(uint16_t counts);

// Mean of n readings, rounded down.
uint16_t average_mv(const uint16_t* samples, std::size_t n);

// Mean of n readings with the lowest and the highest one left out.
uint16_t trimmed_mean_mv(const uint16_t* samples, std::size_t n);

// True when a reading is one a healthy battery and ADC could produce.
bool is_plausible(uint16_t mv);

BatteryStatus classify(uint16_t mv);

// --- Charge accounting ---------------------------------------------------------------

// Adds up the charge the device has drawn from the battery.
class ChargeCounter {
   public:
    void add(uint32_t current_ua, uint32_t seconds);
    uint32_t consumed_uah() const;

   private:
    uint64_t microamp_seconds_ = 0;
};

uint8_t percent_remaining(uint32_t consumed_uah, uint32_t capacity_uah);

uint8_t months_remaining(uint32_t remaining_uah, uint32_t avg_current_ua);

// --- Status tracking -----------------------------------------------------------------

class BatteryMonitor {
   public:
    BatteryStatus status() const { return status_; }
    bool sensor_fault() const;

    // Feed one voltage reading.
    void update(uint16_t mv);

    // Call once every 24 hours.
    void tick_day();

   private:
    BatteryStatus status_ = BatteryStatus::Normal;
    uint8_t worse_count_ = 0;
    uint8_t implausible_run_ = 0;
    uint16_t eri_days_ = 0;
};

// --- Top-level gauge -----------------------------------------------------------------

struct BatteryReport {
    BatteryStatus status;
    uint16_t last_mv;
    uint8_t percent;
    uint8_t months;
    bool sensor_fault;
};

class BatteryGauge {
   public:
    static constexpr std::size_t kWindow = 8;

    explicit BatteryGauge(uint32_t capacity_uah);

    // Feed one raw ADC reading of the battery voltage.
    void add_sample(uint16_t adc_counts);

    // Record that the device drew current_ua for the given number of seconds.
    void add_load(uint32_t current_ua, uint32_t seconds);

    // Call once every 24 hours.
    void tick_day();

    uint16_t last_mv() const { return last_mv_; }

    BatteryReport report(uint32_t avg_current_ua) const;

   private:
    uint32_t capacity_uah_;
    std::array<uint16_t, kWindow> window_{};
    std::size_t fill_ = 0;
    uint16_t last_mv_ = 0;
    ChargeCounter charge_;
    BatteryMonitor monitor_;
};
