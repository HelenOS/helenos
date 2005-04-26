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

#ifndef __CPU_H__
#define __CPU_H__

#include <arch/cpu.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <proc/scheduler.h>
#include <time/clock.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/context.h>

#define CPU_STACK_SIZE	(4096)

struct cpu {
	spinlock_t lock;
	context_t saved_context;

	volatile int nrdy;
	runq_t rq[RQ_COUNT];
	volatile int needs_relink;

	spinlock_t timeoutlock;
	link_t timeout_active_head;

	#ifdef __SMP__
	int kcpulbstarted;
	waitq_t kcpulb_wq;
	#endif /* __SMP__ */

	int id;
	int active;
	int tlb_active;

	__u16 frequency_mhz;
	__u32 delay_loop_const;

	cpu_arch_t arch;
	
	__u8 *stack;
};

/*
 * read/write by associated CPU
 * read only by other CPUs
 */
struct cpu_private_data {
	thread_t *thread;
	task_t *task;
};

extern cpu_private_data_t *cpu_private_data;
extern cpu_t *cpus;

extern void cpu_init(void);
extern void cpu_arch_init(void);
extern void cpu_identify(void);
extern void cpu_print_report(cpu_t *m);

#endif
