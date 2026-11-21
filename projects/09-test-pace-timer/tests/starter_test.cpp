// Starter test so the build has something to run. Replace with your own.
#include <gtest/gtest.h>

#include "pace_timer.h"
#include "timer_bank.h"

TEST(T1_Start, ValidIntervalArmsTheTimer) {
    PaceTimer timer;
    EXPECT_TRUE(timer.start(0, 1000));
    EXPECT_TRUE(timer.armed());
}
