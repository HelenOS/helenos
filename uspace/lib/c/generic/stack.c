/*
 * SPDX-FileCopyrightText: 2012 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stack.h>
#include <sysinfo.h>

size_t stack_size_get(void)
{
	static sysarg_t stack_size = 0;

	if (!stack_size)
		sysinfo_get_value("default.stack_size", &stack_size);

	return (size_t) stack_size;
}

/** @}
 */
