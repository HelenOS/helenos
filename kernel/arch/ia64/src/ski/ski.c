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

/** @addtogroup ia64	
 * @{
 */
/** @file
 */

#include <arch/ski/ski.h>
#include <console/console.h>
#include <console/chardev.h>
#include <sysinfo/sysinfo.h>
#include <arch/types.h>
#include <proc/thread.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch/drivers/kbd.h>
#include <arch.h>

static chardev_t *skiout;

static chardev_t ski_stdout;

static bool kbd_disabled;

/** Display character on debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param d Character device.
 * @param ch Character to be printed.
 */
static void ski_putchar(chardev_t *d, const char ch, bool silent)
{
	if (!silent) {
		asm volatile (
			"mov r15 = %0\n"
			"mov r32 = %1\n"   /* r32 is in0 */
			"break 0x80000\n"  /* modifies r8 */
			:
			: "i" (SKI_PUTCHAR), "r" (ch)
			: "r15", "in0", "r8"
		);
		
		if (ch == '\n')
			ski_putchar(d, '\r', false);
	}
}

static chardev_operations_t ski_ops = {
	.write = ski_putchar
};

/** Ask debug console if a key was pressed.
 *
 * Use SSC (Simulator System Call) to
 * get character from debug console.
 *
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 */
static int32_t ski_getchar(void)
{
	uint64_t ch;
	
	asm volatile (
		"mov r15 = %1\n"
		"break 0x80000;;\n"	/* modifies r8 */
		"mov %0 = r8;;\n"		

		: "=r" (ch)
		: "i" (SKI_GETCHAR)
		: "r15", "r8"
	);

	return (int32_t) ch;
}

/** Ask keyboard if a key was pressed. */
static void poll_keyboard(void)
{
	char ch;
	ipl_t ipl;
	
	ipl = interrupts_disable();
	
	if (kbd_disabled) {
		interrupts_restore(ipl);
		return;
	}
	
	ch = ski_getchar();
	if(ch == '\r')
		ch = '\n'; 
	if (ch && skiout) {
		chardev_push_character(skiout, ch);
		interrupts_restore(ipl);
		return;
	}

	interrupts_restore(ipl);
}

#define POLL_INTERVAL           10000           /* 10 ms */

/** Kernel thread for polling keyboard. */
static void kkbdpoll(void *arg)
{
	while (1) {
		if (!silent) {
			poll_keyboard();
		}
		thread_usleep(POLL_INTERVAL);
	}
}

/** Initialize debug console
 *
 * Issue SSC (Simulator System Call) to
 * to open debug console.
 */
void ski_console_init(chardev_t *devout)
{
	asm volatile (
		"mov r15 = %0\n"
		"break 0x80000\n"
		:
		: "i" (SKI_INIT_CONSOLE)
		: "r15", "r8"
	);

	skiout = devout;
	chardev_initialize("ski_stdout", &ski_stdout, &ski_ops);
	stdout = &ski_stdout;

	thread_t *t = thread_create(kkbdpoll, NULL, TASK, 0, "kkbdpoll", true);
	if (!t)
		panic("Cannot create kkbdpoll.");
	thread_ready(t);

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SKI);

	sysinfo_set_item_val("fb", NULL, false);
}

void ski_kbd_grab(void)
{
	kbd_disabled = true;
}

void ski_kbd_release(void)
{
	kbd_disabled = false;
}

/** @}
 */
