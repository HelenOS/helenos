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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch/thread.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <arch/context.h>
#include <fpu_context.h>
#include <arch/types.h>
#include <typedefs.h>
#include <time/timeout.h>
#include <synch/rwlock.h>
#include <mm/page.h>
#include <list.h>

#define THREAD_STACK_SIZE	PAGE_SIZE

#define THREAD_USER_STACK	1

enum state {
	Invalid,
	Running,
	Sleeping,
	Ready,
	Entering,
	Exiting
};

extern char *thread_states[];

#define X_WIRED		(1<<0)
#define X_STOLEN	(1<<1)

struct thread {
	link_t rq_link;				/* run queue link */
	link_t wq_link;				/* wait queue link */
	link_t th_link;				/* links to threads within the parent task*/
	link_t threads_link;
	
	/* items below are protected by lock */
	spinlock_t lock;

	void (* thread_code)(void *);
	void *thread_arg;

	context_t saved_context;
	context_t sleep_timeout_context;
	fpu_context_t saved_fpu_context;	               
	int fpu_context_exists;	               
	int fpu_context_engaged;               /* Defined only if thread doesn't run. It means that fpu context is in CPU 
						that last time executes this thread. This disables migration */          
	
	
	waitq_t *sleep_queue;
	timeout_t sleep_timeout;
	volatile int timeout_pending;

	rwlock_type_t rwlock_holder_type;
	void (* call_me)(void *);
	void *call_me_with;

	int state;
	int flags;
	
	cpu_t *cpu;
	task_t *task;

	__u64 ticks;

	__u32 tid;
	
	int pri;
	
	ARCH_THREAD_DATA;

	__u8 *kstack;
	__u8 *ustack;
};

extern spinlock_t threads_lock;
extern link_t threads_head;

static void cushion(void);

extern void thread_init(void);
extern thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags);
extern void thread_ready(thread_t *t);
extern void thread_exit(void);

extern void thread_sleep(__u32 sec);
extern void thread_usleep(__u32 usec);

extern void thread_register_call_me(void (* call_me)(void *), void *call_me_with);

#endif
