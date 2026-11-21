# Project 3: Register and telemetry layer

Build a simulated hardware register block and a binary status frame with a checksum. Firmware talks to hardware by reading and writing individual bits at fixed addresses, and talks to the outside world in tightly packed bytes, so this project is all bit-level work.

*Medical context:* an implanted pacemaker reports its status wirelessly to a clinic device called a programmer. That link is called **telemetry**. Two values it typically reports are battery voltage and **lead impedance**, the electrical resistance of the wire (the **lead**) running from the device into the heart. An impedance far outside the normal range suggests a damaged or dislodged lead.

**Part A: registers**

```cpp
struct DeviceRegs {
    volatile uint32_t CTRL;    // bit 0: enable, bits 1-3: mode, bit 7: reset
    volatile uint32_t STATUS;  // bit 0: ready, bit 1: fault, bits 8-15: error code
    volatile uint32_t DATA;
};
```

1. Write `set_bit`, `clear_bit`, `test_bit`, `write_field(reg, shift, width, value)` and `read_field`. Make them `constexpr` where possible and test them with `static_assert`.
2. Wrap the block in a `Device` class that takes a `DeviceRegs&`. In tests, pass an ordinary struct instance; on real hardware it would be `*reinterpret_cast<DeviceRegs*>(0x40001000)`.
3. Replace magic numbers with `enum class Mode : uint8_t` and named `constexpr` masks.

**Part B: telemetry frame**

| Field | Type | Notes |
| --- | --- | --- |
| Sync | `uint8_t` | Always `0xA5` |
| Sequence | `uint8_t` | Increments per frame, wraps at 255 |
| Battery | `uint16_t` | Millivolts |
| Lead impedance | `uint16_t` | Ohms |
| Heart rate | `uint8_t` | Beats per minute |
| Flags | `uint8_t` | Bit 0: fault, bit 1: low battery |
| CRC | `uint16_t` | CRC-16/CCITT-FALSE over all preceding bytes |

4. Write `serialize(const Frame&, uint8_t* out)` and `bool parse(const uint8_t* in, Frame& out)`. Write each field byte by byte in little-endian order with shifts. Do not `memcpy` the struct.
5. Implement the CRC bitwise first (polynomial `0x1021`, initial value `0xFFFF`). The CRC of the ASCII string `"123456789"` must be `0x29B1`. Then write a 256-entry table version built at compile time with `constexpr`.
6. Test that flipping any single bit in a frame makes `parse` return false.
7. Print `sizeof(Frame)` for the naive struct, then reorder the fields and watch it change.

**Check yourself**

- What does `volatile` tell the compiler, and what bug appears at `-O2` without it?
- Why is `memcpy` of a struct onto the wire unsafe across compilers and CPUs? Name both problems (padding and byte order).
- What is the difference between `>>` on a signed and an unsigned value?
- Why are C++ bitfields (`uint32_t mode : 3;`) usually avoided for hardware registers?
- Flash versus RAM: where does the `constexpr` CRC table end up, and why does that matter on a device with a few kilobytes of RAM?

**Stretch:** write a byte-at-a-time parser that resynchronizes on `0xA5` after garbage, as a real serial receiver must.
