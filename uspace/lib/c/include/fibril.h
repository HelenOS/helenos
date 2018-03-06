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

#include <context.h>
#include <types/common.h>
#include <adt/list.h>
#include <libarch/tls.h>

#define FIBRIL_WRITER	1

struct fibril;

typedef struct {
	struct fibril *owned_by;
} fibril_owner_info_t;

typedef enum {
	FIBRIL_PREEMPT,
	FIBRIL_TO_MANAGER,
	FIBRIL_FROM_MANAGER,
	FIBRIL_FROM_DEAD
} fibril_switch_type_t;

typedef sysarg_t fid_t;

typedef struct fibril {
	link_t link;
	link_t all_link;
	context_t ctx;
	void *stack;
	void *arg;
	errno_t (*func)(void *);
	tcb_t *tcb;

	struct fibril *clean_after_me;
	errno_t retval;
	int flags;

	fibril_owner_info_t *waits_for;

	unsigned int switches;
} fibril_t;

/** Fibril-local variable specifier */
#define fibril_local __thread

#define FIBRIL_DFLT_STK_SIZE	0

extern fid_t fibril_create_generic(errno_t (*func)(void *), void *arg, size_t);
extern void fibril_destroy(fid_t fid);
extern fibril_t *fibril_setup(void);
extern void fibril_teardown(fibril_t *f, bool locked);
extern int fibril_switch(fibril_switch_type_t stype);
extern void fibril_add_ready(fid_t fid);
extern void fibril_add_manager(fid_t fid);
extern void fibril_remove_manager(void);
extern fid_t fibril_get_id(void);

static inline fid_t fibril_create(errno_t (*func)(void *), void *arg)
{
	return fibril_create_generic(func, arg, FIBRIL_DFLT_STK_SIZE);
}

static inline int fibril_yield(void)
{
	return fibril_switch(FIBRIL_PREEMPT);
}

#endif

/** @}
 */
