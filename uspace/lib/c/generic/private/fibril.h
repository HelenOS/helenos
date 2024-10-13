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

#ifndef _LIBC_PRIVATE_FIBRIL_H_
#define _LIBC_PRIVATE_FIBRIL_H_

#include <adt/list.h>
#include <context.h>
#include <tls.h>
#include <fibril.h>
#include <ipc/common.h>

#include "./futex.h"

typedef struct {
	fibril_t *fibril;
} fibril_event_t;

#define FIBRIL_EVENT_INIT ((fibril_event_t) {0})

struct fibril {
	// XXX: The first two fields must not move (for taskdump).
	link_t all_link;
	context_t ctx;

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
	int rmutex_locks;
	fibril_owner_info_t *waits_for;
	fibril_event_t *sleep_event;
};

extern fibril_t *fibril_alloc(void);
extern void fibril_setup(fibril_t *);
extern void fibril_teardown(fibril_t *f);
extern fibril_t *fibril_self(void);

extern void __fibrils_init(void);
extern void __fibrils_fini(void);

extern void fibril_wait_for(fibril_event_t *);
extern errno_t fibril_wait_timeout(fibril_event_t *, const struct timespec *);
extern void fibril_notify(fibril_event_t *);

extern errno_t fibril_ipc_wait(ipc_call_t *, const struct timespec *);
extern void fibril_ipc_poke(void);

/**
 * "Restricted" fibril mutex.
 *
 * Similar to `fibril_mutex_t`, but has a set of restrictions placed on its
 * use. Within a rmutex critical section, you
 *         - may not use any other synchronization primitive,
 *           save for another `fibril_rmutex_t`. This includes nonblocking
 *           operations like cvar signal and mutex unlock, unless otherwise
 *           specified.
 *         - may not read IPC messages
 *         - may not start a new thread/fibril
 *           (creating fibril without starting is fine)
 *
 * Additionally, locking with a timeout is not possible on this mutex,
 * and there is no associated condition variable type.
 * This is a design constraint, not a lack of implementation effort.
 */
typedef struct {
	// TODO: At this point, this is just silly handwaving to hide current
	//       futex use behind a fibril based abstraction. Later, the imple-
	//       mentation will change, but the restrictions placed on this type
	//       will allow it to be simpler and faster than a regular mutex.
	//       There might also be optional debug checking of the assumptions.
	//
	//       Note that a consequence of the restrictions is that if we are
	//       running on a single thread, no other fibril can ever get to run
	//       while a fibril has a rmutex locked. That means that for
	//       single-threaded programs, we can reduce all rmutex locks and
	//       unlocks to simple branches on a global bool variable.

	futex_t futex;
} fibril_rmutex_t;

extern errno_t fibril_rmutex_initialize(fibril_rmutex_t *);
extern void fibril_rmutex_destroy(fibril_rmutex_t *);
extern void fibril_rmutex_lock(fibril_rmutex_t *);
extern bool fibril_rmutex_trylock(fibril_rmutex_t *);
extern void fibril_rmutex_unlock(fibril_rmutex_t *);

#endif
