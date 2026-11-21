# Project 12: Make a telemetry monolith testable

Take one 270-line function that works but cannot be unit tested, and change its shape until every decision in it has a fast test of its own, without changing what it does. Most firmware you will work on already exists and looks like this; adding a feature safely starts with getting it under test.

**Code to refactor:** [`src/telemetry_task.cpp`](src/telemetry_task.cpp), with [`src/telemetry_task.h`](src/telemetry_task.h) and [`src/hw.h`](src/hw.h). **Safety net:** [`tests/golden_master_test.cpp`](tests/golden_master_test.cpp), which uses the fake hardware in [`tests/fake_hw.cpp`](tests/fake_hw.cpp).

No defects are seeded in this project. If you think you have found a bug, write it down and keep the behavior as it is; a refactoring step never changes behavior.

*Medical context:* at a check-up the clinician holds a **programmer** (the clinic's device) near the patient. Over a short-range radio link called **telemetry** it opens a **session** with the implant, reads the **programmed parameters** (the settings chosen for this patient, such as pacing rate and pulse strength) and may change them. It can also start a brief **test mode** that paces at a fixed rate while the clinician watches the heart respond. The implant must refuse anything unsafe: a wrong access code, a value outside its limits, or a change in the middle of a test.

**What the code does today**

`telemetry_poll()` is called every 10 ms. Each command is 4 bytes: command, first argument, second argument, one unused byte. Each reply is 4 bytes: `0x06` (accepted) or `0x15` (refused), the command, and two data bytes; on a refusal the first data byte is the reason.

| Command | Arguments | Meaning |
| --- | --- | --- |
| `0x01` | access code, 2 bytes | Open a session. Three wrong codes in a row lock the implant for 30 seconds. |
| `0x02` | none | Close the session. |
| `0x10` | parameter id | Read a parameter. |
| `0x11` | parameter id, new value | Write a parameter, if it is within its limits and agrees with the other parameters. |
| `0x20` | rate in beats per minute, seconds | Enter test mode. |
| `0x21` | none | Leave test mode. |

A session also ends after 5 seconds without a command, or as soon as the programmer goes out of range. The parameter ids and units are in `telemetry_task.h`; the limits are in the code.

**Rules**

- Behavior must not change. The same bytes in give the same register writes out, at the same times.
- The golden master passes after every step. Commit each time it does, so a step that breaks it can be thrown away.
- No rewrite from scratch. Move and extract the existing code in small steps.
- Do not edit the recorded traces. You may change how the golden master calls the code, never what it expects.

**Tasks**

1. **Read and mark.** Go through `telemetry_poll()` and label each block with the job it does: receiving bytes, session and lockout state, decoding commands, parameter rules, pacing hardware, event recording, timeouts. List the jobs, and count how many times the same lines for "end test mode" appear.
2. **Name the obstacle.** Write down what a test of "the upper rate limit must be above the lower rate" needs today before it can check anything. A **seam** is a place where you can change what code does without editing it. The golden master uses a **link seam**: `hw.h` has no implementation in `src/`, so the test executable links `fake_hw.cpp` instead. Note what that costs: global state, one fake per executable, and tests that only see register writes.
3. **Trust the net.** Run the golden master. Change one constant in `telemetry_task.cpp`, see which test fails and how to read the trace diff, then undo the change.
4. **Extract the pure decisions.** Pull the logic that needs no hardware and no stored state into free functions that take values and return values, for example "is this write allowed, given the current parameters, and if not why". Unit test each one directly, including every limit.
5. **Remove the repetition.** Give "end test mode" and "record an event" one function each.
6. **Move the statics into a class.** The file-scope variables become members of one class and `telemetry_poll()` becomes a member function. Keep the free `telemetry_poll()` and `telemetry_reset()` working on a single instance.
7. **Add an object seam.** Pass the hardware and the clock into the class instead of calling `hw_read_reg()`, `hw_write_reg()` and `hw_now_ms()` directly. Do it twice: once as an abstract interface taken by reference, once as a template parameter. Compare the two for code size (`size` on the object file), RAM, and how the tests read.
8. **Thin wrapper.** The free `telemetry_poll()` ends up as a few lines that forward to an instance wired to the real `hw_*` functions. The golden master still passes through it.
9. **Fast unit tests.** With a hand-written fake that you pass in, and no globals, write tests for the session, the lockout, test mode and both timeouts. They must pass in any order: run with `--gtest_shuffle`.
10. **Mutation check.** As in Project 9, change one operator or constant at a time and confirm that a unit test fails, not only the golden master.
11. **Decide the golden master's future.** Keep it, shrink it or delete it, and write two sentences on why.

**Check yourself**

- What does a golden master (characterization test) prove, and what does it not prove? What happens if the recorded behavior contains a bug?
- What is the difference between a link seam and an object seam? When is a link seam the only practical first step?
- Why do file-scope statics make tests depend on the order they run in? What did `telemetry_reset()` cost the production code?
- What does a virtual call cost on a small microcontroller, in flash, RAM and time? When would you choose the template version instead?
- Which of your extracted functions can be `constexpr`, and what would that let the compiler check?
- The golden master compares write times as well as values. Which refactoring mistakes does that catch that comparing values alone would miss?

**Stretch:** replace the per-parameter `switch` with a `constexpr` table of limits and the command `switch` with a table of handlers, as in Project 4. Then add a seventh parameter and count the lines you had to touch, compared with adding it to the original function.
