// Starter test so the build has something to run. Replace with your own.
#include <gtest/gtest.h>

#include "battery_gauge.h"

TEST(R3_Classify, WellAboveThresholdIsNormal) {
    EXPECT_EQ(classify(3000), BatteryStatus::Normal);
}
