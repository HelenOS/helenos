/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <arch/arch.h>
#include <putchar.h>
#include <str.h>

#ifdef PUTCHAR_ADDRESS
#undef PUTCHAR_ADDRESS
#endif

#if defined(MACHINE_msim)
#define _putchar(ch)	msim_putchar((ch))
static void msim_putchar(char ch)
{
	*((char *) MSIM_VIDEORAM_ADDRESS) = ch;
}
#endif

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
#define _putchar(ch)	yamon_putchar((ch))
typedef void (**yamon_print_count_ptr_t)(uint32_t, const char *, uint32_t);
yamon_print_count_ptr_t yamon_print_count =
    (yamon_print_count_ptr_t) YAMON_SUBR_PRINT_COUNT;

static void yamon_putchar(char ch)
{
	(*yamon_print_count)(0, &ch, 1);
}
#endif

void putuchar(const char32_t ch)
{
	if (ascii_check(ch))
		_putchar(ch);
	else
		_putchar('?');
}
