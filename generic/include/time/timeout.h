/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __TIMEOUT_H__
#define __TIMEOUT_H__

#include <arch/types.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <adt/list.h>

#define us2ticks(us)	((__u64)(((__u32) (us)/(1000000/HZ))))

typedef void (* timeout_handler_t)(void *arg);

struct timeout {
	SPINLOCK_DECLARE(lock);

	link_t link;			/**< Link to the list of active timeouts on THE->cpu */
	
	__u64 ticks;			/**< Timeout will be activated in this amount of clock() ticks. */

	timeout_handler_t handler;	/**< Function that will be called on timeout activation. */
	void *arg;			/**< Argument to be passed to handler() function. */
	
	cpu_t *cpu;			/**< On which processor is this timeout registered. */
};

extern void timeout_init(void);
extern void timeout_initialize(timeout_t *t);
extern void timeout_reinitialize(timeout_t *t);
extern void timeout_register(timeout_t *t, __u64 usec, timeout_handler_t f, void *arg);
extern bool timeout_unregister(timeout_t *t);

#endif

/** @}
 */
