#pragma once
#include <cstddef>
#include <cstdint>

#include "packet.h"

// Telemetry link, layer 2: turns the stream of received bytes into packets.
// See the project README for the specification.

class FrameReader {
   public:
    static constexpr std::size_t kQueueCapacity = 4;

    // Feed the next n received bytes. Returns the number of packets added to the queue.
    std::size_t feed(const uint8_t* data, std::size_t n);

    // Moves the oldest queued packet into out. Returns false if the queue is empty.
    bool pop(Packet& out);

    std::size_t pending() const { return count_; }
    uint32_t dropped() const { return dropped_; }
    uint32_t rejected() const { return rejected_; }

    void reset();

   private:
    std::size_t push(uint8_t byte);
    void discard(std::size_t n);
    bool enqueue(const Packet& p);

    uint8_t buf_[kMaxFrame] = {};  // candidate frame; buf_[0] is SYNC whenever have_ > 0
    std::size_t have_ = 0;         // bytes in buf_
    std::size_t need_ = 0;         // full length of the candidate, known once have_ >= 2
    std::size_t head_ = 0;         // index of the oldest queued packet
    std::size_t count_ = 0;        // packets queued
    uint32_t dropped_ = 0;
    uint32_t rejected_ = 0;
    Packet queue_[kQueueCapacity] = {};
};
