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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief CPU subsystem initialization and listing.
 */

#include <cpu.h>
#include <arch.h>
#include <arch/cpu.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <typedefs.h>
#include <config.h>
#include <panic.h>
#include <mem.h>
#include <adt/list.h>
#include <print.h>
#include <sysinfo/sysinfo.h>
#include <arch/cycle.h>
#include <synch/rcu.h>

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

		cpus = (cpu_t *) malloc(sizeof(cpu_t) * config.cpu_count,
		    FRAME_ATOMIC);
		if (!cpus)
			panic("Cannot allocate CPU structures.");

		/* Initialize everything */
		memsetb(cpus, sizeof(cpu_t) * config.cpu_count, 0);

		size_t i;
		for (i = 0; i < config.cpu_count; i++) {
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
	rcu_cpu_init();
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
