// Starter test so the build has something to run. Replace with your own.
#include <gtest/gtest.h>

#include "packet.h"

TEST(P3_Encode, ReturnsFrameLength) {
    Packet p{};
    p.len = 2;
    p.payload[0] = 0x01;
    p.payload[1] = 0x02;

    uint8_t buf[8] = {};
    EXPECT_EQ(encode(p, buf, sizeof(buf)), 5u);
    EXPECT_EQ(buf[0], 0xA5);
}
