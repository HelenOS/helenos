/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_CYCLE_H_
#define KERN_ia64_CYCLE_H_

#include <trace.h>

_NO_TRACE static inline uint64_t get_cycle(void)
{
	return 0;
}

#endif

/** @}
 */
