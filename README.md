# 10 C++ Mini Projects for Embedded Medical Device Work

Ten projects, about 50 to 60 hours in total. In Projects 1 to 6 you build something;
in 7 to 10 you are handed a finished module with defects seeded in it and have to test
and review it. Everything runs on a laptop with no hardware, but follows the rules
firmware for an implantable device lives by: fixed memory, predictable timing, and no
surprises.

| # | Project | Core concepts | Time |
| --- | --- | --- | --- |
| 1 | [Fixed-capacity ring buffer](projects/01-ring-buffer) | Templates, `std::array`, const-correctness, value semantics | 3–4 h |
| 2 | [Memory pool allocator](projects/02-memory-pool) | Placement new, alignment, object lifetime, RAII | 5–6 h |
| 3 | [Register and telemetry layer](projects/03-registers-telemetry) | Bit manipulation, `volatile`, endianness, struct layout, CRC | 5–6 h |
| 4 | [Pacing state machine](projects/04-pacing-state-machine) | `enum class`, table-driven FSM, time abstraction, unit testing | 6–8 h |
| 5 | [Interrupt-style ECG pipeline](projects/05-ecg-pipeline) | Atomics, lock-free SPSC queue, fixed-point math, C# interop | 8–10 h |
| 6 | [Power-loss-safe settings store](projects/06-settings-store) | Trivially copyable structs, versioning, CRC reuse, fault injection | 5–6 h |
| 7 | [Test: battery gauge](projects/07-test-battery-gauge) | Test design from a spec, equivalence classes, boundary values | 3–4 h |
| 8 | [Test: event log](projects/08-test-event-log) | Undefined behavior, sanitizers, static analysis | 3–4 h |
| 9 | [Test: pace timer](projects/09-test-pace-timer) | Requirement traceability, fake time, defect reports | 4–5 h |
| 10 | [Test: packet parser](projects/10-test-packet-parser) | Fuzzing, coverage, independent test vectors, verification report | 6–8 h |

## Getting started on a Mac

1. Install Apple's command line tools. This gives you Apple Clang, Git and the LLDB debugger.

   ```bash
   xcode-select --install
   ```

2. Install [Homebrew](https://brew.sh) if you do not have it, then the build and analysis tools.

   ```bash
   brew install llvm cmake ninja cppcheck googletest
   ```

3. From the repo root, configure, build and run the tests.

   ```bash
   cmake -S . -B build -G Ninja
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```

   On a fresh clone this builds one test executable (the Project 7 starter test) and it
   should pass. The other projects get a test executable as soon as you add a `.cpp`
   file to their `tests/` folder.

Things that differ on a Mac:

- **Two Clangs.** CMake uses Apple Clang (`/usr/bin/clang++`) by default, which is fine
  for the CMake build in every project. Homebrew's LLVM is not put on your `PATH`; it
  lives under `$(brew --prefix llvm)/bin` and is where `clang-tidy`, `clang-format` and
  `llvm-cov` come from. To use them by name, add that folder to your `PATH`:

  ```bash
  echo 'export PATH="$(brew --prefix llvm)/bin:$PATH"' >> ~/.zshrc
  ```

- **The Project 10 fuzzer needs Homebrew's Clang.** Apple Clang does not ship the
  fuzzer library, so build `fuzz.cpp` with `$(brew --prefix llvm)/bin/clang++` instead
  of plain `clang++`.
- **`g++` is not GCC.** On a Mac, `g++` is Apple Clang under another name. For a real
  second compiler, run `brew install gcc` and use the versioned command it installs
  (for example `g++-16`).
- **Use LLDB, not GDB.** GDB is poorly supported on Apple silicon; LLDB comes with the
  command line tools.
- **Editor.** VS Code with the clangd and CMake Tools extensions works well. The build
  writes `build/compile_commands.json`, which clangd picks up automatically.

## Build and run

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Every `.cpp` in `projects/<name>/tests/` (plus `projects/<name>/src/`) becomes one test
executable. The address and undefined-behavior sanitizers are on by default; use
`-DSANITIZER=thread` for Project 5.

## Recommended setup

Install one editor, two compilers, CMake, and a test library. Everything below is free
for learning.

**Editor or IDE (pick one)**

| Option | Best when | Notes |
| --- | --- | --- |
| Visual Studio 2026 Community | You are on Windows | Handles C++ and C# in one place, which matches the job. Install the "Desktop development with C++" and ".NET desktop development" workloads. |
| VS Code + clangd + CMake Tools extensions | Any OS, lightweight | clangd gives accurate completion and inline clang-tidy warnings. Add the C# Dev Kit extension for the .NET stretch. |
| CLion | Any OS, want the most help | Free for non-commercial use such as learning. Strong debugger and refactoring, CMake-native. |

Ask the new team which IDE they use before day one and practice in that one if possible.

**Toolchain**

- **GCC and Clang, both.** Two compilers catch more warnings, and Clang is needed for the fuzzer in Project 10.
- **CMake + Ninja** for builds.
- **GDB or LLDB.** Learn breakpoints, watchpoints and inspecting raw memory.
- **On Windows, add WSL2 with Ubuntu.** It is the easiest way to get GCC, Clang and every sanitizer these projects use.

```bash
# Ubuntu / WSL2
sudo apt install build-essential clang clang-tidy clang-format \
     cmake ninja-build gdb cppcheck libgtest-dev
# macOS: xcode-select --install, then
brew install llvm cmake ninja cppcheck googletest
```

**Analysis tools (used heavily in Projects 7 to 10)**

- **Sanitizers** (`-fsanitize=address,undefined`, and `thread` for Project 5): built into GCC and Clang.
- **clang-tidy** and **cppcheck:** static analysis. Start with the `bugprone-*`, `cppcoreguidelines-*` and `clang-analyzer-*` checks.
- **clang-format:** automatic formatting, so reviews are about logic.
- **gcov or llvm-cov:** test coverage, needed in Project 10.
- **[Compiler Explorer](https://godbolt.org):** shows the assembly for any snippet in the browser.

**Libraries**

| Library | Use it for |
| --- | --- |
| [GoogleTest](https://github.com/google/googletest) | Unit tests in every project (the CMake file downloads it if it is not installed) |
| [{fmt}](https://github.com/fmtlib/fmt) | Readable output in tests and tools; never in the "firmware" code |
| [Embedded Template Library](https://www.etlcpp.com) | Fixed-capacity containers with no heap. Read it after Projects 1 and 2 to compare with your own versions. |
| .NET SDK | The C# interop stretch in Project 5 |

**Later, for real hardware:** the Arm GNU toolchain (`arm-none-eabi-gcc`) and an STM32
Nucleo board. None of the ten projects need it.

## Ground rules for the "firmware" code

Tests can use anything. Code under `src/` follows these:

- No `new`, `delete`, `malloc`, `std::vector`, `std::string` or `std::function`. All memory is sized at compile time.
- No exceptions and no RTTI. Errors come back as return values (`bool`, an `enum class`, or `std::optional`).
- Fixed-width integers only (`uint8_t`, `int32_t`), never bare `int` for data that has a defined size.
- No floating point in Projects 3 to 5. Many small microcontrollers have no FPU.
- Every project ships with tests.

**For each project:** write it without looking anything up first, then read the "Check
yourself" questions, then refactor. Being able to answer those questions out loud
matters more than the code.

## How the test projects work (7 to 10)

Each project gives a short specification and a finished module. The module compiles
without warnings, and it contains seeded defects. In a medical device company, verifying
code against its requirements is as large a part of the job as writing it.

- The specification is the source of truth. Where code and spec disagree, the code is wrong.
- Write tests from the spec first, before studying the implementation closely.
- For every defect found, write a short report: which requirement it breaks, the smallest input that shows it, expected versus actual result, and how serious it would be in a real device.
- Also note anything the spec leaves unclear. Finding gaps in requirements is part of the work.
- The answer key is not in this repo. Ask whoever gave you the course once your reports are written.

## Medical terms used here

No medical background is assumed. Each project also explains its terms where they first matter.

| Term | Plain meaning |
| --- | --- |
| Pacemaker | A small implanted device that watches the heart's rhythm and triggers a beat when the heart is too slow |
| Atrium, ventricle | The heart's upper (filling) and lower (pumping) chambers; there are two of each |
| Lead | The insulated wire from the device into the heart; it carries signals both ways |
| Lead impedance | The lead's electrical resistance in ohms; an abnormal value suggests a damaged or displaced lead |
| Intrinsic beat | A beat the heart starts on its own |
| Sensing | The device detecting the heart's own electrical activity |
| Pacing | The device delivering a small electrical pulse to trigger a beat |
| Inhibit | Withholding a pace because an intrinsic beat was sensed in time |
| VVI | A pacing mode: paces the ventricle, senses the ventricle, inhibits on a sensed beat |
| Lower rate interval | The longest gap the device allows between beats before it paces |
| Refractory period | A short window after a beat when sensed signals are ignored |
| Rate hysteresis | Letting the heart's own rate fall a little below the pacing rate before pacing starts, to favor intrinsic beats |
| ECG | Electrocardiogram: the heart's electrical signal recorded over time |
| QRS complex, R wave | The sharp spike in an ECG when the ventricles contract; the R wave is its peak |
| R-R interval | Time between two consecutive R waves; heart rate = 60,000 / R-R in ms |
| Bradycardia | A heart rate that is too slow, typically below 60 beats per minute at rest |
| Tachycardia | A heart rate that is too fast, typically above 100 beats per minute at rest |
| Telemetry | The wireless link an implanted device uses to report data |
| Programmer | The clinic device that reads telemetry and changes the implant's settings |
| Programmed parameters | The settings a clinician chooses for a patient's device, such as pacing rate |
| Pulse amplitude, pulse width | The voltage of each pacing pulse and how long it lasts |
| ERI | Elective replacement indicator: the battery is low enough that the device should be replaced within months |
| EOL | End of life: the battery can no longer be relied on to run the device |
| ADC | Analog-to-digital converter: hardware that turns a voltage into a number |

## After the ten projects

You are ready when you can explain each item below without notes.

- [ ] Where an object lives (stack, static, heap, flash) and how long it lasts
- [ ] RAII and the rule of five, including when to delete copy and move
- [ ] Why embedded code avoids the heap, exceptions and RTTI
- [ ] Alignment, padding and `sizeof` for any struct you write
- [ ] `const`, `constexpr`, `volatile` and `static`, and what each one changes
- [ ] Signed versus unsigned arithmetic, overflow and wraparound
- [ ] Byte order and why wire formats are serialized explicitly
- [ ] Templates versus virtual functions for polymorphism, and the cost of each
- [ ] Sharing data between an interrupt and the main loop safely
- [ ] Writing timing logic that can be tested without hardware
- [ ] Deriving tests from a spec: equivalence classes, boundaries, and one test per requirement
- [ ] What each tool can and cannot find: sanitizers, static analysis, fuzzing, coverage
- [ ] Writing a defect report with a minimal trigger, expected versus actual, and severity

**Worth reading next**

- *Making Embedded Systems* by Elecia White.
- *Effective Modern C++* by Scott Meyers.
- The [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).
- MISRA C++ and IEC 62304, at overview level. MISRA is a restricted C++ subset used in safety-critical code; IEC 62304 is the software lifecycle standard for medical devices.
