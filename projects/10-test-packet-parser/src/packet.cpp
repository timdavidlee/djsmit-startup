#include "packet.h"

#include <cstring>

uint8_t checksum(const uint8_t* p, std::size_t n) {
    uint8_t c = 0;
    for (std::size_t i = 0; i < n; ++i) c ^= p[i];
    return c;
}

std::size_t encode(const Packet& in, uint8_t* buf, std::size_t cap) {
    const std::size_t total = in.len + kFrameOverhead;
    if (cap < total) return 0;
    buf[0] = kSync;
    buf[1] = in.len;
    std::memcpy(buf + 2, in.payload, in.len);
    buf[2 + in.len] = checksum(in.payload, in.len);
    return total;
}

bool parse(const uint8_t* buf, std::size_t size, Packet& out) {
    if (size == 0 || buf[0] != kSync) return false;
    const uint8_t len = buf[1];
    std::memcpy(out.payload, buf + 2, len);
    out.len = len;
    return buf[2 + len] == checksum(out.payload, len);
}
