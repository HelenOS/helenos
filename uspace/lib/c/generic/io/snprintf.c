/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdarg.h>
#include <stdio.h>

/** Print formatted to the given buffer with limited size.
 *
 * @param str  Buffer
 * @param size Buffer size
 * @param fmt  Format string
 *
 * \see For more details about format string see printf_core.
 *
 */
int snprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int ret = vsnprintf(str, size, fmt, args);

	va_end(args);

	return ret;
}

/** @}
 */
