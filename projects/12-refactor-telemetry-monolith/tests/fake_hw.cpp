#include "fake_hw.h"

#include <cstdio>
#include <deque>

#include "hw.h"

namespace {

std::deque<uint8_t> g_rx;
bool g_link = true;
uint32_t g_now_ms = 0;
std::string g_trace;
bool g_last_line_is_tx = false;
uint32_t g_last_line_ms = 0;

void add_line(const char* reg, const char* format, uint32_t value) {
    char text[64];
    std::snprintf(text, sizeof(text), format, static_cast<unsigned>(value));
    g_trace += std::to_string(g_now_ms) + " " + reg + " " + text + "\n";
    g_last_line_is_tx = false;
    g_last_line_ms = g_now_ms;
}

}  // namespace

namespace fake_hw {

void reset() {
    g_rx.clear();
    g_link = true;
    g_now_ms = 0;
    g_trace.clear();
    g_last_line_is_tx = false;
    g_last_line_ms = 0;
}

void push_rx(std::initializer_list<uint8_t> bytes) { g_rx.insert(g_rx.end(), bytes); }

void set_link(bool programmer_in_range) { g_link = programmer_in_range; }

void advance_ms(uint32_t ms) { g_now_ms += ms; }

const std::string& trace() { return g_trace; }

}  // namespace fake_hw

uint32_t hw_read_reg(uint32_t addr) {
    if (addr == REG_TLM_STATUS) {
        return (g_rx.empty() ? 0u : 0x01u) | (g_link ? 0x02u : 0u);
    }
    if (addr == REG_TLM_RX) {
        if (g_rx.empty()) return 0;
        const uint8_t byte = g_rx.front();
        g_rx.pop_front();
        return byte;
    }
    add_line("READ_OF_UNKNOWN_REGISTER", "%08X", addr);
    return 0;
}

void hw_write_reg(uint32_t addr, uint32_t value) {
    if (addr == REG_TLM_TX) {
        if (g_last_line_is_tx && g_last_line_ms == g_now_ms) {
            char text[16];
            std::snprintf(text, sizeof(text), " %02X", static_cast<unsigned>(value));
            g_trace.insert(g_trace.size() - 1, text);  // before the line's '\n'
        } else {
            add_line("TX", "%02X", value);
            g_last_line_is_tx = true;
        }
    } else if (addr == REG_EVENT_TIME) {
        add_line("EVENT_TIME", "%u", value);
    } else if (addr == REG_EVENT_CODE) {
        add_line("EVENT_CODE", "%06X", value);
    } else if (addr == REG_PULSE_CFG) {
        add_line("PULSE_CFG", "%04X", value);
    } else if (addr == REG_PACE_OVERRIDE) {
        add_line("PACE_OVERRIDE", "%u", value);
    } else {
        add_line("WRITE_TO_UNKNOWN_REGISTER", "%08X", addr);
    }
}

uint32_t hw_now_ms() { return g_now_ms; }
