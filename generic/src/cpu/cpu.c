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

#include <cpu.h>
#include <arch.h>
#include <arch/cpu.h>
#include <mm/heap.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <arch/types.h>
#include <config.h>
#include <panic.h>
#include <typedefs.h>
#include <memstr.h>
#include <list.h>

cpu_t *cpus;

/** Initialize CPUs
 *
 * Initialize kernel CPUs support.
 *
 */
void cpu_init(void) {
	int i, j;
	
	#ifdef CONFIG_SMP
	if (config.cpu_active == 1) {
	#endif /* CONFIG_SMP */
		cpus = (cpu_t *) malloc(sizeof(cpu_t) * config.cpu_count);
		if (!cpus)
			panic("malloc/cpus");

		/* initialize everything */
		memsetb((__address) cpus, sizeof(cpu_t) * config.cpu_count, 0);

		for (i=0; i < config.cpu_count; i++) {
			cpus[i].stack = (__u8 *) frame_alloc(FRAME_KA | FRAME_PANIC, ONE_FRAME);
			
			cpus[i].id = i;
			
			spinlock_initialize(&cpus[i].lock, "cpu_t.lock");

			#ifdef CONFIG_SMP
			waitq_initialize(&cpus[i].kcpulb_wq);
			#endif /* __SMP */
			
			for (j = 0; j < RQ_COUNT; j++) {
				spinlock_initialize(&cpus[i].rq[j].lock, "rq_t.lock");
				list_initialize(&cpus[i].rq[j].rq_head);
			}
		}
		
	#ifdef CONFIG_SMP
	}
	#endif /* CONFIG_SMP */

	CPU = &cpus[config.cpu_active-1];
	
	CPU->active = 1;
	CPU->tlb_active = 1;
	
	cpu_identify();
	cpu_arch_init();
}
