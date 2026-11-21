# Project 11: Refactor a legacy lead monitor

Turn a working module written in old C style into modern C++ without changing what it does, one small step at a time, with the tests passing after every step. Most firmware work is changing code that already exists, and in a regulated product "it still behaves exactly as before" has to be shown, not assumed.

**Legacy code:** [`src/lead_monitor.h`](src/lead_monitor.h) and [`src/lead_monitor.cpp`](src/lead_monitor.cpp). **Safety net:** [`tests/characterization_test.cpp`](tests/characterization_test.cpp), 34 tests that pin what the code does today. There are no seeded defects here: the code is correct, it is just hard to change.

*Medical context:* a **lead** is the insulated wire from the device into the heart. The device checks each lead regularly by passing a tiny **measurement current** through it and working out its electrical resistance (**lead impedance**, in ohms). A very low value suggests the insulation is damaged; a very high value, or a sudden change, suggests the wire is broken or has moved. There is one lead in the **atrium** (upper chamber) and one in the **ventricle** (lower chamber), with different limits. The measurement current must never stay on after a measurement, because it drains the battery and can interfere with sensing. The limits in the code are illustrative.

**What the module does**

- `lm_measure` takes three raw readings, rejects the measurement if a reading is out of range or the three disagree, and otherwise uses the middle one.
- It keeps the last, lowest and highest result and the six most recent results for each lead.
- A result outside the lead's limits, or a jump of more than 500 ohms from the previous result, latches an alert and calls the alert callback once. The alert stays until `lm_clear_alert`.
- The details are in the tests. Where this list and the tests disagree, the tests win.

**Rules**

- Behavior must not change. The characterization tests are the definition of behavior.
- Build and run the tests after every step. If one fails, undo the step; do not fix forward.
- One kind of change per commit: rename, or change a type, or move code, never two at once.
- No rewrite from scratch. Every commit is a small edit of the code that was there before.
- New code follows the ground rules in the top-level README: no heap, no exceptions, no RTTI, fixed-width integers.

**Tasks**

1. **Read first.** Without editing, list every code smell you can find in the two source files and what could go wrong because of it. Aim for at least twelve. Then read the tests and note which behaviors surprised you.
2. **Constants.** Replace the `#define` constants and the magic numbers with `constexpr` values and `enum class` types. Replace the function-like macros with functions.
3. **Types and `const`.** Use fixed-width integers, `bool` where a value is true or false, and `const` wherever something is not modified. Replace the C-style casts.
4. **Arrays.** Replace each raw array and its separate length variable with `std::array` or a small fixed-capacity type of your own.
5. **One lead, written once.** Group the per-lead variables into a struct, then remove the duplicated atrial and ventricular code. The two leads still need different limits.
6. **A class instead of globals.** Move the state into a `LeadMonitor` class with a constructor, so that two monitors can exist at once. Keep the old `lm_` functions working as a thin wrapper around one static instance, so the tests still pass untouched. Write one new test that uses two monitors.
7. **RAII for the measurement current.** Replace the manual on and off calls with a guard object whose destructor switches the current off. Check where its scope has to end; one test tells you.
8. **Errors and results.** In the class, replace `LM_ERR` plus `lm_last_error()` with return values that carry the reason (an `enum class`, or `std::optional` for queries). Replace the out-parameters with return values.
9. **The callback.** Replace the function pointer and `void*` context with something type-safe that still needs no heap. Decide between an interface with a virtual function and a template parameter, and write down why.
10. **Retire the old API.** Port the characterization tests to the class one at a time, then delete the wrapper and the macros.

**Check yourself**

- What is a characterization test, and how is it different from a test written from a specification?
- Why is "one kind of change per commit" worth the extra commits? What does it give a reviewer?
- What can go wrong with `#define MAX(a, b)` that cannot go wrong with a function?
- Before your guard class, how many places had to remember to switch the current off? What happens when someone adds a new early `return`?
- What does file-scope `static` state make impossible to test? What did the fixture have to do in `SetUp` because of it?
- An `enum class` and a set of `#define` codes compile to the same numbers. What does the compiler now check for you?
- `std::function` would be the easy answer for the callback. Why is it not allowed here?
- While refactoring you find behavior that looks wrong. What do you do with it, and why not fix it in the same commit?

**Stretch:** compare the size of the object file before and after (`size` or `llvm-size` on the `.o`) and explain any difference. Then add a third lead (a second ventricular lead, as some devices have) and count the lines you had to touch, compared with how many the legacy code would have needed.
