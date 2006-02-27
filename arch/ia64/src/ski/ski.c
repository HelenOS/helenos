/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <arch/ski/ski.h>
#include <console/console.h>
#include <console/chardev.h>

static chardev_t ski_console;
static bool kb_disable;

static void ski_putchar(chardev_t *d, const char ch);
static __s32 ski_getchar(void);

/** Display character on debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param d Character device.
 * @param ch Character to be printed.
 */
void ski_putchar(chardev_t *d, const char ch)
{
	__asm__ volatile (
		"mov r15=%0\n"
		"mov r32=%1\n"		/* r32 is in0 */
		"break 0x80000\n"	/* modifies r8 */
		:
		: "i" (SKI_PUTCHAR), "r" (ch)
		: "r15", "in0", "r8"
	);
	
	if (ch == '\n')
		ski_putchar(d, '\r');
}

/** Ask debug console if a key was pressed.
 *
 * Use SSC (Simulator System Call) to
 * get character from debug console.
 *
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 */
__s32 ski_getchar(void)
{
	__u64 ch;
	
	__asm__ volatile (
		"mov r15=%1\n"
		"break 0x80000;;\n"	/* modifies r8 */
		"mov %0=r8;;\n"		

		: "=r" (ch)
		: "i" (SKI_GETCHAR)
		: "r15",  "r8"
	);

	return (__s32) ch;
}

/**
 * This is a blocking wrapper for ski_getchar().
 * To be used when the kernel crashes. 
 */
static char ski_getchar_blocking(chardev_t *d)
{
	int ch;

	while(!(ch=ski_getchar()))
		;
	if(ch == '\r')
		ch = '\n'; 
	return (char) ch;
}

/** Ask keyboard if a key was pressed. */
void poll_keyboard(void)
{
	char ch;

	if (kb_disable)
		return;

	ch = ski_getchar();
	if(ch == '\r')
		ch = '\n'; 
	if (ch)
		chardev_push_character(&ski_console, ch);
}

/* Called from getc(). */
static void ski_kb_enable(chardev_t *d)
{
	kb_disable = false;
}

/* Called from getc(). */
static void ski_kb_disable(chardev_t *d)
{
	kb_disable = true;	
}


static chardev_operations_t ski_ops = {
	.resume = ski_kb_enable,
	.suspend = ski_kb_disable,
	.write = ski_putchar,
	.read = ski_getchar_blocking
};


/** Initialize debug console
 *
 * Issue SSC (Simulator System Call) to
 * to open debug console.
 */
void ski_init_console(void)
{
	__asm__ volatile (
		"mov r15=%0\n"
		"break 0x80000\n"
		:
		: "i" (SKI_INIT_CONSOLE)
		: "r15", "r8"
	);

	chardev_initialize("ski_console", &ski_console, &ski_ops);
	stdin = &ski_console;
	stdout = &ski_console;
}
