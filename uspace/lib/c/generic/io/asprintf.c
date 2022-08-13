/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
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
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <io/printf_core.h>

static int asprintf_str_write(const char *str, size_t count, void *unused)
{
	return str_nlength(str, count);
}

static int asprintf_wstr_write(const char32_t *str, size_t count, void *unused)
{
	return wstr_nlength(str, count);
}

int vprintf_length(const char *fmt, va_list args)
{
	printf_spec_t ps = {
		asprintf_str_write,
		asprintf_wstr_write,
		NULL
	};

	return printf_core(fmt, &ps, args);
}

int printf_length(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vprintf_length(fmt, args);
	va_end(args);

	return ret;
}

/** Allocate and print to string.
 *
 * @param strp Address of the pointer where to store the address of
 *             the newly allocated string.
 * @fmt        Format string.
 * @args       Variable argument list
 *
 * @return Number of characters printed or an error code.
 *
 */
int vasprintf(char **strp, const char *fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	int ret = vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);

	if (ret >= 0) {
		*strp = malloc(ret + 1);
		if (*strp == NULL)
			return -1;

		vsnprintf(*strp, ret + 1, fmt, args);
	}

	return ret;
}

/** Allocate and print to string.
 *
 * @param strp Address of the pointer where to store the address of
 *             the newly allocated string.
 * @fmt        Format string.
 *
 * @return Number of characters printed or an error code.
 *
 */
int asprintf(char **strp, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vasprintf(strp, fmt, args);
	va_end(args);

	return ret;
}

/** @}
 */
