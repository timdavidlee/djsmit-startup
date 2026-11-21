# Project 9: Requirement-based testing of a pace timer

Produce a traceability matrix, a test suite and defect reports for a timing module with ten seeded defects. Regulated software must show that every requirement has a test and every test maps to a requirement; this project practices that discipline at small scale.

**Module under test:** [`src/pace_timer.h`](src/pace_timer.h) and [`src/timer_bank.h`](src/timer_bank.h), with their `.cpp` files. Put your tests in `tests/`; a starter test is already there.

*Medical context:* the timing service decides when the next pacing pulse is due. Interval and rate are two views of one number: rate in beats per minute = 60,000 / interval in ms, so 300 ms is 200 beats per minute and 2000 ms is 30. Devices refuse intervals outside safe limits, because pacing far too fast or too slow harms the patient. Four more terms appear below:

- The **escape interval** is how long the device waits for the heart's own beat before it paces.
- **Rate hysteresis** lengthens that wait after a beat the heart started itself, to give the heart's own rhythm a chance to continue.
- The **refractory period** is a short window after a beat when sensed signals are ignored, because they are echoes of that beat and not a new one.
- The **upper rate limit** is a safety stop: whatever else goes wrong, pulses are never delivered closer together than a fixed minimum.

**Specification**

All times are in milliseconds, read from a free-running 32-bit counter and passed in as `now`.

| ID | Requirement |
| --- | --- |
| T1 | `PaceTimer::start(now, interval)` arms the timer and returns true when `interval` is from 300 to 2000 ms inclusive. |
| T2 | For any other interval, `start` returns false and leaves the timer exactly as it was. |
| T3 | `expired(now)` is true only when the timer is armed and at least `interval` ms have passed since `start`. |
| T4 | All timing, in every class of the module, is correct when the 32-bit millisecond counter wraps from `0xFFFFFFFF` to 0. |
| T5 | `stop()` disarms the timer. |
| T6 | `remaining(now)` returns the milliseconds until expiry, and 0 once expired or when not armed. |
| T7 | `rate_to_interval_ms(rate)` returns 60,000 / `rate`, rounded down, for rates from 30 to 200 beats per minute inclusive. For any other rate it returns 0. |
| T8 | `interval_to_rate_bpm(interval)` returns 60,000 / `interval`, rounded to the nearest whole number with a half rounding up, for intervals from 300 to 2000 ms inclusive. For any other interval it returns 0. |
| T9 | `restart(now)` on an armed timer runs the same interval again, counted from `now`, and returns true. On a timer that is not armed it returns false and leaves the timer exactly as it was. |
| T10 | After `RefractoryWindow::begin(now, duration)`, `active(now)` is true while fewer than `duration` ms have passed and false from then on. Before the first `begin` it is false. |
| T11 | `RateLimiter(min_interval)` enforces the upper rate limit. `allow(now)` returns true if no pulse has been allowed yet, or if at least `min_interval` ms have passed since the last allowed pulse. Otherwise it returns false, and a refused request is not recorded. |
| T12 | `escape_interval_ms(base, hysteresis, sensed)` returns `base` after a paced beat (`sensed` false) and `base + hysteresis` after a sensed beat. The result is never more than 2000 ms. |
| T13 | `TimerBank` holds 4 independent timers in slots 0 to 3. Its `start`, `restart`, `stop`, `expired` and `remaining` act on one slot as T1 to T9 describe, except that `stop` returns true. With a slot above 3 they return false (`remaining` returns 0) and change nothing. |
| T14 | `next_expiry(now, slot)` sets `slot` to the armed timer with the least time remaining and returns true. On a tie the lowest slot wins. If no timer is armed it returns false and leaves `slot` unchanged. |
| T15 | `stop_all()` disarms every timer in the bank. |

**Tasks**

1. **Traceability matrix.** Make a table with one row per requirement and the names of the tests that cover it. Every requirement needs at least one normal case and one boundary or failure case.
2. **Write the tests**, named by requirement ID. No sleeping and no real clock: pass `now` values directly.
3. For T4, choose start times just below `0xFFFFFFFF` so the deadline falls after the wrap. Test both just before and just after the deadline, and do this for every class that takes a `now`.
4. **Defect reports.** For each failure: requirement ID, steps, expected, actual, and a severity with a one-line patient-impact argument.
5. **Fix and regress.** Fix one defect at a time and re-run the whole suite after each fix.
6. **Mutation check.** In the fixed code, change one operator at a time (`<` to `<=`, `>=` to `>`, `+` to `-`) and confirm at least one test fails each time. A mutation that survives marks a gap in the tests.

**Check yourself**

- Sanitizers report nothing for this module. Why not, when it has ten defects?
- One defect makes a pacing pulse fire far too early, but only in a narrow time window about every 49.7 days. How would you argue its severity?
- T2 and T9 say "exactly as it was". How does a test prove something did not change?
- Rank the ten defects by severity. Which one would you be most comfortable shipping a device with, and what would you need to write down to justify that?
- Some requirements pass every test you write. How do you tell a correct implementation from a weak test?
- What is the difference between verification (built it right) and validation (built the right thing)?
- What does the mutation check measure that line coverage does not?
