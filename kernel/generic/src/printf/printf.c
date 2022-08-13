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
#include <stdarg.h>

int printf(const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);

	ret = vprintf(fmt, args);

	va_end(args);

	return ret;
}

/** @}
 */
