# Project 4: Pacing state machine

Build a simplified single-chamber pacemaker as a table-driven finite state machine, driven by a fake clock so every test is deterministic. Most device firmware is state machines reacting to events and timers, and testable timing logic is the skill that transfers.

*Medical context:* a healthy heart triggers its own beats; these are called **intrinsic** beats. A pacemaker **senses** the heart's electrical activity through its lead and **paces** (delivers a small electrical pulse that triggers a beat) only when the heart does not beat on its own in time. The simplest mode is **VVI**: it paces the **V**entricle (the heart's main pumping chamber), senses the **V**entricle, and **I**nhibits its pulse when it senses an intrinsic beat. Two timers govern it:

- **Lower rate interval (LRI):** the longest allowed gap between beats. At 1000 ms the heart never drops below 60 beats per minute.
- **Refractory period:** a short window after each beat (250 ms here) when the device ignores what it senses, because the signal is still settling and could be mistaken for a new beat.

This is a teaching model, far simpler than a real device.

**Transition table**

| State | Event | Next state | Action |
| --- | --- | --- | --- |
| Alert | Sense | Refractory | Restart timers (pace inhibited) |
| Alert | LRI expired | Refractory | Deliver pace, restart timers |
| Refractory | Sense | Refractory | Ignore |
| Refractory | Refractory expired | Alert | None |

**Tasks**

1. Define `enum class State : uint8_t` and `enum class Event : uint8_t`. Encode the table above as a `constexpr` array of `{state, event, next, action}` rows, where `action` is a plain function pointer or member function pointer. No `switch` in the dispatch path.
2. Abstract time behind an interface the FSM receives by reference: `struct Clock { virtual uint32_t now_ms() const = 0; };`. Production would read a hardware timer; tests use a `FakeClock` with `advance(ms)`.
3. Abstract the output the same way: `struct PaceOutput { virtual void deliver() = 0; };`. The test double records the timestamp of each pace.
4. Drive the machine with `tick()`, called every 1 ms of fake time, plus `on_sense()`.
5. Write these tests:
    - No sensed beats: paces land at exactly 1000, 2000 and 3000 ms.
    - A sensed beat every 800 ms: zero paces in 10 seconds.
    - A sense 100 ms after a pace is ignored; the next pace is still 1000 ms after the previous one.
    - Intrinsic beats stop after 800 ms: the next pace lands at 1800 ms.
6. Make the tick counter a `uint32_t` in milliseconds and write a test proving the timing still works across the wraparound at 2^32 ms (about 49.7 days).

**Check yourself**

- What does `enum class` give you over a plain `enum`?
- What does a virtual call cost in memory and time? How would you remove it with a template parameter instead (static polymorphism), and what is the trade-off?
- Why does `now - start >= interval` survive unsigned wraparound when `now >= start + interval` does not?
- Why is a table easier to review and verify against a specification than nested `if` statements?
- What should the machine do on an event with no matching row? Decide, then make it explicit.

**Stretch:** add rate hysteresis (see the glossary in the top-level README) or a second chamber (atrium sensing), and see how fast the table grows. Add a watchdog: if `tick()` is not called for 50 ms, enter a `Fault` state that paces at a fixed safe rate.
