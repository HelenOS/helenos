/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup riscv64
 * @{
 */
/** @file
 */

#include <arch.h>
#include <stddef.h>
#include <arch/arch.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/drivers/ucb.h>

#include <halt.h>
#include <config.h>
#include <errno.h>
#include <context.h>
#include <fpu_context.h>
#include <interrupt.h>
#include <syscall/copy.h>
#include <ddi/irq.h>
#include <proc/thread.h>
#include <console/console.h>
#include <mem.h>
#include <str.h>

char memcpy_from_uspace_failover_address;
char memcpy_to_uspace_failover_address;

static void riscv64_post_mm_init(void);

arch_ops_t riscv64_ops = {
	.post_mm_init = riscv64_post_mm_init
};

arch_ops_t *arch_ops = &riscv64_ops;

void riscv64_pre_main(bootinfo_t *bootinfo)
{
	physmem_start = bootinfo->physmem_start;
	htif_frame = bootinfo->htif_frame;
	pt_frame = bootinfo->pt_frame;

	htif_init(bootinfo->ucbinfo.tohost, bootinfo->ucbinfo.fromhost);

	/* Copy tasks map. */
	init.cnt = min3(bootinfo->taskmap.cnt, TASKMAP_MAX_RECORDS,
	    CONFIG_INIT_TASKS);

	for (size_t i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo->taskmap.tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->taskmap.tasks[i].name);
	}

	/* Copy physical memory map. */
	memmap.total = bootinfo->memmap.total;
	memmap.cnt = min(bootinfo->memmap.cnt, MEMMAP_MAX_RECORDS);
	for (size_t i = 0; i < memmap.cnt; i++) {
		memmap.zones[i].start = bootinfo->memmap.zones[i].start;
		memmap.zones[i].size = bootinfo->memmap.zones[i].size;
	}
}

void riscv64_post_mm_init(void)
{
	outdev_t *htifout = htifout_init();
	if (htifout)
		stdout_wire(htifout);
}

void calibrate_delay_loop(void)
{
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

errno_t memcpy_from_uspace(void *dst, const void *uspace_src, size_t size)
{
	return EOK;
}

errno_t memcpy_to_uspace(void *uspace_dst, const void *src, size_t size)
{
	return EOK;
}

/** @}
 */
