// Golden master for telemetry_poll().
//
// Each test plays a scripted programmer session against the firmware and compares
// every register write with a trace recorded from the code as it was handed over.
// These tests say "nothing changed"; they do not say why the behaviour is right.
// Keep them passing after every refactoring step.
//
// Trace format: see fake_hw.h. Time starts at 0 and moves 10 ms per call of
// telemetry_poll(), so a line starting "5010" was written during call number 501.
#include <gtest/gtest.h>

#include <string>

#include "fake_hw.h"
#include "telemetry_task.h"

namespace {

class GoldenMaster : public ::testing::Test {
   protected:
    void SetUp() override {
        fake_hw::reset();
        telemetry_reset();
    }

    // Calls telemetry_poll() `count` times, 10 ms apart.
    static void poll(int count = 1) {
        for (int i = 0; i < count; ++i) {
            fake_hw::advance_ms(10);
            telemetry_poll();
        }
    }

    // The programmer sends one command, then the firmware runs once.
    static void send(uint8_t cmd, uint8_t a = 0, uint8_t b = 0) {
        fake_hw::push_rx({cmd, a, b, 0});
        poll();
    }

    static void open_session() { send(0x01, 0x2A, 0x5C); }
};

// The recorded traces below start on the line after R"( so they are easy to read.
std::string golden(const char* recorded) { return recorded + 1; }

const char kOpenReadWriteClose[] = R"(
10 EVENT_TIME 10
10 EVENT_CODE 010000
10 TX 06 01 00 00
20 TX 06 10 00 3C
30 TX 06 10 01 78
40 TX 06 10 02 23
50 TX 06 10 03 08
60 TX 06 10 04 19
70 TX 06 10 05 19
80 EVENT_TIME 80
80 EVENT_CODE 100046
80 TX 06 11 00 46
90 EVENT_TIME 90
90 EVENT_CODE 100182
90 TX 06 11 01 82
100 PULSE_CFG 0832
100 EVENT_TIME 100
100 EVENT_CODE 100232
100 TX 06 11 02 32
110 PULSE_CFG 0A32
110 EVENT_TIME 110
110 EVENT_CODE 10030A
110 TX 06 11 03 0A
120 EVENT_TIME 120
120 EVENT_CODE 10041E
120 TX 06 11 04 1E
130 EVENT_TIME 130
130 EVENT_CODE 10051E
130 TX 06 11 05 1E
140 TX 06 11 04 1E
150 TX 06 10 00 46
160 TX 06 10 01 82
170 TX 06 10 02 32
180 TX 06 10 03 0A
190 TX 06 10 04 1E
200 TX 06 10 05 1E
210 EVENT_TIME 210
210 EVENT_CODE 020000
210 TX 06 02 00 00
)";

TEST_F(GoldenMaster, OpenReadWriteClose) {
    open_session();
    for (uint8_t id = 0; id < 6; ++id) send(0x10, id);  // read the power-on values

    send(0x11, 0, 70);   // lower rate
    send(0x11, 1, 130);  // upper rate limit
    send(0x11, 2, 50);   // pulse amplitude
    send(0x11, 3, 10);   // pulse width
    send(0x11, 4, 30);   // sensitivity
    send(0x11, 5, 30);   // refractory period
    send(0x11, 4, 30);   // same value again
    for (uint8_t id = 0; id < 6; ++id) send(0x10, id);

    fake_hw::push_rx({0x02, 0xEE, 0xEE, 0xEE});  // close, with junk in the unused bytes
    poll();

    EXPECT_EQ(fake_hw::trace(), golden(kOpenReadWriteClose));
}

const char kRefusedCommands[] = R"(
10 TX 15 02 01 00
20 TX 15 10 01 00
30 TX 15 11 01 00
40 TX 15 20 01 00
50 TX 15 21 01 00
60 TX 15 7F 08 00
70 EVENT_TIME 70
70 EVENT_CODE 010000
70 TX 06 01 00 00
80 TX 15 01 02 00
90 TX 15 10 04 00
100 TX 15 10 04 00
110 TX 15 11 04 00
120 TX 15 11 05 00
130 EVENT_TIME 130
130 EVENT_CODE 100150
130 TX 06 11 01 50
140 TX 15 11 05 00
150 EVENT_TIME 150
150 EVENT_CODE 1001B4
150 TX 06 11 01 B4
160 TX 15 11 05 00
170 EVENT_TIME 170
170 EVENT_CODE 10001E
170 TX 06 11 00 1E
180 TX 15 11 05 00
190 EVENT_TIME 190
190 EVENT_CODE 1000AF
190 TX 06 11 00 AF
200 TX 15 11 05 00
210 PULSE_CFG 0805
210 EVENT_TIME 210
210 EVENT_CODE 100205
210 TX 06 11 02 05
220 TX 15 11 05 00
230 PULSE_CFG 084B
230 EVENT_TIME 230
230 EVENT_CODE 10024B
230 TX 06 11 02 4B
240 TX 15 11 05 00
250 PULSE_CFG 024B
250 EVENT_TIME 250
250 EVENT_CODE 100302
250 TX 06 11 03 02
260 TX 15 11 05 00
270 PULSE_CFG 1E4B
270 EVENT_TIME 270
270 EVENT_CODE 10031E
270 TX 06 11 03 1E
280 TX 15 11 05 00
290 EVENT_TIME 290
290 EVENT_CODE 100405
290 TX 06 11 04 05
300 TX 15 11 05 00
310 EVENT_TIME 310
310 EVENT_CODE 100464
310 TX 06 11 04 64
320 TX 15 11 05 00
330 EVENT_TIME 330
330 EVENT_CODE 10050F
330 TX 06 11 05 0F
340 TX 15 11 05 00
350 EVENT_TIME 350
350 EVENT_CODE 100532
350 TX 06 11 05 32
360 TX 15 11 07 00
370 EVENT_TIME 370
370 EVENT_CODE 1001B0
370 TX 06 11 01 B0
380 EVENT_TIME 380
380 EVENT_CODE 10003C
380 TX 06 11 00 3C
390 EVENT_TIME 390
390 EVENT_CODE 100178
390 TX 06 11 01 78
400 TX 15 11 07 00
410 EVENT_TIME 410
410 EVENT_CODE 100077
410 TX 06 11 00 77
420 TX 15 11 07 00
430 TX 06 11 01 78
440 TX 15 7F 08 00
450 TX 15 21 02 00
460 EVENT_TIME 460
460 EVENT_CODE 020000
460 TX 06 02 00 00
)";

TEST_F(GoldenMaster, RefusedCommands) {
    // Before a session is open.
    send(0x02);
    send(0x10, 0);
    send(0x11, 0, 70);
    send(0x20, 60, 5);
    send(0x21);
    send(0x7F);

    open_session();
    open_session();  // already open

    // Parameter ids that do not exist.
    send(0x10, 6);
    send(0x10, 255);
    send(0x11, 6, 50);

    // Each limit of each parameter: just outside, then exactly on it.
    send(0x11, 1, 79);
    send(0x11, 1, 80);
    send(0x11, 1, 181);
    send(0x11, 1, 180);
    send(0x11, 0, 29);
    send(0x11, 0, 30);
    send(0x11, 0, 176);
    send(0x11, 0, 175);
    send(0x11, 2, 4);
    send(0x11, 2, 5);
    send(0x11, 2, 76);
    send(0x11, 2, 75);
    send(0x11, 3, 1);
    send(0x11, 3, 2);
    send(0x11, 3, 31);
    send(0x11, 3, 30);
    send(0x11, 4, 4);
    send(0x11, 4, 5);
    send(0x11, 4, 101);
    send(0x11, 4, 100);
    send(0x11, 5, 14);
    send(0x11, 5, 15);
    send(0x11, 5, 51);
    send(0x11, 5, 50);

    // The two rates against each other. Lower rate is 175 and upper is 180 here.
    send(0x11, 1, 175);
    send(0x11, 1, 176);
    send(0x11, 0, 60);
    send(0x11, 1, 120);
    send(0x11, 0, 120);
    send(0x11, 0, 119);
    send(0x11, 1, 119);
    send(0x11, 1, 120);

    send(0x7F);  // unknown command inside a session
    send(0x21);  // leave test mode when it is not on
    send(0x02);

    EXPECT_EQ(fake_hw::trace(), golden(kRefusedCommands));
}

const char kAccessCodeLockout[] = R"(
10 EVENT_TIME 10
10 EVENT_CODE 040100
10 TX 15 01 03 00
20 EVENT_TIME 20
20 EVENT_CODE 040200
20 TX 15 01 03 00
30 EVENT_TIME 30
30 EVENT_CODE 010000
30 TX 06 01 00 00
40 EVENT_TIME 40
40 EVENT_CODE 020000
40 TX 06 02 00 00
50 EVENT_TIME 50
50 EVENT_CODE 040100
50 TX 15 01 03 00
60 EVENT_TIME 60
60 EVENT_CODE 040200
60 TX 15 01 03 00
70 EVENT_TIME 70
70 EVENT_CODE 040300
70 EVENT_TIME 70
70 EVENT_CODE 050000
70 TX 15 01 03 00
80 TX 15 01 09 00
90 TX 15 10 09 00
30060 TX 15 10 09 00
30070 EVENT_TIME 30070
30070 EVENT_CODE 060000
30070 EVENT_TIME 30070
30070 EVENT_CODE 040100
30070 TX 15 01 03 00
30080 EVENT_TIME 30080
30080 EVENT_CODE 040200
30080 TX 15 01 03 00
30090 EVENT_TIME 30090
30090 EVENT_CODE 010000
30090 TX 06 01 00 00
30100 EVENT_TIME 30100
30100 EVENT_CODE 020000
30100 TX 06 02 00 00
)";

TEST_F(GoldenMaster, AccessCodeLockout) {
    send(0x01, 0x2A, 0x5D);  // second byte wrong
    send(0x01, 0x2B, 0x5C);  // first byte wrong
    open_session();          // right code after two wrong ones
    send(0x02);

    send(0x01, 0x00, 0x00);
    send(0x01, 0x00, 0x00);
    send(0x01, 0x5C, 0x2A);  // third wrong code in a row, during call 7
    open_session();          // the right code no longer helps
    send(0x10, 0);

    // The lockout keeps running while the programmer is out of range.
    poll(1000);
    fake_hw::set_link(false);
    poll(1000);
    fake_hw::set_link(true);
    poll(996);

    send(0x10, 0);           // call 3006: still locked out
    send(0x01, 0x01, 0x01);  // call 3007: lockout ends, then the command is handled.
                             // The count of wrong codes has started again.
    send(0x01, 0x01, 0x01);
    open_session();
    send(0x02);

    EXPECT_EQ(fake_hw::trace(), golden(kAccessCodeLockout));
}

const char kTestMode[] = R"(
10 EVENT_TIME 10
10 EVENT_CODE 010000
10 TX 06 01 00 00
20 TX 15 20 05 00
30 TX 15 20 05 00
40 TX 15 20 05 00
50 TX 15 20 05 00
60 PACE_OVERRIDE 30
60 EVENT_TIME 60
60 EVENT_CODE 201E01
60 TX 06 20 1E 01
70 TX 15 20 06 00
80 TX 15 11 06 00
90 TX 06 10 00 3C
1060 PACE_OVERRIDE 0
1060 EVENT_TIME 1060
1060 EVENT_CODE 210100
1060 EVENT_TIME 1060
1060 EVENT_CODE 100046
1060 TX 06 11 00 46
1070 PACE_OVERRIDE 180
1070 EVENT_TIME 1070
1070 EVENT_CODE 20B43C
1070 TX 06 20 B4 3C
1080 PACE_OVERRIDE 0
1080 EVENT_TIME 1080
1080 EVENT_CODE 210000
1080 TX 06 21 00 00
1090 TX 15 21 02 00
1100 PACE_OVERRIDE 100
1100 EVENT_TIME 1100
1100 EVENT_CODE 20640A
1100 TX 06 20 64 0A
1110 PACE_OVERRIDE 0
1110 EVENT_TIME 1110
1110 EVENT_CODE 210200
1110 EVENT_TIME 1110
1110 EVENT_CODE 020000
1110 TX 06 02 00 00
)";

TEST_F(GoldenMaster, TestMode) {
    open_session();

    send(0x20, 29, 5);
    send(0x20, 181, 5);
    send(0x20, 90, 0);
    send(0x20, 90, 61);

    send(0x20, 30, 1);  // call 6: slowest rate, shortest time
    send(0x20, 90, 5);  // already in test mode
    send(0x11, 0, 70);  // parameters cannot change during a test
    send(0x10, 0);      // reading still works
    poll(96);           // up to call 105: still in test mode
    send(0x11, 0, 70);  // call 106: test mode ends, then the write is handled

    send(0x20, 180, 60);  // fastest rate, longest time
    send(0x21);           // leave by command
    send(0x21);           // not in test mode any more

    send(0x20, 100, 10);
    send(0x02);  // closing the session ends the test

    EXPECT_EQ(fake_hw::trace(), golden(kTestMode));
}

const char kTimeoutsAndLostLink[] = R"(
10 EVENT_TIME 10
10 EVENT_CODE 010000
10 TX 06 01 00 00
5000 TX 06 10 00 3C
5010 PACE_OVERRIDE 60
5010 EVENT_TIME 5010
5010 EVENT_CODE 203C3C
5010 TX 06 20 3C 3C
10010 PACE_OVERRIDE 0
10010 EVENT_TIME 10010
10010 EVENT_CODE 210200
10010 EVENT_TIME 10010
10010 EVENT_CODE 030000
10020 TX 15 10 01 00
10030 EVENT_TIME 10030
10030 EVENT_CODE 010000
10030 TX 06 01 00 00
10440 TX 06 10 00 3C
10650 EVENT_TIME 10650
10650 EVENT_CODE 300200
10660 TX 06 10 01 78
10670 TX 06 10 00 3C 06 10 01 78
10680 TX 06 10 02 23
10700 TX 06 10 03 08 06 10 04 19
10710 TX 06 10 05 19
10720 PACE_OVERRIDE 70
10720 EVENT_TIME 10720
10720 EVENT_CODE 20461E
10720 TX 06 20 46 1E
10740 PACE_OVERRIDE 0
10740 EVENT_TIME 10740
10740 EVENT_CODE 210200
10740 EVENT_TIME 10740
10740 EVENT_CODE 070000
11290 EVENT_TIME 11290
11290 EVENT_CODE 300200
11300 TX 15 10 01 00
11340 EVENT_TIME 11340
11340 EVENT_CODE 010000
11340 TX 06 01 00 00
11350 EVENT_TIME 11350
11350 EVENT_CODE 070000
11360 TX 15 10 01 00
11370 EVENT_TIME 11370
11370 EVENT_CODE 010000
11370 TX 06 01 00 00
16370 EVENT_TIME 16370
16370 EVENT_CODE 030000
16380 TX 15 02 01 00
)";

TEST_F(GoldenMaster, TimeoutsAndLostLink) {
    // Session timeout.
    open_session();      // call 1
    poll(498);           // calls 2 to 499 with no command
    send(0x10, 0);       // call 500: a command just in time keeps the session open
    send(0x20, 60, 60);  // call 501
    poll(499);           // calls 502 to 1000
    poll();              // call 1001: 500 calls without a command
    send(0x10, 0);

    // A command that arrives in pieces.
    open_session();
    fake_hw::push_rx({0x10, 0x00});
    poll();
    poll(19);
    fake_hw::push_rx({0x00});  // the next byte, just in time
    poll();
    poll(19);
    fake_hw::push_rx({0x00});
    poll();

    // A command that never finishes.
    fake_hw::push_rx({0x11, 0x00});
    poll();
    poll(19);
    poll();
    send(0x10, 1);

    // Three commands waiting at once.
    fake_hw::push_rx({0x10, 0, 0, 0, 0x10, 1, 0, 0, 0x10, 2, 0, 0});
    poll();
    poll();

    // Nine bytes waiting when three bytes of a command are already in.
    fake_hw::push_rx({0x10, 3, 0});
    poll();
    fake_hw::push_rx({0, 0x10, 4, 0, 0, 0x10, 5, 0, 0});
    poll();
    poll();

    // The programmer goes out of range during a test, with half a command received.
    send(0x20, 70, 30);
    fake_hw::push_rx({0x10, 0x03});
    poll();
    fake_hw::set_link(false);
    poll(30);
    fake_hw::push_rx({0x00, 0x00});  // sent while out of range: waits in the radio
    poll(5);
    fake_hw::set_link(true);
    poll();
    poll(19);
    poll();
    send(0x10, 0);

    // Out of range with no session open.
    fake_hw::set_link(false);
    poll(3);
    fake_hw::set_link(true);

    // Out of range with a session open and no test running.
    open_session();
    fake_hw::set_link(false);
    poll();
    fake_hw::set_link(true);
    send(0x10, 0);

    // A session left alone, no test running.
    open_session();
    poll(499);
    poll();
    send(0x02);

    EXPECT_EQ(fake_hw::trace(), golden(kTimeoutsAndLostLink));
}

}  // namespace
