/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_PREEMPTION_H_
#define KERN_PREEMPTION_H_

#include <arch.h>
#include <assert.h>
#include <barrier.h>

#define PREEMPTION_INC         (1 << 0)
#define PREEMPTION_DISABLED    (PREEMPTION_INC <= CURRENT->preemption)
#define PREEMPTION_ENABLED     (!PREEMPTION_DISABLED)

/** Increment preemption disabled counter. */
#define preemption_disable() \
	do { \
		CURRENT->preemption += PREEMPTION_INC; \
		compiler_barrier(); \
	} while (0)

/** Restores preemption but never reschedules. */
#define preemption_enable() \
	do { \
		assert(PREEMPTION_DISABLED); \
		compiler_barrier(); \
		CURRENT->preemption -= PREEMPTION_INC; \
	} while (0)

#endif

/** @}
 */
