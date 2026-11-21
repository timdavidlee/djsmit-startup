#pragma once
#include <cstdint>
#include <cstring>

struct Entry {
    uint32_t timestamp_ms;
    uint16_t code;
    char     msg[16];
};

class EventLog {
public:
    static const int kCapacity = 16;

    void log(uint32_t timestamp_ms, uint16_t code, const char* msg) {
        Entry& e = entries_[next_];
        e.timestamp_ms = timestamp_ms;
        e.code = code;
        std::strncpy(e.msg, msg, sizeof(e.msg));
        next_ = (next_ + 1) % kCapacity;
        if (next_ == 0) overflowed_ = true;
        ++count_;
    }

    // Returns the i-th oldest entry still stored, or nullptr if out of range.
    const Entry* get(int index) const {
        if (index <= count_) return &entries_[index];
        return nullptr;
    }

    // Most recently logged entry.
    const Entry& latest() const { return entries_[next_ - 1]; }

    int  count() const { return count_; }
    bool overflowed() const { return overflowed_; }

private:
    Entry entries_[kCapacity];
    int   next_ = 0;
    int   count_ = 0;
    bool  overflowed_;
};
