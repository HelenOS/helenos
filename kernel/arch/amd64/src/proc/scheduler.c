/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_proc
 * @{
 */
/** @file
 */

#include <proc/scheduler.h>
#include <cpu.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/asm.h>
#include <arch/pm.h>
#include <arch/ddi/ddi.h>
#include <arch/kseg_struct.h>

/** Perform amd64 specific tasks needed before the new task is run.
 *
 * Interrupts are disabled.
 */
void before_task_runs_arch(void)
{
	io_perm_bitmap_install();
}

/** Perform amd64 specific tasks needed before the new thread is scheduled. */
void before_thread_runs_arch(void)
{
	CPU->arch.tss->rsp0 = (uintptr_t) &THREAD->kstack[STACK_SIZE];

	kseg_t *kseg = (kseg_t *) read_msr(AMD_MSR_GS_KERNEL);
	kseg->kstack_rsp = THREAD->arch.kstack_rsp;
}

void after_thread_ran_arch(void)
{
}

/** @}
 */
