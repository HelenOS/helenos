/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_time
 * @{
 */
/** @file
 */

#ifndef KERN_TIMEOUT_H_
#define KERN_TIMEOUT_H_

#include <adt/list.h>
#include <cpu.h>
#include <stdint.h>

typedef void (*timeout_handler_t)(void *arg);

typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);

	/** Link to the list of active timeouts on CURRENT->cpu */
	link_t link;
	/** Timeout will be activated in this amount of clock() ticks. */
	uint64_t ticks;
	/** Function that will be called on timeout activation. */
	timeout_handler_t handler;
	/** Argument to be passed to handler() function. */
	void *arg;
	/** On which processor is this timeout registered. */
	cpu_t *cpu;
} timeout_t;

#define us2ticks(us)  ((uint64_t) (((uint32_t) (us) / (1000000 / HZ))))

extern void timeout_init(void);
extern void timeout_initialize(timeout_t *);
extern void timeout_reinitialize(timeout_t *);
extern void timeout_register(timeout_t *, uint64_t, timeout_handler_t, void *);
extern bool timeout_unregister(timeout_t *);

#endif

/** @}
 */
