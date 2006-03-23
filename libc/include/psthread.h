/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#ifndef __LIBC__PSTHREAD_H__
#define __LIBC__PSTHREAD_H__

#include <libarch/psthread.h>
#include <libadt/list.h>

#ifndef context_set
#define context_set(c, _pc, stack, size, ptls) 	\
	(c)->pc = (sysarg_t) (_pc);		\
	(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; \
        (c)->tls = (sysarg_t) (ptls);
#endif /* context_set */

typedef sysarg_t pstid_t;

struct psthread_data {
	struct psthread_data *self; /* IA32,AMD64 needs to get self address */

	link_t list;
	context_t ctx;
	void *stack;
	void *arg;
	int (*func)(void *);

	struct psthread_data *waiter;
	int finished;
	int retval;
	int flags;
};
typedef struct psthread_data psthread_data_t;

extern int context_save(context_t *c);
extern void context_restore(context_t *c) __attribute__ ((noreturn));

pstid_t psthread_create(int (*func)(void *), void *arg);
int ps_preempt(void);
int ps_join(pstid_t psthrid);

#endif
