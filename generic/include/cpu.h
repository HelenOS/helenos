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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef __CPU_H__
#define __CPU_H__

#include <arch/cpu.h>
#include <proc/scheduler.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/context.h>
#include <config.h>
#include <adt/list.h>
#include <mm/tlb.h>

#define CPU_STACK_SIZE	STACK_SIZE

/** CPU structure.
 *
 * There is one structure like this for every processor.
 */
struct cpu {
	SPINLOCK_DECLARE(lock);

	tlb_shootdown_msg_t tlb_messages[TLB_MESSAGE_QUEUE_LEN];
	count_t tlb_messages_count;
	
	context_t saved_context;

	atomic_t nrdy;
	runq_t rq[RQ_COUNT];
	volatile count_t needs_relink;

	SPINLOCK_DECLARE(timeoutlock);
	link_t timeout_active_head;

	count_t missed_clock_ticks;	/**< When system clock loses a tick, it is recorded here
					     so that clock() can react. This variable is
					     CPU-local and can be only accessed when interrupts
					     are disabled. */

	/**
	 * Processor ID assigned by kernel.
	 */
	int id;
	
	int active;
	int tlb_active;

	__u16 frequency_mhz;
	__u32 delay_loop_const;

	cpu_arch_t arch;

	thread_t *fpu_owner;
	
	/**
	 * Stack used by scheduler when there is no running thread.
	 */
	__u8 *stack;
};

extern cpu_t *cpus;

extern void cpu_init(void);
extern void cpu_list(void);

extern void cpu_arch_init(void);
extern void cpu_identify(void);
extern void cpu_print_report(cpu_t *m);

#endif

/** @}
 */
