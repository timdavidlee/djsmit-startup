#include "frame_reader.h"

#include <cstring>

std::size_t FrameReader::feed(const uint8_t* data, std::size_t n) {
    std::size_t queued = 0;
    for (std::size_t i = 0; i < n; ++i) queued += push(data[i]);
    return queued;
}

bool FrameReader::pop(Packet& out) {
    if (count_ == 0) return false;
    out = queue_[head_];
    head_ = (head_ + 1) % kQueueCapacity;
    --count_;
    return true;
}

void FrameReader::reset() {
    have_ = 0;
    need_ = 0;
    head_ = 0;
    count_ = 0;
    dropped_ = 0;
    rejected_ = 0;
}

// Takes one received byte. Returns the number of packets it completed and queued.
std::size_t FrameReader::push(uint8_t byte) {
    if (have_ == 0 && byte != kSync) return 0;  // still looking for SYNC

    buf_[have_++] = byte;
    if (have_ == 2) need_ = buf_[1] + kFrameOverhead;

    std::size_t queued = 0;
    while (have_ >= 2) {
        if (buf_[1] > kMaxPayload) {
            ++rejected_;
            discard(1);
            continue;
        }
        if (have_ < need_) break;  // frame not complete yet

        Packet p{};
        if (parse(buf_, need_, p)) {
            if (enqueue(p)) ++queued;
            discard(need_);
        } else {
            ++rejected_;
            discard(1);
        }
    }
    return queued;
}

// Drops the first n buffered bytes, then any bytes before the next SYNC.
void FrameReader::discard(std::size_t n) {
    std::size_t next = n;
    while (next < have_ && buf_[next] != kSync) ++next;
    std::memmove(buf_, buf_ + next, have_ - next);
    have_ -= next;
}

bool FrameReader::enqueue(const Packet& p) {
    if (count_ == kQueueCapacity) {
        ++rejected_;
        return false;
    }
    queue_[head_ + count_] = p;
    ++count_;
    return true;
}
