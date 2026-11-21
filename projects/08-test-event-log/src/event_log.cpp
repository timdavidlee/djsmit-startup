#include "event_log.h"

#include <cstring>

namespace {

// The top two bits of an event code are its severity.
constexpr int kSeverityShift = 14;

}  // namespace

bool is_valid_code(uint16_t code) { return (code >> kSeverityShift) < EventLog::kSeverityCount; }

Severity severity_of(uint16_t code) { return static_cast<Severity>(code >> kSeverityShift); }

bool EventLog::log(uint32_t timestamp_ms, uint16_t code, const char* msg) {
    Entry& e = entries_[next_];
    e.timestamp_ms = timestamp_ms;
    e.code = code;
    std::strncpy(e.msg, msg, sizeof(e.msg));

    next_ = (next_ + 1) % kCapacity;
    if (next_ == 0) overflowed_ = true;
    ++count_;
    severity_counts_[static_cast<uint8_t>(severity_of(code))] += 1;
    return true;
}

const Entry* EventLog::get(int index) const {
    if (index > count_) return nullptr;
    return &entries_[index];
}

const Entry& EventLog::latest() const { return entries_[next_ - 1]; }

uint32_t EventLog::count_by_severity(Severity severity) const {
    return severity_counts_[static_cast<uint8_t>(severity)];
}

int EventLog::count_since(uint32_t now_ms, uint32_t window_ms) const {
    int n = 0;
    for (int i = 0; i < count_; ++i) {
        const Entry* e = get(i);
        const int age_ms = static_cast<int>(now_ms) - static_cast<int>(e->timestamp_ms);
        if (age_ms <= static_cast<int>(window_ms)) ++n;
    }
    return n;
}

const Entry* EventLog::find_latest(uint16_t code) const {
    for (int i = count_ - 1; i >= 0; --i) {
        const Entry* e = get(i);
        if (e->code == code) return e;
    }
    return nullptr;
}

std::size_t EventLog::dump(uint8_t* out, std::size_t cap) const {
    std::size_t written = 0;
    for (int i = 0; i < count_; ++i) {
        if (written >= cap) break;

        const Entry* e = get(i);
        uint8_t* record = out + written;
        *reinterpret_cast<uint32_t*>(record) = e->timestamp_ms;
        *reinterpret_cast<uint16_t*>(record + 4) = e->code;
        std::memcpy(record + 6, e->msg, sizeof(e->msg));
        written += kRecordSize;
    }
    return written;
}

EventLog::Summary EventLog::summary() const {
    Summary s{};
    s.stored = count_;
    s.overflowed = overflowed_;
    if (count_ > 0) {
        s.oldest_ms = get(0)->timestamp_ms;
        s.newest_ms = latest().timestamp_ms;
    }
    for (int i = 0; i < kSeverityCount; ++i) s.by_severity[i] = severity_counts_[i];
    return s;
}

void EventLog::clear() {
    count_ = 0;
    overflowed_ = false;
    for (uint32_t& n : severity_counts_) n = 0;
}

bool EventCursor::next(Entry& out) {
    const Entry* e = log_->get(position_);
    if (e == nullptr) return false;
    out = *e;
    ++position_;
    return true;
}
