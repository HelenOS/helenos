/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup time
 * @{
 */
/** @file
 */

#ifndef KERN_TIMEOUT_H_
#define KERN_TIMEOUT_H_

#include <adt/list.h>
#include <cpu.h>
#include <stdint.h>

typedef void (* timeout_handler_t)(void *arg);

typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);

	/** Link to the list of active timeouts on THE->cpu */
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
