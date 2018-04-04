/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#include <cpu.h>
#include <arch.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/drivers/tick.h>
#include <print.h>
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
