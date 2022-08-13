/*
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <arch/sparc64.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <stdint.h>
#include <debug.h>
#include <arch/asm.h>
#include <barrier.h>
#include <log.h>
#include <arch.h>
#include <mm/tlb.h>
#include <config.h>
#include <synch/spinlock.h>
#include <arch/sun4v/hypercall.h>

/** number of uint64_t-s in one CPU mondo message */
#define CPU_MONDO_ENTRY_SIZE	8

/** number of entries (messages) in the CPU mondo queue */
#define CPU_MONDO_NENTRIES	8

/** number of uint64_t-s in the CPU mondo queue */
#define CPU_MONDO_QUEUE_SIZE	((CPU_MONDO_NENTRIES) * (CPU_MONDO_ENTRY_SIZE))

/** used to identify CPU mondo queue in the hypercall */
#define CPU_MONDO_QUEUE_ID	0x3c

/** ASI for reading/writing CPU mondo head/tail registers */
#define ASI_QUEUE		0x25

/** VA for reading the CPU mondo tail */
#define VA_CPU_MONDO_QUEUE_TAIL	0x3c8

/** VA for reading/writing the CPU mondo head */
#define VA_CPU_MONDO_QUEUE_HEAD	0x3c0

/**
 * array which contains CPU mondo queue for every CPU
 */
uint64_t cpu_mondo_queues[MAX_NUM_STRANDS][CPU_MONDO_QUEUE_SIZE]
    __attribute__((aligned(CPU_MONDO_QUEUE_SIZE * sizeof(uint64_t))));

/**
 * Initializes CPU mondo queue for the current CPU.
 */
void sun4v_ipi_init(void)
{
	if (__hypercall_fast3(
	    CPU_QCONF,
	    CPU_MONDO_QUEUE_ID,
	    KA2PA(cpu_mondo_queues[CPU->id]),
	    CPU_MONDO_NENTRIES) != HV_EOK)
		panic("Initializing mondo queue failed on CPU %" PRIu64 ".\n",
		    CPU->arch.id);
}

/**
 * Handler of the CPU Mondo trap. Reads the message queue, updates the head
 * register and processes the message (invokes a function call).
 */
void cpu_mondo(unsigned int tt, istate_t *istate)
{
#ifdef CONFIG_SMP
	unsigned int tail = asi_u64_read(ASI_QUEUE, VA_CPU_MONDO_QUEUE_TAIL);
	unsigned int head = asi_u64_read(ASI_QUEUE, VA_CPU_MONDO_QUEUE_HEAD);

	while (head != tail) {
		uint64_t data1 = cpu_mondo_queues[CPU->id][0];

		head = (head + CPU_MONDO_ENTRY_SIZE * sizeof(uint64_t)) %
		    (CPU_MONDO_QUEUE_SIZE * sizeof(uint64_t));
		asi_u64_write(ASI_QUEUE, VA_CPU_MONDO_QUEUE_HEAD, head);

		if (data1 == (uintptr_t) tlb_shootdown_ipi_recv) {
			((void (*)(void)) data1)();
		} else {
			log(LF_ARCH, LVL_DEBUG, "Spurious interrupt on %" PRIu64
			    ", data = %" PRIx64 ".", CPU->arch.id, data1);
		}
	}
#endif
}

/** @}
 */
