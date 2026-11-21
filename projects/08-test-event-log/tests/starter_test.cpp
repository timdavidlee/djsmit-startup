// Starter test so the build has something to run. Replace with your own.
#include <gtest/gtest.h>

#include "event_log.h"

TEST(E3_Count, OneEventLoggedCountsOne) {
    EventLog log;
    log.log(1000, 0x0001, "boot");
    EXPECT_EQ(log.count(), 1);
}
