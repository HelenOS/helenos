/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

/** Print formatted to string.
 *
 * This function is unsafe, thus it is marked as deprecated. It should never
 * be used in native HelenOS code.
 *
 * @param str Buffer to write to
 * @param fmt Format string
 *
 * @return Number of characters printed on success, negative value on failure
 */
int sprintf(char *s, const char *fmt, ...)
{
	int rc;

	va_list args;
	va_start(args, fmt);
	rc = vsnprintf(s, INT_MAX, fmt, args);
	va_end(args);

	return rc;
}

/** @}
 */
