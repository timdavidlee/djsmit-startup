#include "command.h"

namespace {

constexpr uint8_t kPing = static_cast<uint8_t>(CommandId::Ping);
constexpr uint8_t kSetRate = static_cast<uint8_t>(CommandId::SetRate);
constexpr uint8_t kSetOutput = static_cast<uint8_t>(CommandId::SetOutput);
constexpr uint8_t kReadEvents = static_cast<uint8_t>(CommandId::ReadEvents);

// Payload length of each command, including the id byte.
constexpr uint8_t kPingLen = 1;
constexpr uint8_t kSetRateLen = 3;
constexpr uint8_t kSetOutputLen = 5;
constexpr uint8_t kReadEventsLen = 3;

constexpr uint16_t kRateMinBpm = 30;
constexpr uint16_t kRateMaxBpm = 200;
constexpr uint16_t kAmplitudeMinMv = 500;
constexpr uint16_t kAmplitudeMaxMv = 7500;
constexpr uint16_t kWidthMinUs = 100;
constexpr uint16_t kWidthMaxUs = 1500;
constexpr uint8_t kEventLogSize = 16;

void put_u16(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value & 0xFFu);
    p[1] = static_cast<uint8_t>(value >> 8);
}

uint16_t get_u16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

bool rate_in_range(uint16_t bpm) { return bpm >= kRateMinBpm && bpm <= kRateMaxBpm; }

bool output_in_range(uint16_t amplitude_mv, uint16_t width_us) {
    return amplitude_mv >= kAmplitudeMinMv && amplitude_mv <= kAmplitudeMaxMv &&
           width_us >= kWidthMinUs && width_us <= kWidthMaxUs;
}

bool events_in_range(uint8_t first, uint8_t count) {
    return first < kEventLogSize && count >= 1 && first + count <= kEventLogSize;
}

}  // namespace

bool encode_command(const Command& cmd, Packet& out) {
    Packet p{};
    p.payload[0] = static_cast<uint8_t>(cmd.id);

    switch (cmd.id) {
        case CommandId::Ping:
            p.len = kPingLen;
            break;

        case CommandId::SetRate:
            if (!rate_in_range(cmd.rate_bpm)) return false;
            put_u16(&p.payload[1], cmd.rate_bpm);
            p.len = kSetRateLen;
            break;

        case CommandId::SetOutput:
            if (!output_in_range(cmd.amplitude_mv, cmd.width_us)) return false;
            put_u16(&p.payload[1], cmd.amplitude_mv);
            put_u16(&p.payload[3], cmd.width_us);
            p.len = kSetOutputLen;
            break;

        case CommandId::ReadEvents:
            if (!events_in_range(cmd.first_event, cmd.event_count)) return false;
            p.payload[1] = cmd.first_event;
            p.payload[2] = cmd.event_count;
            p.len = kReadEventsLen;
            break;

        default:
            return false;
    }

    out = p;
    return true;
}

bool decode_command(const Packet& in, Command& out) {
    if (in.len == 0) return false;

    Command cmd{};
    switch (in.payload[0]) {
        case kPing:
            if (in.len != kPingLen) return false;
            cmd.id = CommandId::Ping;
            break;

        case kSetRate:
            if (in.len != kSetRateLen) return false;
            cmd.id = CommandId::SetRate;
            cmd.rate_bpm = get_u16(&in.payload[1]);
            if (!rate_in_range(cmd.rate_bpm)) return false;
            break;

        case kSetOutput:
            if (in.len != kSetOutputLen) return false;
            cmd.id = CommandId::SetOutput;
            cmd.amplitude_mv = get_u16(&in.payload[1]);
            cmd.width_us = get_u16(&in.payload[3]);
            if (!output_in_range(cmd.amplitude_mv, cmd.width_us)) return false;
            break;

        case kReadEvents:
            if (in.len != kReadEventsLen) return false;
            cmd.id = CommandId::ReadEvents;
            cmd.first_event = in.payload[1];
            cmd.event_count = in.payload[2];
            if (cmd.first_event >= kEventLogSize || cmd.event_count < 1 ||
                cmd.event_count > kEventLogSize) {
                return false;
            }
            break;

        default:
            return false;
    }

    out = cmd;
    return true;
}
