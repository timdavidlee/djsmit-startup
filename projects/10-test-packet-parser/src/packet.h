#pragma once
#include <cstddef>
#include <cstdint>

// Telemetry link, layer 1: one frame on the wire.
// See the project README for the specification.

constexpr uint8_t kSync = 0xA5;
constexpr std::size_t kMaxPayload = 32;
constexpr std::size_t kFrameOverhead = 3;  // SYNC, LEN, CHK
constexpr std::size_t kMaxFrame = kMaxPayload + kFrameOverhead;

struct Packet {
    uint8_t len;
    uint8_t payload[kMaxPayload];
};

// XOR of n bytes.
uint8_t checksum(const uint8_t* p, std::size_t n);

// Writes one frame into buf. Returns bytes written, or 0 if the frame cannot be written.
std::size_t encode(const Packet& in, uint8_t* buf, std::size_t cap);

// Returns true and fills out if buf starts with one valid frame.
bool parse(const uint8_t* buf, std::size_t size, Packet& out);
