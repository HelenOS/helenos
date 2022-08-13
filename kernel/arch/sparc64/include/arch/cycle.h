/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CYCLE_H_
#define KERN_sparc64_CYCLE_H_

#include <arch/asm.h>
#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	return tick_read();
}

#endif

/** @}
 */
