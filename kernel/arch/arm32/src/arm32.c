/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief ARM32 architecture specific functions.
 */

#include <arch.h>
#include <arch/arch.h>
#include <config.h>
#include <genarch/fb/fb.h>
#include <abi/fb/visuals.h>
#include <console/console.h>
#include <ddi/irq.h>
#include <config.h>
#include <interrupt.h>
#include <arch/regutils.h>
#include <arch/machine_func.h>
#include <userspace.h>
#include <macros.h>
#include <str.h>
#include <arch/ras.h>
#include <sysinfo/sysinfo.h>

static void arm32_pre_mm_init(void);
static void arm32_post_mm_init(void);
static void arm32_post_smp_init(void);

arch_ops_t arm32_ops = {
	.pre_mm_init = arm32_pre_mm_init,
	.post_mm_init = arm32_post_mm_init,
	.post_smp_init = arm32_post_smp_init,
};

arch_ops_t *arch_ops = &arm32_ops;

/** Performs arm32-specific initialization before main_bsp() is called. */
void arm32_pre_main(void *entry __attribute__((unused)), bootinfo_t *bootinfo)
{
	init.cnt = min3(bootinfo->taskmap.cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);

	size_t i;
	for (i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo->taskmap.tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->taskmap.tasks[i].name);
	}

	/* Initialize machine_ops pointer. */
	machine_ops_init();
}

/** Performs arm32 specific initialization before mm is initialized. */
void arm32_pre_mm_init(void)
{
	/* It is not assumed by default */
	interrupts_disable();
}

/** Performs arm32 specific initialization afterr mm is initialized. */
void arm32_post_mm_init(void)
{
	machine_init();

	/* Initialize exception dispatch table */
	exception_init();
	interrupt_init();

	/* Initialize Restartable Atomic Sequences support. */
	ras_init();

	machine_output_init();
}

/** Performs arm32 specific tasks needed after the multiprocessing is
 * initialized.
 *
 * Currently the function is empty because SMP is not supported.
 */
void arm32_post_smp_init(void)
{
	machine_input_init();
	const char *platform = machine_get_platform_name();

	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));
}

/** Performs arm32 specific tasks needed before the new task is run. */
void before_task_runs_arch(void)
{
}

/** Performs arm32 specific tasks needed before the new thread is scheduled.
 *
 * It sets supervisor_sp.
 */
void before_thread_runs_arch(void)
{
	uint8_t *stck;

	stck = &THREAD->kstack[STACK_SIZE];
	supervisor_sp = (uintptr_t) stck;
}

/** Performs arm32 specific tasks before a thread stops running.
 *
 * Currently the function is empty.
 */
void after_thread_ran_arch(void)
{
}

/** Halts CPU. */
void cpu_halt(void)
{
	while (true)
		machine_cpu_halt();
}

/** Reboot. */
void arch_reboot(void)
{
	/* not implemented */
	while (true)
		;
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
