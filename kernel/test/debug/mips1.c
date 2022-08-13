/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>

const char *test_mips1(void)
{
	TPRINTF("If kconsole is compiled in, you should enter debug mode now.\n");

	asm volatile (
	    "break\n"
	);

	return "Back from debug mode";
}
