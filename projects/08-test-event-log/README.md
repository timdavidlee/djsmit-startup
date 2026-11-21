# Project 8: Review an event log for memory bugs

Find at least seven defects in a 40-line module, first by reading it, then with tools, and record which method caught which bug. Memory errors and undefined behavior are the classic C++ failures, and this module passes a casual test.

**Module under test:** [`event_log.h`](event_log.h). Put your tests in `tests/`.

*Medical context:* implanted devices keep a small log of notable events, such as a detected fast heart rhythm or a lead warning, which the clinician reads out at a check-up. The log is tiny because device memory is, so old entries are overwritten.

**Specification**

| ID | Requirement |
| --- | --- |
| E1 | `log()` stores an event. The message is truncated to 15 characters and is always null-terminated. |
| E2 | The log holds the 16 most recent events. When full, a new event overwrites the oldest. |
| E3 | `count()` returns the number of events stored, never more than 16. |
| E4 | `get(i)` returns the i-th oldest stored event for `0 <= i < count()`, and `nullptr` for any other `i`. |
| E5 | `latest()` returns the most recently logged event. The log must not be empty. |
| E6 | `overflowed()` is false until an event has been overwritten, and true from then on. |

**Tasks**

1. **Read first.** Spend 30 minutes with the code and spec only. Write down every suspected defect and the input that would trigger it.
2. **Write tests** for E1 to E6, including logging exactly 15, 16 and 17 events, a 15-character and a 16-character message, and `get(-1)`.
3. **Run the tools:** AddressSanitizer and UndefinedBehaviorSanitizer, then `clang-tidy` and `cppcheck`, then Valgrind or Clang's MemorySanitizer for uninitialized reads.
4. **Build a table:** one row per defect, with columns for found by reading, by a test, by a sanitizer, by static analysis.
5. Allocate the log three ways (`static EventLog a;`, a local `EventLog b;`, and `new EventLog`) and explain why one defect appears in only some of them.
6. Rewrite the module so every requirement holds, and re-run everything.

**Check yourself**

- What is undefined behavior, and why can a program with it appear to work?
- Which defects did no tool find? What do they have in common?
- Why is `strncpy` a trap? What would you use instead?
- A sanitizer only reports bugs on paths the tests execute. What does that imply about relying on it?
- The spec says the log "must not be empty" for `latest()`. Is that enough? What should a safety-critical module do when a caller breaks a precondition?
