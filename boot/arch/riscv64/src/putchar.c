/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <arch/arch.h>
#include <putchar.h>
#include <str.h>
#include <arch/ucb.h>

void putuchar(char32_t ch)
{
	if (ascii_check(ch))
		htif_cmd(HTIF_DEVICE_CONSOLE, HTIF_CONSOLE_PUTC, ch);
	else
		htif_cmd(HTIF_DEVICE_CONSOLE, HTIF_CONSOLE_PUTC, '?');
}
