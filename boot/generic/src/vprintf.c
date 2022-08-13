/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <stddef.h>
#include <printf.h>
#include <printf_core.h>
#include <putchar.h>
#include <str.h>

static int vprintf_str_write(const char *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;

	while (offset < size) {
		putuchar(str_decode(str, &offset, size));
		chars++;
	}

	return chars;
}

int puts(const char *str)
{
	size_t offset = 0;
	size_t chars = 0;
	char32_t uc;

	while ((uc = str_decode(str, &offset, STR_NO_LIMIT)) != 0) {
		putuchar(uc);
		chars++;
	}

	putuchar('\n');
	return chars;
}

int vprintf(const char *fmt, va_list ap)
{
	printf_spec_t ps = {
		vprintf_str_write,
		NULL
	};

	int ret = printf_core(fmt, &ps, ap);

	return ret;
}

/** @}
 */
