/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#ifndef LIBC_PRIVATE_FIBRIL_H_
#define LIBC_PRIVATE_FIBRIL_H_

#include <adt/list.h>
#include <context.h>
#include <tls.h>
#include <abi/proc/uarg.h>
#include <atomic.h>
#include <futex.h>

struct fibril {
	// XXX: The first two fields must not move (for taskdump).
	link_t all_link;
	context_t ctx;

	uspace_arg_t uarg;
	link_t link;
	void *stack;
	size_t stack_size;
	void *arg;
	errno_t (*func)(void *);
	tcb_t *tcb;

	fibril_t *clean_after_me;
	errno_t retval;

	fibril_t *thread_ctx;

	bool is_running : 1;
	bool is_writer : 1;
	/* In some places, we use fibril structs that can't be freed. */
	bool is_freeable : 1;

	/* Debugging stuff. */
	atomic_t futex_locks;
	fibril_owner_info_t *waits_for;
	fibril_event_t *sleep_event;
};

extern fibril_t *fibril_alloc(void);
extern void fibril_setup(fibril_t *);
extern void fibril_teardown(fibril_t *f);
extern fibril_t *fibril_self(void);

extern void __fibrils_init(void);

#endif
