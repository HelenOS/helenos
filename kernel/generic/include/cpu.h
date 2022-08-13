/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#define CPU                  CURRENT->cpu

/** CPU structure.
 *
 * There is one structure like this for every processor.
 */
typedef struct cpu {
	IRQ_SPINLOCK_DECLARE(lock);

	tlb_shootdown_msg_t tlb_messages[TLB_MESSAGE_QUEUE_LEN];
	size_t tlb_messages_count;

	context_t saved_context;

	atomic_size_t nrdy;
	runq_t rq[RQ_COUNT];
	volatile size_t needs_relink;

	IRQ_SPINLOCK_DECLARE(timeoutlock);
	list_t timeout_active_list;

	/**
	 * When system clock loses a tick, it is
	 * recorded here so that clock() can react.
	 * This variable is CPU-local and can be
	 * only accessed when interrupts are
	 * disabled.
	 */
	size_t missed_clock_ticks;

	/**
	 * Processor cycle accounting.
	 */
	bool idle;
	uint64_t last_cycle;
	uint64_t idle_cycles;
	uint64_t busy_cycles;

	/**
	 * Processor ID assigned by kernel.
	 */
	unsigned int id;

	bool active;
	volatile bool tlb_active;

	uint16_t frequency_mhz;
	uint32_t delay_loop_const;

	cpu_arch_t arch;

	struct thread *fpu_owner;

	/**
	 * Stack used by scheduler when there is no running thread.
	 */
	uint8_t *stack;
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
