# Project 8: Review an event log for memory bugs

Find at least twelve defects in a module of about 180 lines, first by reading it, then with tools, and record which method caught which bug. Memory errors and undefined behavior are the classic C++ failures, and this module passes a casual test.

**Module under test:** [`src/event_log.h`](src/event_log.h) and [`src/event_log.cpp`](src/event_log.cpp). Put your tests in `tests/`.

*Medical context:* implanted devices keep a small log of notable events, such as a detected fast heart rhythm or a lead warning, which the clinician reads out at a check-up. The log is tiny because device memory is, so old entries are overwritten. Each event has a severity so that alarms stand out. At the check-up the programmer (the clinic's device) asks for a summary, pulls the entries over the wireless link as raw bytes, and usually clears the log afterwards.

**Specification**

| ID | Requirement |
| --- | --- |
| E1 | `log()` stores an event. The message is truncated to 15 characters and is always null-terminated. |
| E2 | The log holds the 16 most recent events. When full, a new event overwrites the oldest. |
| E3 | `count()` returns the number of events stored, never more than 16. |
| E4 | `get(i)` returns the i-th oldest stored event for `0 <= i < count()`, and `nullptr` for any other `i`. |
| E5 | `latest()` returns the most recently logged event. The log must not be empty. |
| E6 | `overflowed()` is false until an event has been overwritten, and true from then on. |
| E7 | The top two bits of `code` are the severity: 0 info, 1 warning, 2 alarm. The value 3 is reserved, so a code with it is invalid: `log()` returns false and stores nothing. For a valid code `log()` returns true. |
| E8 | A null `msg` is stored as an empty message. |
| E9 | `count_by_severity(s)` returns how many events of that severity have been logged, including ones that have since been overwritten. |
| E10 | `count_since(now_ms, window_ms)` returns how many stored events are at most `window_ms` old. An event's age is `now_ms - timestamp_ms` as an unsigned 32-bit value, so the answer stays right when the millisecond counter wraps. |
| E11 | `find_latest(code)` returns the most recently logged stored event with exactly that code, or `nullptr` if there is none. |
| E12 | `dump(out, cap)` copies the stored events, oldest first, into `out` as 22-byte records: timestamp (4 bytes, least significant byte first), code (2 bytes, least significant byte first), message (16 bytes). It writes only whole records, never writes past `out[cap - 1]`, and returns the number of bytes written. |
| E13 | An `EventCursor` walks a log from oldest to newest. Each `next(out)` copies the next stored event into `out` and returns true. When none are left it returns false and leaves `out` unchanged. `rewind()` starts again from the oldest. |
| E14 | `summary()` returns the number of stored events, the overflow flag, the timestamps of the oldest and newest stored events (both 0 for an empty log), and the three severity counts. |
| E15 | `clear()` empties the log. Afterwards `count()` is 0, `overflowed()` is false, every severity count is 0, and the log behaves like a new one. |
| E16 | No call that meets the requirements above causes undefined behavior, wherever the log object and the caller's buffers happen to sit in memory. |

**Tasks**

1. **Read first.** Spend 45 minutes with the code and spec only. Write down every suspected defect and the input that would trigger it.
2. **Write tests** for E1 to E15, including logging exactly 15, 16 and 17 events, a 15-character and a 16-character message, `get(-1)`, timestamps on both sides of the 32-bit wrap, and `dump()` into heap buffers of exactly `cap` bytes (`new uint8_t[cap]`) for several values of `cap`.
3. **Run the tools:** AddressSanitizer and UndefinedBehaviorSanitizer, then `clang-tidy` and `cppcheck`, then Valgrind or Clang's MemorySanitizer for uninitialized reads (both need Linux).
4. **Build a table:** one row per defect, with columns for found by reading, by a test, by a sanitizer, by static analysis.
5. Allocate the log three ways (`static EventLog a;`, a local `EventLog b;`, and `new EventLog`) and explain why one defect appears in only some of them.
6. Rewrite the module so every requirement holds, and re-run everything.

**Check yourself**

- What is undefined behavior, and why can a program with it appear to work?
- Which defects did no tool find? What do they have in common?
- Why is `strncpy` a trap? What would you use instead?
- A sanitizer only reports bugs on paths the tests execute. What does that imply about relying on it?
- One out-of-bounds write is reported by UndefinedBehaviorSanitizer but not by AddressSanitizer. What is different about where that write lands?
- One defect does no harm on your laptop's processor and can crash a small microcontroller. Which rule does it break, and which tool told you?
- The spec says the log "must not be empty" for `latest()`. Is that enough? What should a safety-critical module do when a caller breaks a precondition?
