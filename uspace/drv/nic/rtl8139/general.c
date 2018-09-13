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
 *  General functions and structures used in rtl8139 driver
 */

#include "general.h"

#include <mem.h>
#include <errno.h>
#include <stdint.h>

/** Copy block of the memory from wrapped source buffer.
 *
 * Start on the specific offset in the source buffer and * copy data_size bytes
 * - continue from the buffer start after getting the end
 *
 * @param dest        The destination memory
 * @param src_begin   The begin of the source buffer
 * @param src_offset  The offset in the source buffer to start copy
 * @param src_size    The source buffer size
 * @param data_size   The amount of data to copy
 *
 * @return NULL if the error occures, dest if succeed
 */
void *rtl8139_memcpy_wrapped(void *dest, const void *src, size_t src_offset,
    size_t src_size, size_t data_size)
{
	src_offset %= src_size;
	if (data_size > src_size)
		return NULL;

	size_t to_src_end = src_size - src_offset;
	if (data_size <= to_src_end) {
		return memcpy(dest, src + src_offset, data_size);
	}

	size_t rem_size = data_size - to_src_end;

	/*
	 * First copy the end part of the data (from the source begining),
	 * then copy the begining
	 */
	memcpy(dest + to_src_end, src, rem_size);
	return memcpy(dest, src + src_offset, to_src_end);
}

/** Initialize the timer register structures
 *
 *  The structure will be initialized to the state that the first call of
 *  rtl8139_timer_act_step function will be the period expiration
 *
 *  @param ta          The timer structure
 *  @param timer_freq  The timer frequency in kHz
 *  @param time        The requested time
 *
 *  @return EOK if succeed, error code otherwise
 */
errno_t rtl8139_timer_act_init(rtl8139_timer_act_t *ta, uint32_t timer_freq,
    const struct timespec *time)
{
	if (!ta || timer_freq == 0 || !time)
		return EINVAL;
	memset(ta, 0, sizeof(rtl8139_timer_act_t));

	uint32_t tics_per_ms = timer_freq;
	uint32_t seconds_in_reg = UINT32_MAX / (tics_per_ms * 1000);
	ta->full_val = seconds_in_reg * tics_per_ms * 1000;

	struct timespec remains = *time;
	ta->full_skips = remains.tv_sec / seconds_in_reg;
	remains.tv_sec = remains.tv_sec % seconds_in_reg;

	if (NSEC2USEC(remains.tv_nsec) > RTL8139_USEC_IN_SEC) {
		remains.tv_sec += NSEC2USEC(remains.tv_nsec) / RTL8139_USEC_IN_SEC;
		remains.tv_nsec = NSEC2USEC(remains.tv_nsec) % RTL8139_USEC_IN_SEC;

		/* it can be increased above seconds_in_reg again */
		ta->full_skips += remains.tv_sec / seconds_in_reg;
		remains.tv_sec = remains.tv_sec % seconds_in_reg;
	}

	ta->last_val = SEC2MSEC(remains.tv_sec) + NSEC2MSEC(remains.tv_nsec);
	ta->last_val *= tics_per_ms;

	/* Force inital setting in the next step */
	ta->full_skips_remains = 0;
	ta->last_run = 1;
	return EOK;
}

/** Make one step timer step
 *
 *  @param ta            Timer structure
 *  @param new_reg[out]  Register value to set
 *
 *  @return Nonzero if whole period expired, zero if part of period expired
 */
int rtl8139_timer_act_step(rtl8139_timer_act_t *ta, uint32_t *new_reg)
{
	uint32_t next_val = 0;
	int expired = 0;

	if (ta->last_run || (ta->last_val == 0 && ta->full_skips_remains == 0)) {
		ta->full_skips_remains = ta->full_skips;
		ta->last_run = 0;
		expired = 1;
	}

	if (ta->full_skips_remains > 0) {
		next_val = ta->full_val;
		ta->full_skips_remains--;
	} else {
		next_val = ta->last_val;
		ta->last_run = 1;
	}

	if (new_reg)
		*new_reg = next_val;

	return expired;
}
