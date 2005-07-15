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


cpu_private_data_t *cpu_private_data;
cpu_t *cpus;


/** Initialize CPUs
 *
 * Initialize kernel CPUs support.
 *
 */
void cpu_init(void) {
	int i, j;
	
	#ifdef __SMP__
	if (config.cpu_active == 1) {
	#endif /* __SMP__ */
		cpu_private_data = (cpu_private_data_t *) malloc(sizeof(cpu_private_data_t) * config.cpu_count);
		if (!cpu_private_data)
			panic("malloc/cpu_private_data");

		cpus = (cpu_t *) malloc(sizeof(cpu_t) * config.cpu_count);
		if (!cpus)
			panic("malloc/cpus");

		/* initialize everything */
		memsetb((__address) cpu_private_data, sizeof(cpu_private_data_t) * config.cpu_count, 0);
		memsetb((__address) cpus, sizeof(cpu_t) * config.cpu_count, 0);

		for (i=0; i < config.cpu_count; i++) {
			cpus[i].stack = (__u8 *) malloc(CPU_STACK_SIZE);
			if (!cpus[i].stack)
				panic("malloc/cpus[%d].stack\n", i);
			
			cpus[i].id = i;
			
			#ifdef __SMP__
			waitq_initialize(&cpus[i].kcpulb_wq);
			#endif /* __SMP */
			
			for (j = 0; j < RQ_COUNT; j++) {
				list_initialize(&cpus[i].rq[j].rq_head);
			}
		}
		
	#ifdef __SMP__
	}
	#endif /* __SMP__ */
	
	CPU->active = 1;
	CPU->tlb_active = 1;
	
	cpu_identify();
	cpu_arch_init();
}
