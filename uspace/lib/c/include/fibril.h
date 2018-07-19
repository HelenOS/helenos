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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_FIBRIL_H_
#define LIBC_FIBRIL_H_

#include <types/common.h>
#include <time.h>
#include <_bits/__noreturn.h>
#include <ipc/common.h>

typedef struct fibril fibril_t;

typedef struct {
	fibril_t *owned_by;
} fibril_owner_info_t;

typedef fibril_t *fid_t;

typedef struct {
	fibril_t *fibril;
} fibril_event_t;

#define FIBRIL_EVENT_INIT ((fibril_event_t) {0})

/** Fibril-local variable specifier */
#define fibril_local __thread

#define FIBRIL_DFLT_STK_SIZE	0

extern fid_t fibril_create_generic(errno_t (*)(void *), void *, size_t);
extern void fibril_destroy(fid_t);
extern void fibril_add_ready(fid_t);
extern fid_t fibril_get_id(void);
extern void fibril_yield(void);

extern void fibril_usleep(suseconds_t);
extern void fibril_sleep(unsigned int);

extern void fibril_enable_multithreaded(void);
extern int fibril_test_spawn_runners(int);

extern void fibril_detach(fid_t fid);

static inline fid_t fibril_create(errno_t (*func)(void *), void *arg)
{
	return fibril_create_generic(func, arg, FIBRIL_DFLT_STK_SIZE);
}

extern void fibril_start(fid_t);
extern __noreturn void fibril_exit(long);

extern void fibril_wait_for(fibril_event_t *);
extern errno_t fibril_wait_timeout(fibril_event_t *, const struct timeval *);
extern void fibril_notify(fibril_event_t *);

extern errno_t fibril_ipc_wait(ipc_call_t *, const struct timeval *);
extern void fibril_ipc_poke(void);

#endif

/** @}
 */
