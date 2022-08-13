/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CYCLE_H_
#define KERN_mips32_CYCLE_H_

#include <arch/cp0.h>
#include <arch/interrupt.h>
#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	return ((uint64_t) count_hi << 32) + ((uint64_t) cp0_count_read());
}

#endif

/** @}
 */
