/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <print.h>
#include <printf/printf_core.h>

int snprintf(char *str, size_t size, const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);
	ret = vsnprintf(str, size, fmt, args);

	va_end(args);

	return ret;
}

/** @}
 */
