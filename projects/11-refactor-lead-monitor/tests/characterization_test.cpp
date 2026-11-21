// Characterization tests: they pin what the legacy lead monitor does today.
// Keep every one of them passing after each refactoring step.
#include <gtest/gtest.h>

#include <vector>

#include "lead_monitor.h"

namespace {

struct AlertEvent {
    int lead;
    int flags;
    unsigned short ohms;
    int current_was_on;
    lead_info_t info_seen;
};

struct Recorder {
    std::vector<AlertEvent> events;
};

void on_alert(void* ctx, int lead, int flags, unsigned short ohms) {
    AlertEvent e{};
    e.lead = lead;
    e.flags = flags;
    e.ohms = ohms;
    e.current_was_on = lm_current_is_on();
    lm_get_info(lead, &e.info_seen);
    static_cast<Recorder*>(ctx)->events.push_back(e);
}

// One clean measurement of the given resistance. Ohms must be a multiple of 4.
int measure_ohms(int lead, unsigned short ohms) {
    const unsigned short raw = static_cast<unsigned short>(ohms / 4);
    return lm_measure(lead, raw, raw, raw);
}

lead_info_t info(int lead) {
    lead_info_t i{};
    EXPECT_EQ(lm_get_info(lead, &i), LM_OK);
    return i;
}

std::vector<unsigned short> history(int lead) {
    unsigned short buf[16] = {};
    int n = -1;
    EXPECT_EQ(lm_get_history(lead, buf, 16, &n), LM_OK);
    return std::vector<unsigned short>(buf, buf + n);
}

class LeadMonitor : public ::testing::Test {
   protected:
    void SetUp() override {
        lm_shutdown();
        ASSERT_EQ(lm_init(on_alert, &rec), LM_OK);
    }
    void TearDown() override { lm_shutdown(); }

    Recorder rec;
};

}  // namespace

// ---- init and shutdown ---------------------------------------------------------------

TEST_F(LeadMonitor, SecondInitFails) {
    EXPECT_EQ(lm_init(on_alert, &rec), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_ALREADY_INIT);
    EXPECT_EQ(measure_ohms(LEAD_ATRIAL, 1000), LM_OK);  // still usable
}

TEST_F(LeadMonitor, EveryCallFailsAfterShutdown) {
    lm_clear_alert(5);  // leave an error behind
    lm_shutdown();
    EXPECT_EQ(lm_last_error(), LM_E_NONE);

    lead_info_t i{};
    unsigned short buf[4] = {};
    unsigned short raws[3] = {250, 250, 250};
    int n = 0;

    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 250, 250, 250), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_NOT_INIT);
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, 3), LM_ERR);
    EXPECT_EQ(lm_get_info(LEAD_ATRIAL, &i), LM_ERR);
    EXPECT_EQ(lm_get_history(LEAD_ATRIAL, buf, 4, &n), LM_ERR);
    EXPECT_EQ(lm_clear_alert(LEAD_ATRIAL), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_NOT_INIT);
    EXPECT_EQ(lm_pulse_count(), 0u);
    EXPECT_EQ(lm_current_is_on(), 0);

    lm_shutdown();  // a second shutdown is harmless
    EXPECT_EQ(lm_last_error(), LM_E_NOT_INIT);
    EXPECT_EQ(lm_init(nullptr, nullptr), LM_OK);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);
}

TEST_F(LeadMonitor, InitAfterShutdownStartsFromScratch) {
    for (int lead : {LEAD_ATRIAL, LEAD_VENTRICULAR}) {
        EXPECT_EQ(measure_ohms(lead, 1000), LM_OK);
        EXPECT_EQ(measure_ohms(lead, 100), LM_OK);       // alert
        EXPECT_EQ(lm_measure(lead, 0, 0, 500), LM_ERR);  // reject
    }
    lm_shutdown();
    ASSERT_EQ(lm_init(on_alert, &rec), LM_OK);

    for (int lead : {LEAD_ATRIAL, LEAD_VENTRICULAR}) {
        const lead_info_t i = info(lead);
        EXPECT_EQ(i.last_ohms, 0);
        EXPECT_EQ(i.min_ohms, 0);
        EXPECT_EQ(i.max_ohms, 0);
        EXPECT_EQ(i.status, LEAD_UNKNOWN);
        EXPECT_EQ(i.alert, 0);
        EXPECT_EQ(i.count, 0u);
        EXPECT_EQ(i.rejects, 0u);
        EXPECT_TRUE(history(lead).empty());
    }
    EXPECT_EQ(lm_pulse_count(), 0u);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);
}

// ---- one measurement -----------------------------------------------------------------

TEST_F(LeadMonitor, ResultIsFourTimesTheMiddleReadingWhateverTheOrder) {
    const unsigned short orders[6][3] = {{240, 250, 260}, {240, 260, 250}, {250, 240, 260},
                                         {250, 260, 240}, {260, 240, 250}, {260, 250, 240}};
    for (const auto& o : orders) {
        EXPECT_EQ(lm_measure(LEAD_ATRIAL, o[0], o[1], o[2]), LM_OK);
        EXPECT_EQ(info(LEAD_ATRIAL).last_ohms, 1000);
    }
}

TEST_F(LeadMonitor, MiddleReadingIsUsedNotTheMean) {
    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 100, 100, 130), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).last_ohms, 400);
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 130, 100, 130), LM_OK);
    EXPECT_EQ(info(LEAD_VENTRICULAR).last_ohms, 520);
}

TEST_F(LeadMonitor, SmallestAndLargestReadingsAreAccepted) {
    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 0, 0, 0), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).last_ohms, 0);
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 1023, 1023, 1023), LM_OK);
    EXPECT_EQ(info(LEAD_VENTRICULAR).last_ohms, 4092);
}

TEST_F(LeadMonitor, ReadingAbove1023IsRejectedInAnyPosition) {
    EXPECT_EQ(measure_ohms(LEAD_ATRIAL, 1000), LM_OK);

    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 1024, 250, 250), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_RAW);
    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 250, 1024, 250), LM_ERR);
    EXPECT_EQ(lm_measure(LEAD_ATRIAL, 250, 250, 65535), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_RAW);
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 10, 2000, 10), LM_ERR);  // checked before noise
    EXPECT_EQ(lm_last_error(), LM_E_BAD_RAW);

    const lead_info_t i = info(LEAD_ATRIAL);
    EXPECT_EQ(i.rejects, 3u);
    EXPECT_EQ(i.count, 1u);
    EXPECT_EQ(i.last_ohms, 1000);
    EXPECT_EQ(history(LEAD_ATRIAL).size(), 1u);
    EXPECT_TRUE(rec.events.empty());
}

TEST_F(LeadMonitor, ReadingsMoreThan40ApartAreRejectedAsNoisy) {
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 250, 290, 270), LM_OK);  // exactly 40 apart
    EXPECT_EQ(info(LEAD_VENTRICULAR).last_ohms, 1080);

    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 250, 291, 270), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_NOISY);
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 291, 270, 250), LM_ERR);
    EXPECT_EQ(lm_measure(LEAD_VENTRICULAR, 270, 250, 291), LM_ERR);

    const lead_info_t i = info(LEAD_VENTRICULAR);
    EXPECT_EQ(i.rejects, 3u);
    EXPECT_EQ(i.count, 1u);
    EXPECT_EQ(i.last_ohms, 1080);
    EXPECT_EQ(info(LEAD_ATRIAL).rejects, 0u);
}

TEST_F(LeadMonitor, UnknownLeadIsRejectedEverywhere) {
    lead_info_t i{};
    i.count = 77;
    unsigned short buf[4] = {};
    unsigned short raws[3] = {250, 250, 250};
    int n = 55;

    for (int lead : {-1, 2}) {
        EXPECT_EQ(lm_measure(lead, 250, 250, 250), LM_ERR);
        EXPECT_EQ(lm_last_error(), LM_E_BAD_LEAD);
        EXPECT_EQ(lm_measure_many(lead, raws, 3), LM_ERR);
        EXPECT_EQ(lm_last_error(), LM_E_BAD_LEAD);
        EXPECT_EQ(lm_get_info(lead, &i), LM_ERR);
        EXPECT_EQ(lm_last_error(), LM_E_BAD_LEAD);
        EXPECT_EQ(lm_get_history(lead, buf, 4, &n), LM_ERR);
        EXPECT_EQ(lm_last_error(), LM_E_BAD_LEAD);
        EXPECT_EQ(lm_clear_alert(lead), LM_ERR);
        EXPECT_EQ(lm_last_error(), LM_E_BAD_LEAD);
    }
    EXPECT_EQ(i.count, 77u);  // outputs untouched
    EXPECT_EQ(n, 55);
    EXPECT_EQ(lm_pulse_count(), 0u);
}

// ---- measurement current -------------------------------------------------------------

TEST_F(LeadMonitor, CurrentIsOffAfterEveryKindOfMeasurement) {
    EXPECT_EQ(lm_current_is_on(), 0);
    measure_ohms(LEAD_ATRIAL, 1000);
    EXPECT_EQ(lm_current_is_on(), 0);
    lm_measure(LEAD_ATRIAL, 1024, 250, 250);
    EXPECT_EQ(lm_current_is_on(), 0);
    lm_measure(LEAD_VENTRICULAR, 0, 300, 600);
    EXPECT_EQ(lm_current_is_on(), 0);
    measure_ohms(LEAD_VENTRICULAR, 100);  // raises an alert
    EXPECT_EQ(lm_current_is_on(), 0);
    lm_measure(7, 250, 250, 250);
    EXPECT_EQ(lm_current_is_on(), 0);
}

TEST_F(LeadMonitor, CurrentPulsesOncePerAttemptedMeasurement) {
    measure_ohms(LEAD_ATRIAL, 1000);
    EXPECT_EQ(lm_pulse_count(), 1u);
    lm_measure(LEAD_ATRIAL, 1024, 250, 250);  // bad reading: current was used
    EXPECT_EQ(lm_pulse_count(), 2u);
    lm_measure(LEAD_VENTRICULAR, 0, 300, 600);  // noisy: current was used
    EXPECT_EQ(lm_pulse_count(), 3u);
    lm_measure(7, 250, 250, 250);  // bad lead: current never switched on
    EXPECT_EQ(lm_pulse_count(), 3u);
}

// ---- status limits -------------------------------------------------------------------

TEST_F(LeadMonitor, AtrialStatusLimitsAre200And2000) {
    measure_ohms(LEAD_ATRIAL, 196);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_LOW);
    measure_ohms(LEAD_ATRIAL, 200);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_OK);
    measure_ohms(LEAD_ATRIAL, 2000);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_OK);
    measure_ohms(LEAD_ATRIAL, 2004);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_HIGH);
}

TEST_F(LeadMonitor, VentricularStatusLimitsAre300And3000) {
    measure_ohms(LEAD_VENTRICULAR, 296);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_LOW);
    measure_ohms(LEAD_VENTRICULAR, 300);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_OK);
    measure_ohms(LEAD_VENTRICULAR, 3000);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_OK);
    measure_ohms(LEAD_VENTRICULAR, 3004);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_HIGH);
}

// ---- alerts --------------------------------------------------------------------------

TEST_F(LeadMonitor, LowResultLatchesAlertAndCallsBackOnce) {
    measure_ohms(LEAD_ATRIAL, 196);
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].lead, LEAD_ATRIAL);
    EXPECT_EQ(rec.events[0].flags, ALERT_LOW);
    EXPECT_EQ(rec.events[0].ohms, 196);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, ALERT_LOW);

    measure_ohms(LEAD_ATRIAL, 196);
    EXPECT_EQ(rec.events.size(), 1u);
}

TEST_F(LeadMonitor, HighResultLatchesAlert) {
    measure_ohms(LEAD_VENTRICULAR, 3004);
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].lead, LEAD_VENTRICULAR);
    EXPECT_EQ(rec.events[0].flags, ALERT_HIGH);
    EXPECT_EQ(rec.events[0].ohms, 3004);
    EXPECT_EQ(info(LEAD_VENTRICULAR).alert, ALERT_HIGH);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, 0);
}

TEST_F(LeadMonitor, ChangeOfMoreThan500OhmsLatchesJumpAlert) {
    measure_ohms(LEAD_ATRIAL, 1000);
    measure_ohms(LEAD_ATRIAL, 1500);  // exactly 500: no alert
    measure_ohms(LEAD_ATRIAL, 1000);
    EXPECT_TRUE(rec.events.empty());
    EXPECT_EQ(info(LEAD_ATRIAL).alert, 0);

    measure_ohms(LEAD_ATRIAL, 1504);
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].flags, ALERT_JUMP);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, ALERT_JUMP);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_OK);
}

TEST_F(LeadMonitor, DownwardJumpOnVentricularLeadAlsoAlerts) {
    measure_ohms(LEAD_VENTRICULAR, 1504);
    measure_ohms(LEAD_VENTRICULAR, 1004);
    EXPECT_TRUE(rec.events.empty());
    measure_ohms(LEAD_VENTRICULAR, 500);
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].lead, LEAD_VENTRICULAR);
    EXPECT_EQ(rec.events[0].flags, ALERT_JUMP);
}

TEST_F(LeadMonitor, FirstMeasurementIsNeverAJump) {
    measure_ohms(LEAD_ATRIAL, 1800);
    measure_ohms(LEAD_VENTRICULAR, 2800);
    EXPECT_TRUE(rec.events.empty());
}

TEST_F(LeadMonitor, JumpIsMeasuredAgainstTheLastAcceptedResult) {
    measure_ohms(LEAD_ATRIAL, 1000);
    lm_measure(LEAD_ATRIAL, 0, 500, 1000);  // rejected, must not move the baseline
    measure_ohms(LEAD_ATRIAL, 1400);
    EXPECT_TRUE(rec.events.empty());
}

TEST_F(LeadMonitor, JumpAndLimitFlagsCombine) {
    measure_ohms(LEAD_ATRIAL, 1000);
    measure_ohms(LEAD_ATRIAL, 2400);
    ASSERT_EQ(rec.events.size(), 1u);
    EXPECT_EQ(rec.events[0].flags, ALERT_HIGH | ALERT_JUMP);

    measure_ohms(LEAD_VENTRICULAR, 1000);
    measure_ohms(LEAD_VENTRICULAR, 240);
    ASSERT_EQ(rec.events.size(), 2u);
    EXPECT_EQ(rec.events[1].flags, ALERT_LOW | ALERT_JUMP);
}

TEST_F(LeadMonitor, SameResistanceCanBeFineOnOneLeadAndNotTheOther) {
    measure_ohms(LEAD_ATRIAL, 2400);
    measure_ohms(LEAD_VENTRICULAR, 2400);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_HIGH);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_OK);

    measure_ohms(LEAD_ATRIAL, 2000);
    measure_ohms(LEAD_VENTRICULAR, 2000);
    measure_ohms(LEAD_ATRIAL, 1520);
    measure_ohms(LEAD_VENTRICULAR, 1520);
    measure_ohms(LEAD_ATRIAL, 1040);
    measure_ohms(LEAD_VENTRICULAR, 1040);
    measure_ohms(LEAD_ATRIAL, 560);
    measure_ohms(LEAD_VENTRICULAR, 560);
    measure_ohms(LEAD_ATRIAL, 240);
    measure_ohms(LEAD_VENTRICULAR, 240);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_OK);
    EXPECT_EQ(info(LEAD_VENTRICULAR).status, LEAD_LOW);
}

TEST_F(LeadMonitor, LatchedAlertKeepsItsFirstFlagsAndStaysQuiet) {
    for (int lead : {LEAD_ATRIAL, LEAD_VENTRICULAR}) {
        rec.events.clear();
        measure_ohms(lead, 100);   // ALERT_LOW
        measure_ohms(lead, 400);   // back in range
        measure_ohms(lead, 3400);  // high and a jump, but already latched
        EXPECT_EQ(rec.events.size(), 1u);
        EXPECT_EQ(info(lead).alert, ALERT_LOW);
        EXPECT_EQ(info(lead).status, LEAD_HIGH);
    }
}

TEST_F(LeadMonitor, ClearAlertRearmsTheCallback) {
    measure_ohms(LEAD_ATRIAL, 100);
    EXPECT_EQ(lm_clear_alert(LEAD_ATRIAL), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, 0);
    EXPECT_EQ(info(LEAD_ATRIAL).status, LEAD_LOW);  // status is not cleared
    EXPECT_EQ(history(LEAD_ATRIAL).size(), 1u);

    measure_ohms(LEAD_ATRIAL, 100);
    ASSERT_EQ(rec.events.size(), 2u);
    EXPECT_EQ(rec.events[1].flags, ALERT_LOW);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, ALERT_LOW);
}

TEST_F(LeadMonitor, ClearAlertOnlyClearsTheNamedLead) {
    measure_ohms(LEAD_ATRIAL, 100);
    measure_ohms(LEAD_VENTRICULAR, 100);
    EXPECT_EQ(lm_clear_alert(LEAD_VENTRICULAR), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, ALERT_LOW);
    EXPECT_EQ(info(LEAD_VENTRICULAR).alert, 0);
    EXPECT_EQ(lm_clear_alert(LEAD_VENTRICULAR), LM_OK);  // nothing to clear is fine
    EXPECT_EQ(lm_clear_alert(LEAD_ATRIAL), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, 0);
}

TEST_F(LeadMonitor, CallbackRunsWithCurrentOffAndStateAlreadyUpdated) {
    measure_ohms(LEAD_VENTRICULAR, 1000);
    measure_ohms(LEAD_VENTRICULAR, 200);
    ASSERT_EQ(rec.events.size(), 1u);
    const AlertEvent& e = rec.events[0];
    EXPECT_EQ(e.current_was_on, 0);
    EXPECT_EQ(e.info_seen.last_ohms, 200);
    EXPECT_EQ(e.info_seen.alert, ALERT_LOW | ALERT_JUMP);
    EXPECT_EQ(e.info_seen.status, LEAD_LOW);
    EXPECT_EQ(e.info_seen.count, 2u);
    EXPECT_EQ(e.info_seen.min_ohms, 200);
}

TEST_F(LeadMonitor, CallbackReceivesTheContextGivenToInit) {
    Recorder other;
    lm_shutdown();
    ASSERT_EQ(lm_init(on_alert, &other), LM_OK);
    measure_ohms(LEAD_ATRIAL, 100);
    EXPECT_TRUE(rec.events.empty());
    EXPECT_EQ(other.events.size(), 1u);
}

TEST_F(LeadMonitor, NoCallbackIsAllowed) {
    lm_shutdown();
    ASSERT_EQ(lm_init(nullptr, nullptr), LM_OK);
    EXPECT_EQ(measure_ohms(LEAD_ATRIAL, 100), LM_OK);
    EXPECT_EQ(info(LEAD_ATRIAL).alert, ALERT_LOW);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);
}

// ---- statistics and history ----------------------------------------------------------

TEST_F(LeadMonitor, MinMaxLastAndCountTrackAcceptedMeasurementsPerLead) {
    measure_ohms(LEAD_ATRIAL, 800);
    EXPECT_EQ(info(LEAD_ATRIAL).min_ohms, 800);
    EXPECT_EQ(info(LEAD_ATRIAL).max_ohms, 800);
    measure_ohms(LEAD_ATRIAL, 600);
    measure_ohms(LEAD_ATRIAL, 1000);
    measure_ohms(LEAD_ATRIAL, 900);
    measure_ohms(LEAD_VENTRICULAR, 1200);
    EXPECT_EQ(info(LEAD_VENTRICULAR).min_ohms, 1200);
    EXPECT_EQ(info(LEAD_VENTRICULAR).max_ohms, 1200);
    measure_ohms(LEAD_VENTRICULAR, 1500);
    measure_ohms(LEAD_VENTRICULAR, 1100);

    const lead_info_t a = info(LEAD_ATRIAL);
    EXPECT_EQ(a.min_ohms, 600);
    EXPECT_EQ(a.max_ohms, 1000);
    EXPECT_EQ(a.last_ohms, 900);
    EXPECT_EQ(a.count, 4u);

    const lead_info_t v = info(LEAD_VENTRICULAR);
    EXPECT_EQ(v.min_ohms, 1100);
    EXPECT_EQ(v.max_ohms, 1500);
    EXPECT_EQ(v.last_ohms, 1100);
    EXPECT_EQ(v.count, 3u);
}

TEST_F(LeadMonitor, HistoryIsNewestFirstAndKeepsSix) {
    const std::vector<unsigned short> expected = {1028, 1024, 1020, 1016, 1012, 1008};
    for (int lead : {LEAD_ATRIAL, LEAD_VENTRICULAR}) {
        EXPECT_TRUE(history(lead).empty());
        for (unsigned short n = 1; n <= 8; ++n) {
            measure_ohms(lead, static_cast<unsigned short>(996 + 4 * n));  // 1000 ... 1028
            EXPECT_EQ(history(lead).size(), n < 6u ? n : 6u);
        }
        EXPECT_EQ(history(lead), expected);
        EXPECT_EQ(info(lead).count, 8u);
    }
}

TEST_F(LeadMonitor, HistoryCopiesNoMoreThanTheBufferHolds) {
    for (int lead : {LEAD_ATRIAL, LEAD_VENTRICULAR}) {
        measure_ohms(lead, 1000);
        measure_ohms(lead, 1100);
        measure_ohms(lead, 1200);

        unsigned short buf[4] = {7, 7, 7, 7};
        int n = -1;
        EXPECT_EQ(lm_get_history(lead, buf, 2, &n), LM_OK);
        EXPECT_EQ(n, 2);
        EXPECT_EQ(buf[0], 1200);
        EXPECT_EQ(buf[1], 1100);
        EXPECT_EQ(buf[2], 7);

        EXPECT_EQ(lm_get_history(lead, buf, 0, &n), LM_OK);
        EXPECT_EQ(n, 0);
        EXPECT_EQ(buf[0], 1200);
    }
}

TEST_F(LeadMonitor, BadPointersAndLengthsAreRejected) {
    unsigned short buf[4] = {};
    int n = 55;
    EXPECT_EQ(lm_get_history(LEAD_ATRIAL, nullptr, 4, &n), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(lm_get_info(LEAD_ATRIAL, nullptr), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(lm_get_history(LEAD_ATRIAL, buf, 4, nullptr), LM_ERR);
    EXPECT_EQ(lm_get_history(LEAD_ATRIAL, buf, -1, &n), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(n, 55);

    // A bad pointer is reported even when the lead is wrong too.
    EXPECT_EQ(lm_get_info(9, nullptr), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(lm_get_history(9, nullptr, 4, &n), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
}

TEST_F(LeadMonitor, LastErrorIsClearedByTheNextSuccessfulCall) {
    lm_measure(LEAD_ATRIAL, 1024, 0, 0);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_RAW);
    measure_ohms(LEAD_ATRIAL, 1000);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);

    lm_clear_alert(5);
    lead_info_t i{};
    lm_get_info(LEAD_ATRIAL, &i);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);

    unsigned short buf[2] = {};
    int n = 0;
    lm_clear_alert(5);
    lm_get_history(LEAD_ATRIAL, buf, 2, &n);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);

    lm_measure(4, 0, 0, 0);
    lm_clear_alert(LEAD_ATRIAL);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);
}

// ---- several measurements in one call ------------------------------------------------

TEST_F(LeadMonitor, MeasureManyRunsEachTripleAndCountsTheAcceptedOnes) {
    unsigned short raws[9] = {250, 250, 250, /* noisy */ 0, 300, 600, 260, 260, 260};
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, 9), 2);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);  // the last triple was fine

    const lead_info_t i = info(LEAD_ATRIAL);
    EXPECT_EQ(i.count, 2u);
    EXPECT_EQ(i.rejects, 1u);
    EXPECT_EQ(i.last_ohms, 1040);
    EXPECT_EQ(lm_pulse_count(), 3u);
    const std::vector<unsigned short> expected = {1040, 1000};
    EXPECT_EQ(history(LEAD_ATRIAL), expected);
    EXPECT_EQ(info(LEAD_VENTRICULAR).count, 0u);

    unsigned short bad_last[6] = {250, 250, 250, 250, 250, 1024};
    EXPECT_EQ(lm_measure_many(LEAD_VENTRICULAR, bad_last, 6), 1);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_RAW);  // the error of the last triple is left behind
    EXPECT_EQ(info(LEAD_VENTRICULAR).count, 1u);
}

TEST_F(LeadMonitor, MeasureManyRejectsBadArgumentsBeforeMeasuring) {
    unsigned short raws[6] = {250, 250, 250, 250, 250, 250};
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, nullptr, 3), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, -3), LM_ERR);
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, 4), LM_ERR);
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, 5), LM_ERR);
    EXPECT_EQ(lm_last_error(), LM_E_BAD_ARG);
    EXPECT_EQ(lm_pulse_count(), 0u);
    EXPECT_EQ(info(LEAD_ATRIAL).count, 0u);

    lm_clear_alert(5);  // leave an error behind
    EXPECT_EQ(lm_measure_many(LEAD_ATRIAL, raws, 0), 0);
    EXPECT_EQ(lm_last_error(), LM_E_NONE);
}
