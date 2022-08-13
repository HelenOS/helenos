/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief CPU subsystem initialization and listing.
 */

#include <cpu.h>
#include <arch.h>
#include <arch/cpu.h>
#include <stdlib.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <typedefs.h>
#include <config.h>
#include <panic.h>
#include <mem.h>
#include <adt/list.h>
#include <stdio.h>
#include <sysinfo/sysinfo.h>
#include <arch/cycle.h>

cpu_t *cpus;

/** Initialize CPUs
 *
 * Initialize kernel CPUs support.
 *
 */
void cpu_init(void)
{
#ifdef CONFIG_SMP
	if (config.cpu_active == 1) {
#endif /* CONFIG_SMP */

		cpus = (cpu_t *) malloc(sizeof(cpu_t) * config.cpu_count);
		if (!cpus)
			panic("Cannot allocate CPU structures.");

		/* Initialize everything */
		memsetb(cpus, sizeof(cpu_t) * config.cpu_count, 0);

		/*
		 * NOTE: All kernel stacks must be aligned to STACK_SIZE,
		 *       see CURRENT.
		 */
		for (size_t i = 0; i < config.cpu_count; i++) {
			uintptr_t stack_phys = frame_alloc(STACK_FRAMES,
			    FRAME_LOWMEM | FRAME_ATOMIC, STACK_SIZE - 1);
			if (!stack_phys)
				panic("Cannot allocate CPU stack.");

			cpus[i].stack = (uint8_t *) PA2KA(stack_phys);
			cpus[i].id = i;

			irq_spinlock_initialize(&cpus[i].lock, "cpus[].lock");

			for (unsigned int j = 0; j < RQ_COUNT; j++) {
				irq_spinlock_initialize(&cpus[i].rq[j].lock, "cpus[].rq[].lock");
				list_initialize(&cpus[i].rq[j].rq);
			}
		}

#ifdef CONFIG_SMP
	}
#endif /* CONFIG_SMP */

	CPU = &cpus[config.cpu_active - 1];

	CPU->active = true;
	CPU->tlb_active = true;

	CPU->idle = false;
	CPU->last_cycle = get_cycle();
	CPU->idle_cycles = 0;
	CPU->busy_cycles = 0;

	cpu_identify();
	cpu_arch_init();
}

/** List all processors. */
void cpu_list(void)
{
	unsigned int i;

	for (i = 0; i < config.cpu_count; i++) {
		if (cpus[i].active)
			cpu_print_report(&cpus[i]);
		else
			printf("cpu%u: not active\n", i);
	}
}

/** @}
 */
