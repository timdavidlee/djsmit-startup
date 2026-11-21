#include "telemetry_task.h"

#include "hw.h"

uint8_t g_params[kParamCount] = {60, 120, 35, 8, 25, 25};

static uint8_t s_state = 0;  // 0 = idle, 1 = session open, 2 = locked out
static uint8_t s_bad_codes = 0;
static uint16_t s_lockout_polls = 0;
static uint16_t s_idle_polls = 0;
static uint8_t s_rx_buf[4] = {0, 0, 0, 0};
static uint8_t s_rx_count = 0;
static uint8_t s_rx_idle = 0;
static uint8_t s_test_active = 0;
static uint16_t s_test_polls = 0;

void telemetry_reset() {
    g_params[0] = 60;
    g_params[1] = 120;
    g_params[2] = 35;
    g_params[3] = 8;
    g_params[4] = 25;
    g_params[5] = 25;
    s_state = 0;
    s_bad_codes = 0;
    s_lockout_polls = 0;
    s_idle_polls = 0;
    s_rx_count = 0;
    s_rx_idle = 0;
    s_test_active = 0;
    s_test_polls = 0;
}

void telemetry_poll() {
    // A lockout runs down whether or not a programmer is in range.
    if (s_state == 2) {
        s_lockout_polls--;
        if (s_lockout_polls == 0) {
            s_state = 0;
            s_bad_codes = 0;
            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
            hw_write_reg(REG_EVENT_CODE, 0x060000);
        }
    }

    // Programmer out of range: an open session ends at once.
    if ((hw_read_reg(REG_TLM_STATUS) & 0x02) == 0) {
        if (s_state == 1) {
            if (s_test_active) {
                hw_write_reg(REG_PACE_OVERRIDE, 0);
                s_test_active = 0;
                hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                hw_write_reg(REG_EVENT_CODE, 0x210200);
            }
            s_state = 0;
            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
            hw_write_reg(REG_EVENT_CODE, 0x070000);
        }
        s_rx_count = 0;
        s_rx_idle = 0;
        return;
    }

    // Test mode runs down.
    if (s_test_active) {
        s_test_polls--;
        if (s_test_polls == 0) {
            hw_write_reg(REG_PACE_OVERRIDE, 0);
            s_test_active = 0;
            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
            hw_write_reg(REG_EVENT_CODE, 0x210100);
        }
    }

    // Take up to 8 bytes per call. Every 4 bytes are one command:
    //   byte 0 = command, byte 1 = first argument, byte 2 = second argument, byte 3 = reserved
    uint8_t bytes_this_poll = 0;
    bool got_command = false;
    while ((hw_read_reg(REG_TLM_STATUS) & 0x01) != 0 && bytes_this_poll < 8) {
        s_rx_buf[s_rx_count] = static_cast<uint8_t>(hw_read_reg(REG_TLM_RX) & 0xFF);
        s_rx_count++;
        bytes_this_poll++;
        s_rx_idle = 0;
        if (s_rx_count < 4) continue;

        s_rx_count = 0;
        s_idle_polls = 0;
        got_command = true;

        const uint8_t cmd = s_rx_buf[0];
        const uint8_t a = s_rx_buf[1];
        const uint8_t b = s_rx_buf[2];

        // Reply: 0x06 = accepted, 0x15 = refused (then d0 is the reason).
        uint8_t reply = 0x06;
        uint8_t d0 = 0;
        uint8_t d1 = 0;

        if (s_state == 2) {
            reply = 0x15;
            d0 = 0x09;  // locked out
        } else {
            switch (cmd) {
                case 0x01:  // open session, a and b are the access code
                    if (s_state == 1) {
                        reply = 0x15;
                        d0 = 0x02;  // wrong state
                    } else if (a == 0x2A && b == 0x5C) {
                        s_state = 1;
                        s_bad_codes = 0;
                        hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                        hw_write_reg(REG_EVENT_CODE, 0x010000);
                    } else {
                        s_bad_codes++;
                        hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                        hw_write_reg(REG_EVENT_CODE, 0x040000u | (uint32_t{s_bad_codes} << 8));
                        if (s_bad_codes >= 3) {
                            s_state = 2;
                            s_lockout_polls = 3000;
                            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                            hw_write_reg(REG_EVENT_CODE, 0x050000);
                        }
                        reply = 0x15;
                        d0 = 0x03;  // access refused
                    }
                    break;

                case 0x02:  // close session
                    if (s_state != 1) {
                        reply = 0x15;
                        d0 = 0x01;  // no session
                    } else {
                        if (s_test_active) {
                            hw_write_reg(REG_PACE_OVERRIDE, 0);
                            s_test_active = 0;
                            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                            hw_write_reg(REG_EVENT_CODE, 0x210200);
                        }
                        s_state = 0;
                        hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                        hw_write_reg(REG_EVENT_CODE, 0x020000);
                    }
                    break;

                case 0x10:  // read parameter a
                    if (s_state != 1) {
                        reply = 0x15;
                        d0 = 0x01;
                    } else if (a >= 6) {
                        reply = 0x15;
                        d0 = 0x04;  // no such parameter
                    } else {
                        d0 = a;
                        d1 = g_params[a];
                    }
                    break;

                case 0x11:  // write parameter a, new value b
                    if (s_state != 1) {
                        reply = 0x15;
                        d0 = 0x01;
                        break;
                    }
                    if (s_test_active) {
                        reply = 0x15;
                        d0 = 0x06;  // busy
                        break;
                    }
                    switch (a) {
                        case 0:  // lower rate
                            if (b < 30 || b > 175) {
                                reply = 0x15;
                                d0 = 0x05;  // out of range
                            } else if (b >= g_params[1]) {
                                reply = 0x15;
                                d0 = 0x07;  // conflicts with another parameter
                            }
                            break;
                        case 1:  // upper rate limit
                            if (b < 80 || b > 180) {
                                reply = 0x15;
                                d0 = 0x05;
                            } else if (b <= g_params[0]) {
                                reply = 0x15;
                                d0 = 0x07;
                            }
                            break;
                        case 2:  // pulse amplitude
                            if (b < 5 || b > 75) {
                                reply = 0x15;
                                d0 = 0x05;
                            }
                            break;
                        case 3:  // pulse width
                            if (b < 2 || b > 30) {
                                reply = 0x15;
                                d0 = 0x05;
                            }
                            break;
                        case 4:  // sensitivity
                            if (b < 5 || b > 100) {
                                reply = 0x15;
                                d0 = 0x05;
                            }
                            break;
                        case 5:  // refractory period
                            if (b < 15 || b > 50) {
                                reply = 0x15;
                                d0 = 0x05;
                            }
                            break;
                        default:
                            reply = 0x15;
                            d0 = 0x04;
                            break;
                    }
                    if (reply == 0x06) {
                        if (g_params[a] != b) {
                            g_params[a] = b;
                            if (a == 2 || a == 3) {
                                hw_write_reg(REG_PULSE_CFG,
                                             (uint32_t{g_params[3]} << 8) | g_params[2]);
                            }
                            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                            hw_write_reg(REG_EVENT_CODE, 0x100000u | (uint32_t{a} << 8) | b);
                        }
                        d0 = a;
                        d1 = b;
                    }
                    break;

                case 0x20:  // enter test mode: pace at a bpm for b seconds
                    if (s_state != 1) {
                        reply = 0x15;
                        d0 = 0x01;
                    } else if (s_test_active) {
                        reply = 0x15;
                        d0 = 0x06;
                    } else if (a < 30 || a > 180) {
                        reply = 0x15;
                        d0 = 0x05;
                    } else if (b < 1 || b > 60) {
                        reply = 0x15;
                        d0 = 0x05;
                    } else {
                        s_test_active = 1;
                        s_test_polls = static_cast<uint16_t>(b * 100);
                        hw_write_reg(REG_PACE_OVERRIDE, a);
                        hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                        hw_write_reg(REG_EVENT_CODE, 0x200000u | (uint32_t{a} << 8) | b);
                        d0 = a;
                        d1 = b;
                    }
                    break;

                case 0x21:  // leave test mode
                    if (s_state != 1) {
                        reply = 0x15;
                        d0 = 0x01;
                    } else if (!s_test_active) {
                        reply = 0x15;
                        d0 = 0x02;
                    } else {
                        hw_write_reg(REG_PACE_OVERRIDE, 0);
                        s_test_active = 0;
                        hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                        hw_write_reg(REG_EVENT_CODE, 0x210000);
                    }
                    break;

                default:
                    reply = 0x15;
                    d0 = 0x08;  // unknown command
                    break;
            }
        }

        hw_write_reg(REG_TLM_TX, reply);
        hw_write_reg(REG_TLM_TX, cmd);
        hw_write_reg(REG_TLM_TX, d0);
        hw_write_reg(REG_TLM_TX, d1);
    }

    // Part of a command arrived and then nothing for 20 calls: throw the part away.
    if (bytes_this_poll == 0 && s_rx_count != 0) {
        s_rx_idle++;
        if (s_rx_idle >= 20) {
            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
            hw_write_reg(REG_EVENT_CODE, 0x300000u | (uint32_t{s_rx_count} << 8));
            s_rx_count = 0;
            s_rx_idle = 0;
        }
    }

    // An open session with no command for 500 calls (5 seconds) is closed.
    if (s_state == 1 && !got_command) {
        s_idle_polls++;
        if (s_idle_polls >= 500) {
            if (s_test_active) {
                hw_write_reg(REG_PACE_OVERRIDE, 0);
                s_test_active = 0;
                hw_write_reg(REG_EVENT_TIME, hw_now_ms());
                hw_write_reg(REG_EVENT_CODE, 0x210200);
            }
            s_state = 0;
            hw_write_reg(REG_EVENT_TIME, hw_now_ms());
            hw_write_reg(REG_EVENT_CODE, 0x030000);
        }
    }
}
