/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_time
 * @{
 */

/**
 * @file
 * @brief	Active delay function.
 */

#include <time/delay.h>
#include <proc/thread.h>
#include <typedefs.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>

/** Delay the execution for the given number of microseconds (or slightly more).
 *
 * The delay is implemented as active delay loop.
 *
 * @param usec Number of microseconds to sleep.
 */
void delay(uint32_t usec)
{
	/*
	 * The delay loop is calibrated for each and every CPU in the system.
	 * If running in a thread context, it is therefore necessary to disable
	 * thread migration. We want to do this in a lightweight manner.
	 */
	if (THREAD)
		thread_migration_disable();
	asm_delay_loop(usec * CPU->delay_loop_const);
	if (THREAD)
		thread_migration_enable();
}

/** @}
 */
