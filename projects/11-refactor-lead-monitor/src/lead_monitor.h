#ifndef LEAD_MONITOR_H
#define LEAD_MONITOR_H

/*
 * lead_monitor.h - lead impedance monitor
 *
 * Usage: lm_init() once, then lm_measure() whenever a measurement is due.
 * The alert callback must not call lm_measure().
 */

#define LEAD_ATRIAL 0
#define LEAD_VENTRICULAR 1

/* return values */
#define LM_OK 0
#define LM_ERR (-1)

/* reason for the last LM_ERR, see lm_last_error() */
#define LM_E_NONE 0
#define LM_E_NOT_INIT 1
#define LM_E_ALREADY_INIT 2
#define LM_E_BAD_LEAD 3
#define LM_E_BAD_RAW 4
#define LM_E_NOISY 5
#define LM_E_BAD_ARG 6

/* lead status */
#define LEAD_UNKNOWN 0
#define LEAD_OK 1
#define LEAD_LOW 2
#define LEAD_HIGH 3

/* alert flags, can be OR-ed together */
#define ALERT_LOW 0x01
#define ALERT_HIGH 0x02
#define ALERT_JUMP 0x04

#define HISTORY_SIZE 6

typedef void (*lm_alert_cb)(void* ctx, int lead, int flags, unsigned short ohms);

typedef struct {
    unsigned short last_ohms;
    unsigned short min_ohms;
    unsigned short max_ohms;
    int status;           /* LEAD_xxx */
    int alert;            /* 0 = no alert, otherwise ALERT_xxx flags */
    unsigned int count;   /* accepted measurements */
    unsigned int rejects; /* rejected measurements */
} lead_info_t;

int lm_init(lm_alert_cb cb, void* ctx);
void lm_shutdown(void);

/* One measurement from three raw ADC readings (0..1023 each). */
int lm_measure(int lead, unsigned short raw1, unsigned short raw2, unsigned short raw3);

/* n_raws / 3 measurements in a row. Returns how many were accepted, or LM_ERR. */
int lm_measure_many(int lead, unsigned short* raws, int n_raws);

int lm_get_info(int lead, lead_info_t* out);

/* Copies up to buf_len results into buf, newest first. */
int lm_get_history(int lead, unsigned short* buf, int buf_len, int* out_count);

int lm_clear_alert(int lead);

int lm_current_is_on(void);         /* 1 while the measurement current is on */
unsigned long lm_pulse_count(void); /* times the current was switched on since lm_init */
int lm_last_error(void);

#endif
