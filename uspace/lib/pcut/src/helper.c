/*
 * SPDX-FileCopyrightText: 2018 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * Helper functions.
 */

#include "internal.h"

#pragma warning(push, 0)
#include <stdarg.h>
#include <stdio.h>
#pragma warning(pop)

/*
 * It seems that not all Unixes are the same and snprintf
 * may not be always exported?
 */
#ifdef __unix
extern int snprintf(char *, size_t, const char *, ...);
#endif


int pcut_snprintf(char *dest, size_t size, const char *format, ...) {
	va_list args;
	int ret;

	va_start(args, format);

	/*
	 * Use sprintf_s in Windows but only with Microsoft compiler.
	 * Namely, let MinGW use snprintf.
	 */
#if (defined(__WIN64) || defined(__WIN32) || defined(_WIN32)) && defined(_MSC_VER)
	ret = _vsnprintf_s(dest, size, _TRUNCATE, format, args);
#else
	ret = vsnprintf(dest, size, format, args);
#endif

	va_end(args);

	return ret;
}
