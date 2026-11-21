# Project 10: Capstone, verify a packet parser

Verify a packet encoder and parser with six seeded defects using unit tests, a fuzzer and coverage measurement, then write a one-page verification report. A parser is the part of a device that handles bytes from outside, so it is where robustness and security matter most.

**Module under test:** [`packet.h`](packet.h). Fuzz harness: [`fuzz.cpp`](fuzz.cpp). Put your tests in `tests/`.

*Medical context:* implanted devices receive commands over a wireless link. A parser that misreads a malformed or malicious packet could corrupt memory in a device inside a patient, which is why regulators now expect evidence of cybersecurity testing for connected medical devices.

**Specification**

| ID | Requirement |
| --- | --- |
| P1 | A frame is: `SYNC` (`0xA5`), `LEN` (0 to 32), `PAYLOAD` (`LEN` bytes), `CHK`. |
| P2 | `CHK` is the XOR of the `LEN` byte and every payload byte. |
| P3 | `encode` writes one frame and returns the bytes written. It returns 0 and writes nothing if the frame does not fit in `cap` or if `in.len` is above 32. |
| P4 | `parse` returns true only when `buf` starts with one complete, valid frame: correct `SYNC`, `LEN` of 32 or less, at least `LEN + 3` bytes available, correct `CHK`. Extra bytes after the frame are ignored. |
| P5 | `parse` never reads past `buf[size - 1]` and never writes outside `out`, for any input at all. |
| P6 | When `parse` returns false, `out` is unchanged. |

**Tasks**

1. **Unit tests from the spec,** including empty payload, 32-byte payload, a 1-byte buffer, a truncated frame, a wrong `SYNC` and a wrong `CHK`. Put input buffers on the heap at their exact size (`new uint8_t[n]`) so AddressSanitizer can see an over-read.
2. **Round-trip property test.** For 10,000 random packets, `parse(encode(p))` must equal `p`. Note that this passes.
3. **Independent test vector.** Work out by hand, from P1 and P2 only, the exact bytes for a 2-byte payload `01 02`. Compare with what `encode` produces. Explain why the round-trip test could not catch this defect.
4. **Fuzz it.** Build `fuzz.cpp` with Clang (the command is at the top of the file) and run it for a minute. Each crash it reports comes with the input that caused it; turn each one into a permanent unit test.
5. **Measure coverage** of the unit tests with `gcov` or `llvm-cov`. Reach 100% of lines and branches in the fixed module, then explain why 100% coverage still did not prove the original correct.
6. **Fix** all defects, test first, and re-run the fuzzer for five minutes with no findings.
7. **Verification report,** one page: scope, what was tested and how, a requirement-to-test matrix, defects found with severity, coverage achieved, and what remains untested or assumed.

**Check yourself**

- Why can two matching bugs in an encoder and a decoder hide each other? Where else does that pattern show up?
- What is the fuzzer good at finding, and which of the six defects could it never find?
- Why validate `LEN` before using it, and why is "never trust a length field" the first rule of parsers?
- If you could run only one of these techniques on a new module, which would it be, and what would you miss?
- What would you write under "residual risk" in the report?
