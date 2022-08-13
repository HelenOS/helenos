/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_proc
 * @{
 */
/** @file
 */

#include <proc/scheduler.h>
#include <cpu.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <arch/interrupt.h>
#include <arch/pm.h>
#include <arch/asm.h>
#include <arch/ddi/ddi.h>

/** Perform ia32 specific tasks needed before the new task is run.
 *
 * Interrupts are disabled.
 */
void before_task_runs_arch(void)
{
	io_perm_bitmap_install();
}

/** Perform ia32 specific tasks needed before the new thread is scheduled.
 *
 * THREAD is locked and interrupts are disabled.
 */
void before_thread_runs_arch(void)
{
	uintptr_t kstk = (uintptr_t) &THREAD->kstack[STACK_SIZE];

#ifndef PROCESSOR_i486
	if (CPU->arch.fi.bits.sep) {
		/* Set kernel stack for CP3 -> CPL0 switch via SYSENTER */
		write_msr(IA32_MSR_SYSENTER_ESP, kstk - sizeof(istate_t));
	}
#endif

	/* Set kernel stack for CPL3 -> CPL0 switch via interrupt */
	CPU->arch.tss->esp0 = kstk;
	CPU->arch.tss->ss0 = GDT_SELECTOR(KDATA_DES);
}

void after_thread_ran_arch(void)
{
}

/** @}
 */
