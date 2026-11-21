#include "battery_gauge.h"

namespace {

constexpr uint32_t kAdcMaxCounts = 4095;
constexpr uint32_t kAdcReferenceMv = 3300;

constexpr uint16_t kNormalMinMv = 2600;
constexpr uint16_t kReplaceSoonMinMv = 2400;

constexpr uint16_t kPlausibleMinMv = 1500;
constexpr uint16_t kPlausibleMaxMv = 3300;

constexpr uint8_t kReadingsToWorsen = 3;
constexpr uint8_t kReadingsToFault = 5;
constexpr uint16_t kEriToEolDays = 90;

constexpr uint32_t kSecondsPerHour = 3600;
constexpr uint32_t kHoursPerMonth = 730;
constexpr uint8_t kMaxMonths = 120;

}  // namespace

// --- Voltage measurement -------------------------------------------------------------

uint16_t adc_to_millivolts(uint16_t counts) {
    return static_cast<uint16_t>(static_cast<uint32_t>(counts) * kAdcReferenceMv / kAdcMaxCounts);
}

uint16_t average_mv(const uint16_t* samples, std::size_t n) {
    uint16_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        sum = static_cast<uint16_t>(sum + samples[i]);
    }
    return static_cast<uint16_t>(sum / n);
}

uint16_t trimmed_mean_mv(const uint16_t* samples, std::size_t n) {
    if (n <= 3) return average_mv(samples, n);

    uint32_t sum = 0;
    uint16_t lowest = samples[0];
    uint16_t highest = samples[0];
    for (std::size_t i = 0; i < n; ++i) {
        const uint16_t s = samples[i];
        sum += s;
        if (s < lowest) lowest = s;
        if (s > highest) highest = s;
    }
    return static_cast<uint16_t>((sum - lowest - highest) / (n - 2));
}

bool is_plausible(uint16_t mv) { return mv >= kPlausibleMinMv && mv <= kPlausibleMaxMv; }

BatteryStatus classify(uint16_t mv) {
    if (mv > kNormalMinMv) return BatteryStatus::Normal;
    if (mv >= kReplaceSoonMinMv) return BatteryStatus::ReplaceSoon;
    return BatteryStatus::EndOfLife;
}

// --- Charge accounting ---------------------------------------------------------------

void ChargeCounter::add(uint32_t current_ua, uint32_t seconds) {
    microamp_seconds_ += static_cast<uint64_t>(current_ua) * seconds;
}

uint32_t ChargeCounter::consumed_uah() const {
    const uint64_t uah = microamp_seconds_ / kSecondsPerHour;
    return uah > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(uah);
}

uint8_t percent_remaining(uint32_t consumed_uah, uint32_t capacity_uah) {
    if (capacity_uah == 0 || consumed_uah >= capacity_uah) return 0;
    const uint64_t used_percent = static_cast<uint64_t>(consumed_uah) * 100u / capacity_uah;
    return static_cast<uint8_t>(100u - used_percent);
}

uint8_t months_remaining(uint32_t remaining_uah, uint32_t avg_current_ua) {
    if (avg_current_ua == 0) return kMaxMonths;
    const uint64_t uah_per_month = static_cast<uint64_t>(avg_current_ua) * kHoursPerMonth;
    uint8_t months = static_cast<uint8_t>(remaining_uah / uah_per_month);
    if (months > kMaxMonths) months = kMaxMonths;
    return months;
}

// --- Status tracking -----------------------------------------------------------------

bool BatteryMonitor::sensor_fault() const { return implausible_run_ >= kReadingsToFault; }

void BatteryMonitor::update(uint16_t mv) {
    if (!is_plausible(mv)) {
        if (implausible_run_ < kReadingsToFault) ++implausible_run_;
        return;
    }
    implausible_run_ = 0;

    const BatteryStatus reading = classify(mv);
    if (reading > status_) {
        if (++worse_count_ >= kReadingsToWorsen) {
            status_ = reading;
            worse_count_ = 0;
        }
    }
}

void BatteryMonitor::tick_day() {
    if (status_ != BatteryStatus::ReplaceSoon) return;
    if (++eri_days_ > kEriToEolDays) status_ = BatteryStatus::EndOfLife;
}

// --- Top-level gauge -----------------------------------------------------------------

BatteryGauge::BatteryGauge(uint32_t capacity_uah) : capacity_uah_(capacity_uah) {}

void BatteryGauge::add_sample(uint16_t adc_counts) {
    window_[fill_++] = adc_to_millivolts(adc_counts);
    if (fill_ < kWindow) return;

    last_mv_ = trimmed_mean_mv(window_.data(), kWindow);
    monitor_.update(last_mv_);
    fill_ = 0;
}

void BatteryGauge::add_load(uint32_t current_ua, uint32_t seconds) {
    charge_.add(current_ua, seconds);
}

void BatteryGauge::tick_day() { monitor_.tick_day(); }

BatteryReport BatteryGauge::report(uint32_t avg_current_ua) const {
    const uint32_t consumed_uah = charge_.consumed_uah();
    const uint32_t remaining_uah = consumed_uah >= capacity_uah_ ? 0 : capacity_uah_ - consumed_uah;

    BatteryReport r{};
    r.status = monitor_.status();
    r.last_mv = last_mv_;
    r.percent = percent_remaining(consumed_uah, capacity_uah_);
    r.months = months_remaining(remaining_uah, avg_current_ua);
    r.sensor_fault = monitor_.sensor_fault();
    return r;
}
