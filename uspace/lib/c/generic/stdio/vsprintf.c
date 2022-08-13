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

/** Print formatted to string with arguments passed as va_list.
 *
 * This function is unsafe, thus it is marked as deprecated. It should never
 * be used in native HelenOS code.
 *
 * @param str Buffer to write to
 * @param fmt Format string
 * @param ap Arguments
 *
 * @return Number of characters printed on success, negative value on failure
 */
int vsprintf(char *s, const char *fmt, va_list ap)
{
	return vsnprintf(s, INT_MAX, fmt, ap);
}

/** @}
 */
