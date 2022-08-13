/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ia64_PAL_H_
#define BOOT_ia64_PAL_H_

#include <arch/types.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Essential PAL procedures' IDs
 */
#define PAL_FREQ_RATIOS		14

extern uint64_t pal_proc_freq_ratio(void);

#define pal_static_call_0_1(id, ret1) \
	pal_static_call((id), 0, 0, 0, (ret1), NULL, NULL)

extern uint64_t pal_static_call(uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t *, uint64_t *, uint64_t *);

#endif
