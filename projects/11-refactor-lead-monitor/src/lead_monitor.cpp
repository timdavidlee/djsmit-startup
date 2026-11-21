/*
 * lead_monitor.cpp - lead impedance monitor
 *
 * Ported from the C code base. Do not change behaviour without a test.
 */
#include "lead_monitor.h"

#include <stddef.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define RAW_TO_OHMS(r) ((unsigned short)((r) * 4))
#define CHECK_INIT()               \
    do {                           \
        if (!g_init) {             \
            g_err = LM_E_NOT_INIT; \
            return LM_ERR;         \
        }                          \
    } while (0)

static int g_init = 0;
static int g_err = 0;
static int g_cur = 0; /* measurement current */
static unsigned long g_pulses = 0;
static lm_alert_cb g_cb = NULL;
static void* g_ctx = NULL;

/* atrial lead */
static unsigned short a_hist[HISTORY_SIZE];
static int a_n = 0;
static unsigned short a_last = 0;
static unsigned short a_min = 0;
static unsigned short a_max = 0;
static int a_status = 0;
static int a_alert = 0;
static unsigned int a_count = 0;
static unsigned int a_rej = 0;

/* ventricular lead */
static unsigned short v_hist[HISTORY_SIZE];
static int v_n = 0;
static unsigned short v_last = 0;
static unsigned short v_min = 0;
static unsigned short v_max = 0;
static int v_status = 0;
static int v_alert = 0;
static unsigned int v_count = 0;
static unsigned int v_rej = 0;

static void current_on(void) {
    g_cur = 1;
    g_pulses++;
}

static void current_off(void) { g_cur = 0; }

int lm_init(lm_alert_cb cb, void* ctx) {
    if (g_init) {
        g_err = LM_E_ALREADY_INIT;
        return LM_ERR;
    }

    memset(a_hist, 0, sizeof(a_hist));
    a_n = 0;
    a_last = 0;
    a_min = 0;
    a_max = 0;
    a_status = 0;
    a_alert = 0;
    a_count = 0;
    a_rej = 0;

    memset(v_hist, 0, sizeof(v_hist));
    v_n = 0;
    v_last = 0;
    v_min = 0;
    v_max = 0;
    v_status = 0;
    v_alert = 0;
    v_count = 0;
    v_rej = 0;

    g_cb = cb;
    g_ctx = ctx;
    g_cur = 0;
    g_pulses = 0;
    g_err = LM_E_NONE;
    g_init = 1;
    return LM_OK;
}

void lm_shutdown(void) {
    if (!g_init) return;
    current_off();
    g_cb = NULL;
    g_ctx = NULL;
    g_err = LM_E_NONE;
    g_init = 0;
}

int lm_measure(int lead, unsigned short raw1, unsigned short raw2, unsigned short raw3) {
    unsigned short lo, hi, med, ohms;
    int flags = 0;
    int fire = 0;

    CHECK_INIT();
    if (lead != LEAD_ATRIAL && lead != LEAD_VENTRICULAR) {
        g_err = LM_E_BAD_LEAD;
        return LM_ERR;
    }

    current_on();

    /* 10 bit ADC */
    if (raw1 > 1023 || raw2 > 1023 || raw3 > 1023) {
        if (lead == LEAD_ATRIAL)
            a_rej++;
        else
            v_rej++;
        current_off();
        g_err = LM_E_BAD_RAW;
        return LM_ERR;
    }

    /* the three readings have to agree */
    lo = (unsigned short)MIN(MIN(raw1, raw2), raw3);
    hi = (unsigned short)MAX(MAX(raw1, raw2), raw3);
    if (hi - lo > 40) {
        if (lead == LEAD_ATRIAL)
            a_rej++;
        else
            v_rej++;
        current_off();
        g_err = LM_E_NOISY;
        return LM_ERR;
    }

    /* middle one of the three */
    med = (unsigned short)MAX(MIN(raw1, raw2), MIN(MAX(raw1, raw2), raw3));
    ohms = RAW_TO_OHMS(med);

    if (lead == LEAD_ATRIAL) {
        if (ohms < 200) {
            a_status = LEAD_LOW;
            flags |= ALERT_LOW;
        } else if (ohms > 2000) {
            a_status = LEAD_HIGH;
            flags |= ALERT_HIGH;
        } else {
            a_status = LEAD_OK;
        }

        if (a_count > 0) {
            if ((ohms > a_last ? ohms - a_last : a_last - ohms) > 500) flags |= ALERT_JUMP;
            if (ohms < a_min) a_min = ohms;
            if (ohms > a_max) a_max = ohms;
        } else {
            a_min = ohms;
            a_max = ohms;
        }

        /* newest result goes in front */
        memmove(&a_hist[1], &a_hist[0], (HISTORY_SIZE - 1) * sizeof(unsigned short));
        a_hist[0] = ohms;
        if (a_n < HISTORY_SIZE) a_n++;

        a_last = ohms;
        a_count++;

        if (flags != 0 && a_alert == 0) {
            a_alert = flags;
            fire = 1;
        }
    } else {
        if (ohms < 300) {
            v_status = LEAD_LOW;
            flags |= ALERT_LOW;
        } else if (ohms > 3000) {
            v_status = LEAD_HIGH;
            flags |= ALERT_HIGH;
        } else {
            v_status = LEAD_OK;
        }

        if (v_count > 0) {
            if ((ohms > v_last ? ohms - v_last : v_last - ohms) > 500) flags |= ALERT_JUMP;
            if (ohms < v_min) v_min = ohms;
            if (ohms > v_max) v_max = ohms;
        } else {
            v_min = ohms;
            v_max = ohms;
        }

        /* newest result goes in front */
        memmove(&v_hist[1], &v_hist[0], (HISTORY_SIZE - 1) * sizeof(unsigned short));
        v_hist[0] = ohms;
        if (v_n < HISTORY_SIZE) v_n++;

        v_last = ohms;
        v_count++;

        if (flags != 0 && v_alert == 0) {
            v_alert = flags;
            fire = 1;
        }
    }

    current_off();
    g_err = LM_E_NONE;

    if (fire && g_cb != NULL) g_cb(g_ctx, lead, flags, ohms);
    return LM_OK;
}

int lm_measure_many(int lead, unsigned short* raws, int n_raws) {
    int i;
    int ok = 0;

    CHECK_INIT();
    if (lead != LEAD_ATRIAL && lead != LEAD_VENTRICULAR) {
        g_err = LM_E_BAD_LEAD;
        return LM_ERR;
    }
    if (raws == NULL || n_raws < 0 || n_raws % 3 != 0) {
        g_err = LM_E_BAD_ARG;
        return LM_ERR;
    }

    g_err = LM_E_NONE;
    for (i = 0; i < n_raws; i += 3) {
        if (lm_measure(lead, raws[i], raws[i + 1], raws[i + 2]) == LM_OK) ok++;
    }
    return ok;
}

int lm_get_info(int lead, lead_info_t* out) {
    lead_info_t tmp;

    CHECK_INIT();
    if (out == NULL) {
        g_err = LM_E_BAD_ARG;
        return LM_ERR;
    }

    memset(&tmp, 0, sizeof(tmp));
    if (lead == LEAD_ATRIAL) {
        tmp.last_ohms = a_last;
        tmp.min_ohms = a_min;
        tmp.max_ohms = a_max;
        tmp.status = a_status;
        tmp.alert = a_alert;
        tmp.count = a_count;
        tmp.rejects = a_rej;
    } else if (lead == LEAD_VENTRICULAR) {
        tmp.last_ohms = v_last;
        tmp.min_ohms = v_min;
        tmp.max_ohms = v_max;
        tmp.status = v_status;
        tmp.alert = v_alert;
        tmp.count = v_count;
        tmp.rejects = v_rej;
    } else {
        g_err = LM_E_BAD_LEAD;
        return LM_ERR;
    }

    memcpy(out, &tmp, sizeof(tmp));
    g_err = LM_E_NONE;
    return LM_OK;
}

int lm_get_history(int lead, unsigned short* buf, int buf_len, int* out_count) {
    int n;

    CHECK_INIT();
    if (buf == NULL || out_count == NULL || buf_len < 0) {
        g_err = LM_E_BAD_ARG;
        return LM_ERR;
    }

    if (lead == LEAD_ATRIAL) {
        n = MIN(a_n, buf_len);
        memcpy(buf, a_hist, (size_t)n * sizeof(unsigned short));
    } else if (lead == LEAD_VENTRICULAR) {
        n = MIN(v_n, buf_len);
        memcpy(buf, v_hist, (size_t)n * sizeof(unsigned short));
    } else {
        g_err = LM_E_BAD_LEAD;
        return LM_ERR;
    }

    *out_count = n;
    g_err = LM_E_NONE;
    return LM_OK;
}

int lm_clear_alert(int lead) {
    CHECK_INIT();
    if (lead == LEAD_ATRIAL) {
        a_alert = 0;
    } else if (lead == LEAD_VENTRICULAR) {
        v_alert = 0;
    } else {
        g_err = LM_E_BAD_LEAD;
        return LM_ERR;
    }
    g_err = LM_E_NONE;
    return LM_OK;
}

int lm_current_is_on(void) { return g_cur; }

unsigned long lm_pulse_count(void) { return g_pulses; }

int lm_last_error(void) { return g_err; }
