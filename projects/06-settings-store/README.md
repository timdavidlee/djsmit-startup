# Project 6: Power-loss-safe settings store

Build a module that saves device settings to simulated flash memory and always loads either the old settings or the new ones, never a corrupt mix, even if power is cut mid-write. This is the last build project and the bridge to testing: half the work is injecting failures.

*Medical context:* a clinician sets a pacemaker's **programmed parameters** through the programmer. Three typical ones are the pacing rate, the **pulse amplitude** (the voltage of each pacing pulse) and the **pulse width** (how long each pulse lasts, in fractions of a millisecond). They must survive a device reset, and a device that cannot read valid settings must fall back to known safe defaults.

**Starting point**

```cpp
struct Settings {
    uint16_t pacing_rate_bpm;      // 30..200
    uint16_t pulse_amplitude_mv;   // 500..7500
    uint16_t pulse_width_us;       // 100..1500
};

struct Flash {                     // interface the store receives by reference
    virtual bool read(uint32_t addr, uint8_t* out, uint32_t len) = 0;
    virtual bool write(uint32_t addr, const uint8_t* in, uint32_t len) = 0;
    virtual bool erase_page(uint32_t page) = 0;
};
```

**Tasks**

1. Write `FakeFlash`: two 256-byte pages in a `std::array`. Model real flash: erase sets every byte to `0xFF`, and a write can only clear bits (`stored &= value`).
2. Define the on-flash record: magic number, format version, sequence counter, the settings, and the CRC-16 from Project 3. Serialize it field by field. Add `static_assert(std::is_trivially_copyable_v<Settings>)`.
3. Implement `save(const Settings&)`: erase the page that does not hold the current record, write the new record there with sequence + 1.
4. Implement `load()`: read both pages, discard any with a bad magic, version or CRC, return the one with the higher sequence. If neither is valid, return the safe defaults and report that it did so.
5. Validate ranges in `save` and again in `load`. Explain why both.
6. **Fault injection.** Give `FakeFlash` a "fail after N bytes written" switch. For every N from 0 to the record size, run `save`, cut power at byte N, then `load`, and assert the result is exactly the old settings or exactly the new ones.
7. Add a version 2 record with one new field, and make `load` upgrade a version 1 record.

**Check yourself**

- Why two pages? What goes wrong with a single page that is erased and rewritten?
- What does "trivially copyable" mean, and why does it matter for bytes that go to storage?
- What happens when the sequence counter wraps?
- Flash pages wear out after a limited number of erases. How does the A/B scheme affect wear?
- The fault-injection loop is a test no one would write by hand for one value of N. What does that say about where bugs hide?

**Stretch:** inject single-bit flips into a stored record and confirm `load` rejects it and falls back to the other page.
