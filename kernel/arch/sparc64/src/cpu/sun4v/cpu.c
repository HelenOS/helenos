/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <cpu.h>
#include <arch.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/drivers/tick.h>
#include <stdio.h>
#include <arch/sun4v/md.h>
#include <arch/sun4v/hypercall.h>
#include <arch/trap/sun4v/interrupt.h>

/** Perform sparc64 specific initialization of the processor structure for the
 * current processor.
 */
void cpu_arch_init(void)
{
	uint64_t myid;
	__hypercall_fast_ret1(0, 0, 0, 0, 0, CPU_MYID, &myid);

	CPU->arch.id = myid;

	md_node_t node = md_get_root();

	/* walk through MD, find the current CPU node & its clock-frequency */
	while (true) {
		if (!md_next_node(&node, "cpu")) {
			panic("Could not determine CPU frequency.");
		}
		uint64_t id = 0;
		md_get_integer_property(node, "id", &id);

		if (id == myid) {
			uint64_t clock_frequency = 0;
			md_get_integer_property(node, "clock-frequency",
			    &clock_frequency);
			CPU->arch.clock_frequency = clock_frequency;
			break;
		}
	}

	tick_init();

	sun4v_ipi_init();
}

/**
 * Implemented as an empty function as accessing the VER register is
 * a hyperprivileged operation on sun4v.
 */
void cpu_identify(void)
{
}

/** Print version information for a processor.
 *
 * This function is called by the bootstrap processor.
 *
 * @param m Processor structure of the CPU for which version information is to
 * 	be printed.
 */
void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: Niagara (%d MHz)\n", m->id,
	    m->arch.clock_frequency / 1000000);
}

/** @}
 */
