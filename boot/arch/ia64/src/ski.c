/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/ski.h>
#include <stdbool.h>
#include <stddef.h>

#define SKI_INIT_CONSOLE	20
#define	SKI_PUTCHAR		31

static void ski_console_init(void)
{
	static bool initialized = false;

	if (initialized)
		return;

	asm volatile (
	    "mov r15 = %[cmd]\n"
	    "break 0x80000\n"
	    :
	    : [cmd] "i" (SKI_INIT_CONSOLE)
	    : "r15", "r8"
	);

	initialized = true;
}

void ski_putchar(char ch)
{
	ski_console_init();

	if (ch == '\n')
		ski_putchar('\r');

	asm volatile (
	    "mov r15 = %[cmd]\n"
	    "mov r32 = %[ch]\n"   /* r32 is in0 */
	    "break 0x80000\n"     /* modifies r8 */
	    :
	    : [cmd] "i" (SKI_PUTCHAR), [ch] "r" (ch)
	    : "r15", "in0", "r8"
	);
}
