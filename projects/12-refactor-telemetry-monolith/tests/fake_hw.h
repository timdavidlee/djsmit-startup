#pragma once
#include <cstdint>
#include <initializer_list>
#include <string>

// Laptop stand-in for the hardware behind src/hw.h.
//
// fake_hw.cpp defines hw_read_reg(), hw_write_reg() and hw_now_ms(), so the firmware
// links against this instead of real registers. It keeps its state in globals, because
// the firmware calls plain functions and there is nowhere to pass an object in.
namespace fake_hw {

// Empty receive queue, programmer in range, clock at 0, empty trace.
void reset();

// Bytes the programmer sends. The firmware takes them one at a time from REG_TLM_RX.
void push_rx(std::initializer_list<uint8_t> bytes);

void set_link(bool programmer_in_range);

void advance_ms(uint32_t ms);

// Every register write so far, one line each: "<time in ms> <register> <value>".
// EVENT_TIME (ms) and PACE_OVERRIDE (beats per minute) are decimal, everything else
// is hex. Bytes sent to REG_TLM_TX at the same time share one line.
const std::string& trace();

}  // namespace fake_hw
