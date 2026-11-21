# Project 7: Test a battery gauge from its spec

Design tests from a specification using equivalence classes and boundary values, and find five defects. Treat the module as a black box as far as possible. The ground rules for the test projects are in the top-level README.

**Module under test:** [`battery_gauge.h`](battery_gauge.h). Put your tests in `tests/`.

*Medical context:* a pacemaker's battery cannot be recharged or swapped; when it runs low, the whole device is replaced in a minor operation. The device therefore tracks battery voltage and raises an **elective replacement indicator (ERI)** while there are still months of life left, then an **end of life (EOL)** status. A false alarm causes an unnecessary operation; a missed one is dangerous. The voltages below are illustrative.

**Specification**

| ID | Requirement |
| --- | --- |
| R1 | `adc_to_millivolts(counts)` converts a 12-bit ADC reading (0 to 4095) to millivolts as `counts * 3300 / 4095`, rounded down. Readings above 4095 are treated as 4095. |
| R2 | `average_mv(samples, n)` returns the mean of `n` readings, rounded down, for `n` up to 64. If `n` is 0 it returns 0. |
| R3 | `classify(mv)` returns `Normal` at 2600 mV and above, `ReplaceSoon` (ERI) from 2400 to 2599 mV, and `EndOfLife` below 2400 mV. |
| R4 | `BatteryMonitor` starts at `Normal`. Its status changes only after 3 consecutive readings that classify as worse than the current status. |
| R5 | `BatteryMonitor` status never improves once it has worsened. |

**Tasks**

1. For each requirement, list the equivalence classes (groups of inputs that should behave the same) and the boundaries between them. Do this on paper first.
2. Write one test per class and one on each side of every boundary. Name each test after its requirement, for example `R3_Boundary_2600_IsNormal`.
3. Run with the sanitizers on. Record every failure as a defect report.
4. Fix each defect, with the failing test written before the fix.
5. List at least one question about the spec that the author should answer.

**Check yourself**

- Why do bugs cluster at boundaries?
- Which inputs did the spec not mention at all? What should happen for each?
- One defect only shows with a larger `n`. What does that say about choosing test data sizes?
- Which of the five defects would be most dangerous in a real device, and why?
