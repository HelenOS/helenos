/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup sparc32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <typedefs.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/machine_func.h>
#include <func.h>
#include <config.h>
#include <errno.h>
#include <context.h>
#include <fpu_context.h>
#include <interrupt.h>
#include <syscall/copy.h>
#include <ddi/irq.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <macros.h>
#include <memstr.h>
#include <str.h>

char memcpy_from_uspace_failover_address;
char memcpy_to_uspace_failover_address;

static bootinfo_t machine_bootinfo;

void arch_pre_main(void *unused, bootinfo_t *bootinfo)
{
	init.cnt = min3(bootinfo->cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);
	memcpy(&machine_bootinfo, bootinfo, sizeof(machine_bootinfo));
	
	for (size_t i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->tasks[i].addr);
		init.tasks[i].size = bootinfo->tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->tasks[i].name);
	}
	
	machine_ops_init();
}

void arch_pre_mm_init(void)
{
}

extern void func1(void);

void arch_post_mm_init(void)
{
	machine_init(&machine_bootinfo);
	
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(16, 16);
		
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
	
	machine_output_init();
}


void arch_post_cpu_init()
{
}

void arch_pre_smp_init(void)
{
}

void arch_post_smp_init(void)
{
	machine_input_init();
}

void calibrate_delay_loop(void)
{
}

sysarg_t sys_tls_set(uintptr_t addr)
{
	return EOK;
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void arch_reboot(void)
{
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

void istate_decode(istate_t *istate)
{
	(void) istate;
}

void fpu_init(void)
{
}

void fpu_context_save(fpu_context_t *ctx)
{
}

void fpu_context_restore(fpu_context_t *ctx)
{
}

int memcpy_from_uspace(void *dst, const void *uspace_src, size_t size)
{
	memcpy(dst, uspace_src, size);
	return 1;
}

int memcpy_to_uspace(void *uspace_dst, const void *src, size_t size)
{
	memcpy(uspace_dst, src, size);
	return 1;
}

/** @}
 */
