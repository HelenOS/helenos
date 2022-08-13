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

#include <abi/proc/uarg.h>
#include <userspace.h>

void userspace(uspace_arg_t *kernel_uarg)
{
	// FIXME
	while (true)
		;
}

/** @}
 */
