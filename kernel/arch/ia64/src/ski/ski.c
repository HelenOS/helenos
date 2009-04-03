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
#include <string.h>
#include <arch.h>

static indev_t skiin;		/**< Ski input device. */
static outdev_t skiout;		/**< Ski output device. */

static bool kbd_disabled;

static void ski_do_putchar(const wchar_t ch)
{
	asm volatile (
		"mov r15 = %[cmd]\n"
		"mov r32 = %[ch]\n"   /* r32 is in0 */
		"break 0x80000\n"  /* modifies r8 */
		:
		: [cmd] "i" (SKI_PUTCHAR), [ch] "r" (ch)
		: "r15", "in0", "r8"
	);
}

/** Display character on debug console
 *
 * Use SSC (Simulator System Call) to
 * display character on debug console.
 *
 * @param d Character device.
 * @param ch Character to be printed.
 */
static void ski_putchar(outdev_t *d, const wchar_t ch, bool silent)
{
	if (!silent) {
		if (ascii_check(ch)) {
			if (ch == '\n')
				ski_do_putchar('\r');
			
			ski_do_putchar(ch);
		} else
			ski_do_putchar(invalch);
	}
}

static indev_operations_t skiin_ops = {
	.poll = NULL
};

static outdev_operations_t skiout_ops = {
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
	
	if (kbd_disabled)
		return;
	ch = ski_getchar();
	if(ch == '\r')
		ch = '\n'; 
	if (ch) {
		indev_push_character(&skiin, ch);
		return;
	}
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
static void ski_init(void)
{
	static bool initialized;

	if (initialized)
		return;
	
	asm volatile (
		"mov r15 = %0\n"
		"break 0x80000\n"
		:
		: "i" (SKI_INIT_CONSOLE)
		: "r15", "r8"
	);
	
	initialized = true;
}

indev_t *skiin_init(void)
{
	ski_init();

	indev_initialize("skiin", &skiin, &skiin_ops);
	thread_t *t = thread_create(kkbdpoll, NULL, TASK, 0, "kkbdpoll", true);
	if (t)
		thread_ready(t);
	else
		return NULL;

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SKI);

	return &skiin;
}


void skiout_init(void)
{
	ski_init();

	outdev_initialize("skiout", &skiout, &skiout_ops);
	stdout = &skiout;

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
