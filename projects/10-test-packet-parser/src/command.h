#pragma once
#include <cstdint>

#include "packet.h"

// Telemetry link, layer 3: the commands a programmer can send, carried in a packet payload.
// See the project README for the specification.

enum class CommandId : uint8_t {
    Ping = 0x01,
    SetRate = 0x10,
    SetOutput = 0x11,
    ReadEvents = 0x20,
};

struct Command {
    CommandId id;
    uint16_t rate_bpm;      // SetRate
    uint16_t amplitude_mv;  // SetOutput
    uint16_t width_us;      // SetOutput
    uint8_t first_event;    // ReadEvents
    uint8_t event_count;    // ReadEvents
};

// Writes cmd into out as a packet payload. Returns false if cmd cannot be sent.
bool encode_command(const Command& cmd, Packet& out);

// Reads a command from a received packet. Returns false if the packet is not a valid command.
bool decode_command(const Packet& in, Command& out);
