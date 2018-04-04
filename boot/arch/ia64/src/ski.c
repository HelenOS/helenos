/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

void ski_putchar(const wchar_t ch)
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
