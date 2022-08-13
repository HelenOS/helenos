/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <genarch/ofw.h>
#include <stddef.h>
#include <putchar.h>
#include <str.h>

typedef int (*ofw_entry_t)(ofw_args_t *args);

ofw_arg_t ofw(ofw_args_t *args)
{
	return ((ofw_entry_t) ofw_cif)(args);
}

void putuchar(char32_t ch)
{
	if (ch == '\n')
		ofw_putchar('\r');

	if (ascii_check(ch))
		ofw_putchar(ch);
	else
		ofw_putchar('?');
}
