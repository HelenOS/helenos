/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
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

uintptr_t memcpy_from_uspace(void *dst, uspace_addr_t uspace_src, size_t size)
{
	return 0;
}

uintptr_t memcpy_to_uspace(uspace_addr_t uspace_dst, const void *src, size_t size)
{
	return 0;
}

/** @}
 */
