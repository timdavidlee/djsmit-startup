#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

struct Packet {
    uint8_t len;
    uint8_t payload[32];
};

inline uint8_t checksum(const uint8_t* p, std::size_t n) {
    uint8_t c = 0;
    for (std::size_t i = 0; i < n; ++i) c ^= p[i];
    return c;
}

// Returns bytes written, or 0 if the frame does not fit in cap.
inline std::size_t encode(const Packet& in, uint8_t* buf, std::size_t cap) {
    const std::size_t total = in.len + 3u;
    if (cap < total) return 0;
    buf[0] = 0xA5;
    buf[1] = in.len;
    std::memcpy(buf + 2, in.payload, in.len);
    buf[2 + in.len] = checksum(in.payload, in.len);
    return total;
}

// Returns true and fills out if buf starts with one valid frame.
inline bool parse(const uint8_t* buf, std::size_t size, Packet& out) {
    if (size == 0 || buf[0] != 0xA5) return false;
    const uint8_t len = buf[1];
    std::memcpy(out.payload, buf + 2, len);
    out.len = len;
    return buf[2 + len] == checksum(out.payload, len);
}
