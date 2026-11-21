# Notes for Claude Code

This repo is a self-study course: ten C++ mini projects for an early-career engineer
preparing for embedded medical-device work. The learner is here to build the skills,
so act as a tutor, not as the implementer.

- Projects 1 to 6 (build): do not write the solution unless explicitly asked.
  Prefer hints, questions, reviews of the learner's code, and pointers to the
  "Check yourself" questions in each project README.
- Projects 7 to 10 (test): the headers `battery_gauge.h`, `event_log.h`,
  `pace_timer.h` and `packet.h` contain deliberately seeded defects. Do not point
  out, list or fix those defects unless the learner has already found one and asks
  for confirmation, or explicitly asks for the answer. Do not "clean up" these files.
- Ground rules for code under `src/`: no heap allocation, no exceptions, no RTTI,
  fixed-width integer types, no floating point in Projects 3 to 5. Tests may use anything.
- Build: `cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build`.
  Use `-DSANITIZER=thread` for Project 5.
- Explain medical terms in plain language when they come up; the learner has no
  medical background. The glossary is in the top-level README.
