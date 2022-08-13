/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Halt function.
 */

#include <stdbool.h>
#include <halt.h>
#include <log.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <console/kconsole.h>

/** Halt flag */
atomic_bool haltstate = false;

/** Halt wrapper
 *
 * Set halt flag and halt the CPU.
 *
 */
void halt(void)
{
#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	bool rundebugger = false;

	if (!atomic_load(&haltstate)) {
		atomic_store(&haltstate, true);
		rundebugger = true;
	}
#else
	atomic_store(&haltstate, true);
#endif

	interrupts_disable();

#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	if ((rundebugger) && (kconsole_check_poll()))
		kconsole("panic", "\nLast resort kernel console ready.\n", false);
#endif

	if (CPU)
		log(LF_OTHER, LVL_NOTE, "cpu%u: halted", CPU->id);
	else
		log(LF_OTHER, LVL_NOTE, "cpu: halted");

	cpu_halt();
}

/** @}
 */
