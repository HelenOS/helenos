/*
 * SPDX-FileCopyrightText: 2011 Jiri Michalec
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * General functions and structures used in rtl8139 driver
 */

#ifndef RTL8139_GENERAL_H_
#define RTL8139_GENERAL_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/** Number of microseconds in second */
#define RTL8139_USEC_IN_SEC  1000000

/** Structure for HW timer control */
typedef struct {
	/** Register value set in the last timer period */
	uint32_t last_val;

	/** Register value set in the common timer period */
	uint32_t full_val;

	/** Amount of full register periods in timer period */
	size_t full_skips;

	/** Remaining full register periods to the next period end */
	size_t full_skips_remains;

	/** Mark if there is a last run */
	int last_run;
} rtl8139_timer_act_t;

extern void *rtl8139_memcpy_wrapped(void *, const void *, size_t, size_t,
    size_t);
extern errno_t rtl8139_timer_act_init(rtl8139_timer_act_t *, uint32_t,
    const struct timespec *);
extern int rtl8139_timer_act_step(rtl8139_timer_act_t *, uint32_t *);

#endif
