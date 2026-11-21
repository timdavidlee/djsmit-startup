# Project 9: Requirement-based testing of a pace timer

Produce a traceability matrix, a test suite and defect reports for a small timing module with four seeded defects. Regulated software must show that every requirement has a test and every test maps to a requirement; this project practices that discipline at small scale.

**Module under test:** [`pace_timer.h`](pace_timer.h). Put your tests in `tests/`.

*Medical context:* the timer decides when the next pacing pulse is due. Interval and rate are two views of one number: rate in beats per minute = 60,000 / interval in ms, so 300 ms is 200 beats per minute and 2000 ms is 30. Devices refuse intervals outside safe limits, because pacing far too fast or too slow harms the patient.

**Specification**

| ID | Requirement |
| --- | --- |
| T1 | `start(now, interval)` arms the timer and returns true when `interval` is from 300 to 2000 ms inclusive. |
| T2 | For any other interval, `start` returns false and leaves the timer exactly as it was. |
| T3 | `expired(now)` is true only when the timer is armed and at least `interval` ms have passed since `start`. |
| T4 | All timing is correct when the 32-bit millisecond counter wraps from `0xFFFFFFFF` to 0. |
| T5 | `stop()` disarms the timer. |
| T6 | `remaining(now)` returns the milliseconds until expiry, and 0 once expired or when not armed. |

**Tasks**

1. **Traceability matrix.** Make a table with one row per requirement and the names of the tests that cover it. Every requirement needs at least one normal case and one boundary or failure case.
2. **Write the tests**, named by requirement ID. No sleeping and no real clock: pass `now` values directly.
3. For T4, choose start times just below `0xFFFFFFFF` so the deadline falls after the wrap. Test both just before and just after the deadline.
4. **Defect reports.** For each failure: requirement ID, steps, expected, actual, and a severity with a one-line patient-impact argument.
5. **Fix and regress.** Fix one defect at a time and re-run the whole suite after each fix.
6. **Mutation check.** In the fixed code, change one operator at a time (`<` to `<=`, `>=` to `>`, `+` to `-`) and confirm at least one test fails each time. A mutation that survives marks a gap in the tests.

**Check yourself**

- Sanitizers report nothing for this module. Why not, when it has four defects?
- One defect makes a pacing pulse fire far too early, but only in a narrow time window about every 49.7 days. How would you argue its severity?
- T2 says "exactly as it was". How does a test prove something did not change?
- What is the difference between verification (built it right) and validation (built the right thing)?
- What does the mutation check measure that line coverage does not?
