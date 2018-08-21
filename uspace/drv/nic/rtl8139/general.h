/*
 * Copyright (c) 2011 Jiri Michalec
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
