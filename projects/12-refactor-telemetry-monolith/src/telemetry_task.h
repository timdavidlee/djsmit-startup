#pragma once
#include <cstdint>

// Programmer command handler. See the project README for the command set.

constexpr uint8_t kParamCount = 6;

// Programmed parameters, indexed by parameter id. The pacing code reads these.
//   0  lower rate          beats per minute
//   1  upper rate limit    beats per minute
//   2  pulse amplitude     units of 0.1 V
//   3  pulse width         units of 0.05 ms
//   4  sensitivity         units of 0.1 mV
//   5  refractory period   units of 10 ms
extern uint8_t g_params[kParamCount];

// Call once every 10 ms from the main loop.
void telemetry_poll();

// Puts the handler and the parameters back in their power-on state.
void telemetry_reset();
