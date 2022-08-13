/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_proc
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <arch/boot/boot.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <arch.h>

/** Perform ppc32 specific tasks needed before the new task is run.
 *
 */
void before_task_runs_arch(void)
{
}

/** Perform ppc32 specific tasks needed before the new thread is scheduled.
 *
 */
void before_thread_runs_arch(void)
{
	tlb_invalidate_all();

	asm volatile (
	    "mtsprg0 %[ksp]\n"
	    :: [ksp] "r" (KA2PA(&THREAD->kstack[STACK_SIZE]))
	);
}

void after_thread_ran_arch(void)
{
}

/** @}
 */
