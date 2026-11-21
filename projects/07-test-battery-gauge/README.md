# Project 7: Test a battery gauge from its spec

Design tests from a specification using equivalence classes and boundary values, and find ten defects. Treat the module as a black box as far as possible. The ground rules for the test projects are in the top-level README.

**Module under test:** [`src/battery_gauge.h`](src/battery_gauge.h) and [`src/battery_gauge.cpp`](src/battery_gauge.cpp). Put your tests in `tests/`; [`tests/starter_test.cpp`](tests/starter_test.cpp) shows the setup.

*Medical context:* a pacemaker's battery cannot be recharged or swapped; when it runs low, the whole device is replaced in a minor operation. The device therefore tracks battery voltage and raises an **elective replacement indicator (ERI)** while there are still months of life left, then an **end of life (EOL)** status. A false alarm causes an unnecessary operation; a missed one is dangerous. The device also adds up the charge it has drawn, so the clinic can show the patient a percentage and an estimate of the months left. The voltages and limits below are illustrative.

**Specification**

Voltage measurement

| ID | Requirement |
| --- | --- |
| R1 | `adc_to_millivolts(counts)` converts a 12-bit ADC reading (0 to 4095) to millivolts as `counts * 3300 / 4095`, rounded down. Readings above 4095 are treated as 4095. |
| R2 | `average_mv(samples, n)` returns the mean of `n` readings, rounded down, for `n` up to 64. If `n` is 0 it returns 0. |
| R3 | `classify(mv)` returns `Normal` at 2600 mV and above, `ReplaceSoon` (ERI) from 2400 to 2599 mV, and `EndOfLife` below 2400 mV. |
| R6 | `trimmed_mean_mv(samples, n)`, for `n` from 3 to 64, leaves out one lowest and one highest reading and returns the mean of the rest, rounded down. For `n` of 1 or 2 it returns the plain mean, and for 0 it returns 0. |
| R7 | `is_plausible(mv)` is true from 1800 to 3300 mV inclusive and false for anything else. |

Status tracking (`BatteryMonitor`)

| ID | Requirement |
| --- | --- |
| R4 | `BatteryMonitor` starts at `Normal`. Its status changes only after 3 consecutive readings that classify as worse than the current status. |
| R5 | `BatteryMonitor` status never improves once it has worsened. |
| R8 | An implausible reading (R7) is ignored: it does not change the status, and it neither counts toward nor interrupts a run of consecutive readings. `sensor_fault()` is true after 5 consecutive implausible readings and false again after the next plausible one. |
| R12 | `tick_day()` is called once a day. As the one exception to R4, the status becomes `EndOfLife` on the 90th call after the status became `ReplaceSoon`, whatever the voltage. Calls made while the status is `Normal` have no effect. |

Charge accounting

| ID | Requirement |
| --- | --- |
| R9 | `ChargeCounter::add(current_ua, seconds)` adds `current_ua * seconds` to a running total. `consumed_uah()` returns the total in microamp-hours (total / 3600), rounded down and capped at 4,294,967,295. |
| R10 | `percent_remaining(consumed_uah, capacity_uah)` returns `(capacity - consumed) * 100 / capacity`, rounded down. It returns 0 when `consumed` is at or above `capacity`, or when `capacity` is 0. |
| R11 | `months_remaining(remaining_uah, avg_current_ua)` returns `remaining_uah / (avg_current_ua * 730)`, rounded down and capped at 120. If `avg_current_ua` is 0 it returns 120. |

Top-level gauge (`BatteryGauge`)

| ID | Requirement |
| --- | --- |
| R13 | `add_sample(counts)` converts the reading (R1) and stores it. On every 8th sample it takes the trimmed mean (R6) of those 8, passes it to the monitor, and starts a new group. `last_mv()` returns the most recent trimmed mean, or 0 before the first group is complete. |
| R14 | `report(avg_current_ua)` returns the monitor status, `last_mv()`, the sensor fault flag, the percentage from R10, and the months from R11 using `capacity - consumed` (0 if more than the capacity has been consumed). `add_load` and `tick_day` pass straight through to the charge counter and the monitor. |

**Tasks**

1. For each requirement, list the equivalence classes (groups of inputs that should behave the same) and the boundaries between them. Do this on paper first.
2. Write one test per class and one on each side of every boundary. Name each test after its requirement, for example `R3_Boundary_2600_IsNormal`.
3. Run with the sanitizers on. Record every failure as a defect report.
4. Fix each defect, with the failing test written before the fix.
5. Test `BatteryGauge` last, through its public functions only. Note which of your earlier defects are visible at this level and which are not.
6. List at least three questions about the spec that the author should answer.

**Check yourself**

- Why do bugs cluster at boundaries?
- Which inputs did the spec not mention at all? What should happen for each?
- One defect only shows with a larger `n`. What does that say about choosing test data sizes?
- One defect passes a test on each side of its boundary and only fails far away from it. How many values from one equivalence class are enough?
- Not every function here has a defect. How confident are you about the ones where you found none, and what is that confidence based on?
- Which of the ten defects would be most dangerous in a real device, and why?
