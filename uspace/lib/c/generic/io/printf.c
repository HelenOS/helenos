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

#include <io/printf_core.h>
#include <stdio.h>

/** Print formatted text.
 *
 * @param stream Output stream
 * @param fmt    Format string
 *
 * \see For more details about format string see printf_core.
 *
 */
int fprintf(FILE *stream, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int ret = vfprintf(stream, fmt, args);

	va_end(args);

	return ret;
}

/** Print formatted text to stdout.
 *
 * @param fmt Format string
 *
 * \see For more details about format string see printf_core.
 *
 */
int printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int ret = vprintf(fmt, args);

	va_end(args);

	return ret;
}

/** @}
 */
