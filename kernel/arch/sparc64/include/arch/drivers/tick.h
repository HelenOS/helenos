/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TICK_H_
#define KERN_sparc64_TICK_H_

#include <arch/asm.h>
#include <arch/interrupt.h>

/* mask of the "counter" field of the Tick register */
#define TICK_COUNTER_MASK  (~(1l << 63))

extern void tick_init(void);
extern void tick_interrupt(unsigned int, istate_t *);

/**
 * Reads the Tick register counter.
 */
static inline uint64_t tick_counter_read(void)
{
	return TICK_COUNTER_MASK & tick_read();
}

#endif

/** @}
 */
