/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le
 * @{
 */
/** @file
 */

#include <userspace.h>
#include <stdbool.h>
#include <arch.h>
#include <abi/proc/uarg.h>
#include <mm/as.h>

void userspace(uspace_arg_t *kernel_uarg)
{
	/*
	 * On real hardware this switches the CPU to user
	 * space mode and jumps to kernel_uarg->uspace_entry.
	 */

	while (true)
		;
}

/** @}
 */
