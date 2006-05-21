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
#include <libarch/thread.h>

#ifndef context_set
#define context_set(c, _pc, stack, size, ptls) 			\
	(c)->pc = (sysarg_t) (_pc);				\
	(c)->sp = ((sysarg_t) (stack)) + (size) - SP_DELTA; 	\
        (c)->tls = (sysarg_t) (ptls);
#endif /* context_set */

typedef enum {
	PS_TO_MANAGER,
	PS_FROM_MANAGER,
	PS_PREEMPT
} pschange_type;

typedef sysarg_t pstid_t;

struct psthread_data {
	link_t link;
	context_t ctx;
	void *stack;
	void *arg;
	int (*func)(void *);
	tcb_t *tcb;

	struct psthread_data *waiter;
	int finished;
	int retval;
	int flags;
};
typedef struct psthread_data psthread_data_t;

extern int context_save(context_t *c);
extern void context_restore(context_t *c) __attribute__ ((noreturn));

pstid_t psthread_create(int (*func)(void *), void *arg);
int psthread_join(pstid_t psthrid);
psthread_data_t * psthread_setup(void);
void psthread_teardown(psthread_data_t *pt);
int psthread_schedule_next_adv(pschange_type ctype);
void psthread_add_ready(pstid_t ptid);
void psthread_add_manager(pstid_t psthrid);
void psthread_remove_manager(void);

static inline int psthread_schedule_next() {
	return psthread_schedule_next_adv(PS_PREEMPT);
}


#endif
