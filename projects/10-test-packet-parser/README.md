# Project 10: Capstone, verify a telemetry link

Verify a small three-layer telemetry link with eleven seeded defects using unit tests, hand-built test vectors, two fuzzers and coverage measurement, then write a one-page verification report. The link is the part of a device that handles bytes from outside, so it is where robustness and security matter most.

**Module under test,** in [`src/`](src):

| Files | Layer | What it does |
| --- | --- | --- |
| [`packet.h`](src/packet.h), [`packet.cpp`](src/packet.cpp) | Frame | Encodes one packet into a frame and parses one frame back |
| [`frame_reader.h`](src/frame_reader.h), [`frame_reader.cpp`](src/frame_reader.cpp) | Stream | Finds frames in the bytes as they arrive and queues the packets |
| [`command.h`](src/command.h), [`command.cpp`](src/command.cpp) | Command | Turns a packet payload into a typed command and back |

Fuzz harnesses: [`fuzz.cpp`](fuzz.cpp) for `parse`, and [`fuzz_stream.cpp`](fuzz_stream.cpp) for the reader and the command decoder. Put your tests in `tests/`; a starter test is already there.

*Medical context:* at a check-up, the clinic's programmer talks to the implanted device over a wireless link. It can change the pacing rate, change the strength of the pacing pulse (its amplitude in millivolts and its width in microseconds), and read back the device's event log. Radio bytes arrive in bursts of any size, with noise in between. A link that misreads a malformed or malicious packet could corrupt memory, or change the therapy, in a device inside a patient. That is why regulators now expect evidence of cybersecurity testing for connected medical devices.

**Specification**

Frame layer:

| ID | Requirement |
| --- | --- |
| P1 | A frame is: `SYNC` (`0xA5`), `LEN` (0 to 32), `PAYLOAD` (`LEN` bytes), `CHK`. |
| P2 | `CHK` is the XOR of the `LEN` byte and every payload byte. |
| P3 | `encode` writes one frame and returns the bytes written. It returns 0 and writes nothing if the frame does not fit in `cap` or if `in.len` is above 32. |
| P4 | `parse` returns true only when `buf` starts with one complete, valid frame: correct `SYNC`, `LEN` of 32 or less, at least `LEN + 3` bytes available, correct `CHK`. Extra bytes after the frame are ignored. |
| P5 | `parse` never reads past `buf[size - 1]` and never writes outside `out`, for any input at all. |
| P6 | When `parse` returns false, `out` is unchanged. |

Stream layer (`FrameReader`):

| ID | Requirement |
| --- | --- |
| P7 | `feed(data, n)` takes the next `n` bytes received. A frame may be split across calls in any way: the packets produced depend only on the bytes, not on how they were split. `feed` returns the number of packets it added to the queue. |
| P8 | The reader discards bytes until it sees `SYNC`, then collects one frame as defined in P1 and P2. If that frame is invalid (`LEN` above 32, or wrong `CHK`), the reader discards only the `SYNC` byte that started it and resumes the search at the very next byte, so a valid frame that begins inside a rejected one is still found. |
| P9 | Each valid frame becomes one packet in a queue that holds 4. `pop(out)` moves the oldest packet into `out` and returns true. If the queue is empty it returns false and leaves `out` unchanged. `pending()` is the number of packets queued. |
| P10 | If a valid frame arrives while the queue is full, that new packet is discarded and `dropped()` goes up by one. `rejected()` goes up by one for each `SYNC` discarded under P8. Both start at 0. |
| P11 | `reset()` discards any partial frame and every queued packet, and sets both counters to 0. |

Command layer:

| ID | Requirement |
| --- | --- |
| P12 | A command is a packet payload: one command ID byte, then that command's fields in the order shown below, with no padding. 16-bit fields are sent most significant byte first (big-endian). |
| P13 | `encode_command(cmd, out)` writes the payload and returns true when `cmd.id` is in the table and every field that command uses is in range. Otherwise it returns false and leaves `out` unchanged. Fields the command does not use are ignored. |
| P14 | `decode_command(in, out)` returns true only when the first payload byte is an ID from the table, `in.len` is exactly that command's length, and every field is in range. Fields the command does not use are set to 0. When it returns false, `out` is unchanged. |

| Command | ID | Fields after the ID | Payload length | Valid range |
| --- | --- | --- | --- | --- |
| `Ping` | `0x01` | none | 1 | |
| `SetRate` | `0x10` | `rate_bpm` (16-bit) | 3 | 30 to 200 beats per minute |
| `SetOutput` | `0x11` | `amplitude_mv` (16-bit), then `width_us` (16-bit) | 5 | Amplitude 500 to 7500 mV; width 100 to 1500 µs |
| `ReadEvents` | `0x20` | `first_event` (8-bit), then `event_count` (8-bit) | 3 | `first_event` 0 to 15; `event_count` 1 to 16; `first_event + event_count` at most 16 |

`ReadEvents` asks for a run of entries from the 16-entry event log of Project 8.

**Tasks**

1. **Unit tests for the frame layer,** from P1 to P6, including empty payload, 32-byte payload, a 1-byte buffer, a truncated frame, a wrong `SYNC` and a wrong `CHK`. Put input buffers on the heap at their exact size (`new uint8_t[n]`) so AddressSanitizer can see an over-read.
2. **Round-trip property tests.** For 10,000 random packets, `parse(encode(p))` must equal `p`. For every command at both ends of each range, `decode_command(encode_command(c))` must equal `c`. Note that both pass.
3. **Independent test vectors.** Work out by hand, from P1 and P2 only, the exact bytes for a 2-byte payload `01 02`. Then work out, from P12 only, the payload for `SetRate` at 60 beats per minute and for `SetOutput` at 2500 mV and 400 µs. Compare with what `encode` and `encode_command` produce. Explain why the round-trip tests could not catch what you find.
4. **Unit tests for the stream layer,** from P7 to P11: a frame in one call, one byte per call, and split at every position; noise before a frame; a rejected frame followed by a good one; a good frame that begins inside a rejected one; a full queue and one packet more; `reset()` in the middle of a frame.
5. **Unit tests for the command layer,** from P12 to P14: each field at both ends of its range and just outside, every wrong length, unknown IDs. `encode_command` will refuse to build an out-of-range packet for you, so build those by hand.
6. **Fuzz it.** Build both harnesses with Clang (the command is at the top of each file) and run each for a minute. Each crash comes with the input that caused it; turn each one into a permanent unit test, fix the defect, and run the fuzzer again until it stays quiet. On a Mac the fuzzer needs Homebrew's Clang, `$(brew --prefix llvm)/bin/clang++`, because Apple's Clang ships without the fuzzer library.
7. **Give the fuzzer an oracle.** A fuzzer only reports crashes unless you tell it what "wrong" looks like. Extend `fuzz_stream.cpp`: whenever `decode_command` accepts a packet, `encode_command` must accept the decoded command and reproduce the same payload; call `__builtin_trap()` if it does not. Start the fuzzer from a folder holding a few valid frames taken from your unit tests, and run it for a few minutes.
8. **Measure coverage** of the unit tests with `gcov` or `llvm-cov` (on a Mac, `xcrun llvm-cov` and `xcrun llvm-profdata`). Reach 100% of lines and branches in the fixed module, then explain why 100% coverage still did not prove the original correct.
9. **Fix** all defects, test first, and re-run both fuzzers for five minutes with no findings.
10. **Verification report,** one page: scope, what was tested and how, a requirement-to-test matrix, defects found with severity, coverage achieved, and what remains untested or assumed.

**Check yourself**

- Why can two matching bugs in an encoder and a decoder hide each other? Where else does that pattern show up?
- What is a fuzzer good at finding, and which of the eleven defects could it never find without an oracle?
- Did the stream fuzzer find anything your unit tests for the reader had missed? What did it try that you did not?
- Why validate `LEN` before using it, and why is "never trust a length field" the first rule of parsers?
- `encode_command` and `decode_command` both check ranges. Why is it not enough for the encoder alone to check?
- What does the reader do if the last byte of a frame never arrives? Is that in the spec?
- Not all eleven defects are equally serious. Which would you fix first, and which could wait for the next release?
- If you could run only one of these techniques on a new module, which would it be, and what would you miss?
- What would you write under "residual risk" in the report?
