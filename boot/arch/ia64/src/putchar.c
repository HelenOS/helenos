/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/arch.h>
#include <putchar.h>
#include <stddef.h>
#include <str.h>
#include <arch/ski.h>

void putuchar(const char32_t ch)
{
#ifdef MACHINE_ski
	if (ascii_check(ch))
		ski_putchar(ch);
	else
		ski_putchar('?');
#endif
}
