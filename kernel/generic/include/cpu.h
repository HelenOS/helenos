/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_CPU_H_
#define KERN_CPU_H_

#include <mm/tlb.h>
#include <synch/spinlock.h>
#include <proc/scheduler.h>
#include <arch/cpu.h>
#include <arch/context.h>
#include <adt/list.h>
#include <arch.h>

#define CPU                  (CURRENT->cpu)
#define CPU_LOCAL            (&CPU->local)

/**
 * Contents of CPU_LOCAL. These are variables that are only ever accessed by
 * the CPU they belong to, so they don't need any synchronization,
 * just locally disabled interrupts.
 */
typedef struct cpu_local {
	/**
	 * When system clock loses a tick, it is
	 * recorded here so that clock() can react.
	 */
	size_t missed_clock_ticks;

	uint64_t current_clock_tick;
	uint64_t preempt_deadline;  /* < when should the currently running thread be preempted */
	uint64_t relink_deadline;

	/**
	 * Stack used by scheduler when there is no running thread.
	 * This field is unchanged after initialization.
	 */
	uint8_t *stack;

	/**
	 * Processor cycle accounting.
	 */
	bool idle;
	uint64_t last_cycle;

	context_t scheduler_context;

	struct thread *prev_thread;
} cpu_local_t;

/** CPU structure.
 *
 * There is one structure like this for every processor.
 */
typedef struct cpu {
	IRQ_SPINLOCK_DECLARE(tlb_lock);

	tlb_shootdown_msg_t tlb_messages[TLB_MESSAGE_QUEUE_LEN];
	size_t tlb_messages_count;

	atomic_size_t nrdy;
	runq_t rq[RQ_COUNT];

	IRQ_SPINLOCK_DECLARE(timeoutlock);
	list_t timeout_active_list;

	/**
	 * Processor cycle accounting.
	 */
	atomic_time_stat_t idle_cycles;
	atomic_time_stat_t busy_cycles;

	/**
	 * Processor ID assigned by kernel.
	 */
	unsigned int id;

	bool active;
	volatile bool tlb_active;

	uint16_t frequency_mhz;
	uint32_t delay_loop_const;

	cpu_arch_t arch;

#ifdef CONFIG_FPU_LAZY
	/* For synchronization between FPU trap and thread destructor. */
	IRQ_SPINLOCK_DECLARE(fpu_lock);
#endif
	_Atomic(struct thread *) fpu_owner;

	cpu_local_t local;
} cpu_t;

extern cpu_t *cpus;

extern void cpu_init(void);
extern void cpu_list(void);

extern void cpu_arch_init(void);
extern void cpu_identify(void);
extern void cpu_print_report(cpu_t *);

#endif

/** @}
 */
