/*
 * SPDX-FileCopyrightText: 2007 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Shutdown procedures.
 */

#include <arch.h>
#include <proc/task.h>
#include <halt.h>
#include <log.h>

void reboot(void)
{
	task_done();

#ifdef CONFIG_DEBUG
	log(LF_OTHER, LVL_DEBUG, "Rebooting the system");
#endif

	arch_reboot();
	halt();
}

/** @}
 */
