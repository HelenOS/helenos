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
#include <putchar.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <typedefs.h>
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

static int vprintf_wstr_write(const char32_t *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;

	while (offset < size) {
		putuchar(str[chars]);
		chars++;
		offset += sizeof(char32_t);
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
		vprintf_wstr_write,
		NULL
	};

	return printf_core(fmt, &ps, ap);
}

/** @}
 */
