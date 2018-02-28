/*
 * Copyright (c) 2005 Jakub Jermar
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

#include <arch.h>
#include <arch/arch.h>
#include <debug.h>
#include <config.h>
#include <macros.h>
#include <arch/trap/trap.h>
#include <arch/console.h>
#include <arch/sun4v/md.h>
#include <console/console.h>
#include <arch/boot/boot.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/mm/page.h>
#include <arch/stack.h>
#include <interrupt.h>
#include <genarch/ofw/ofw_tree.h>
#include <userspace.h>
#include <ddi/irq.h>
#include <str.h>
#include <arch/drivers/niagara.h>
#include <sysinfo/sysinfo.h>

static void sun4v_pre_mm_init(void);
static void sun4v_post_mm_init(void);
static void sun4v_post_smp_init(void);

arch_ops_t sun4v_ops = {
	.pre_mm_init = sun4v_pre_mm_init,
	.post_mm_init = sun4v_post_mm_init,
	.post_smp_init = sun4v_post_smp_init,
};

arch_ops_t *sparc64_ops = &sun4v_ops;

memmap_t memmap;

/** Perform sparc64-specific initialization before main_bsp() is called. */
void sparc64_pre_main(bootinfo_t *bootinfo)
{
	/* Copy init task info. */
	init.cnt = min3(bootinfo->taskmap.cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);

	size_t i;
	for (i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo->taskmap.tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->taskmap.tasks[i].name);
	}

	/* Copy physical memory map. */
	memmap.total = bootinfo->memmap.total;
	memmap.cnt = min(bootinfo->memmap.cnt, MEMMAP_MAX_RECORDS);
	for (i = 0; i < memmap.cnt; i++) {
		memmap.zones[i].start = bootinfo->memmap.zones[i].start;
		memmap.zones[i].size = bootinfo->memmap.zones[i].size;
	}

	md_init();
}

/** Perform sparc64 specific initialization before mm is initialized. */
void sun4v_pre_mm_init(void)
{
	if (config.cpu_active == 1) {
		trap_init();
		exc_arch_init();
	}
}

/** Perform sparc64 specific initialization afterr mm is initialized. */
void sun4v_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Map OFW information into sysinfo */
		ofw_sysinfo_map();

		/*
		 * We have 2^11 different interrupt vectors.
		 * But we only create 128 buckets.
		 */
		irq_init(1 << 11, 128);
	}
}

void sun4v_post_smp_init(void)
{
	/* Currently the only supported platform for sparc64/sun4v is 'sun4v'. */
	static const char *platform = "sun4v";

	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));

	niagarain_init();
}

/** Calibrate delay loop.
 *
 * On sparc64, we implement delay() by waiting for the TICK register to
 * reach a pre-computed value, as opposed to performing some pre-computed
 * amount of instructions of known duration. We set the delay_loop_const
 * to 1 in order to neutralize the multiplication done by delay().
 */
void calibrate_delay_loop(void)
{
	CPU->delay_loop_const = 1;
}

/** Wait several microseconds.
 *
 * We assume that interrupts are already disabled.
 *
 * @param t Microseconds to wait.
 */
void asm_delay_loop(const uint32_t usec)
{
	uint64_t stop = tick_read() + (uint64_t) usec * (uint64_t)
	    CPU->arch.clock_frequency / 1000000;

	while (tick_read() < stop)
		;
}

/** Switch to userspace. */
void userspace(uspace_arg_t *kernel_uarg)
{
	(void) interrupts_disable();
	switch_to_userspace((uintptr_t) kernel_uarg->uspace_entry,
	    ((uintptr_t) kernel_uarg->uspace_stack) +
	    kernel_uarg->uspace_stack_size -
	    (ALIGN_UP(STACK_ITEM_SIZE, STACK_ALIGNMENT) + STACK_BIAS),
	    (uintptr_t) kernel_uarg->uspace_uarg);

	/* Not reached */
	while (1);
}

void arch_reboot(void)
{
	// TODO
	while (1);
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

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

/** @}
 */
