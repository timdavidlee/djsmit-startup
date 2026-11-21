#pragma once
#include <cstdint>

// Hardware access used by the firmware.
//
// On the real device these three functions read and write memory-mapped registers
// and a hardware timer. There is no implementation of them under src/: whatever
// links the firmware has to supply one. On a laptop the tests do (see tests/fake_hw.cpp).

uint32_t hw_read_reg(uint32_t addr);
void hw_write_reg(uint32_t addr, uint32_t value);
uint32_t hw_now_ms();

// Telemetry radio.
constexpr uint32_t REG_TLM_STATUS = 0x40001000;  // read: bit 0 = a received byte is waiting,
                                                 //       bit 1 = a programmer is in range
constexpr uint32_t REG_TLM_RX = 0x40001004;      // read: takes the next received byte
constexpr uint32_t REG_TLM_TX = 0x40001008;      // write: sends one byte

// Event recorder. Write the time first, then the code; the code write stores the entry.
constexpr uint32_t REG_EVENT_TIME = 0x40002000;  // write: time of the event in ms
constexpr uint32_t REG_EVENT_CODE = 0x40002004;  // write: (event << 16) | (detail1 << 8) | detail2

// Pacing output.
constexpr uint32_t REG_PULSE_CFG = 0x40003000;      // write: (pulse width << 8) | pulse amplitude
constexpr uint32_t REG_PACE_OVERRIDE = 0x40003004;  // write: fixed test rate in bpm, 0 = off
