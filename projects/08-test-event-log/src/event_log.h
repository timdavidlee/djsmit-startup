#pragma once
#include <cstddef>
#include <cstdint>

// Event log for an implantable device. See the project README for the specification.

enum class Severity : uint8_t { Info = 0, Warning = 1, Alarm = 2 };

// One logged event. The top two bits of the code are its severity; the low 14 bits say
// what happened.
struct Entry {
    uint32_t timestamp_ms;
    uint16_t code;
    char msg[16];
};

// True unless the code's severity bits hold the reserved value 3.
bool is_valid_code(uint16_t code);

Severity severity_of(uint16_t code);

class EventLog {
   public:
    static constexpr int kCapacity = 16;
    static constexpr int kSeverityCount = 3;
    static constexpr std::size_t kRecordSize = 22;

    // Stores one event. Returns false if the code is not valid.
    bool log(uint32_t timestamp_ms, uint16_t code, const char* msg);

    // Returns the i-th oldest entry still stored, or nullptr if out of range.
    const Entry* get(int index) const;

    // Most recently logged entry.
    const Entry& latest() const;

    int count() const { return count_; }
    bool overflowed() const { return overflowed_; }

    // Events logged with this severity, including ones already overwritten.
    uint32_t count_by_severity(Severity severity) const;

    // Stored events that are at most window_ms old at time now_ms.
    int count_since(uint32_t now_ms, uint32_t window_ms) const;

    // Most recently logged stored event with exactly this code, or nullptr.
    const Entry* find_latest(uint16_t code) const;

    // Copies the stored events into out for telemetry read-out. Returns bytes written.
    std::size_t dump(uint8_t* out, std::size_t cap) const;

    // What the programmer shows first when a clinician opens the log.
    struct Summary {
        int stored;
        bool overflowed;
        uint32_t oldest_ms;
        uint32_t newest_ms;
        uint32_t by_severity[kSeverityCount];
    };
    Summary summary() const;

    // Empties the log.
    void clear();

   private:
    Entry entries_[kCapacity];
    uint32_t severity_counts_[kSeverityCount] = {};
    int next_ = 0;
    int count_ = 0;
    bool overflowed_;
};

// Walks the stored events of a log from oldest to newest.
class EventCursor {
   public:
    explicit EventCursor(const EventLog& log) : log_(&log) {}

    // Copies the next event into out. Returns false when there are no more.
    bool next(Entry& out);

    // Starts again from the oldest event.
    void rewind() { position_ = 0; }

   private:
    const EventLog* log_;
    int position_ = 0;
};
