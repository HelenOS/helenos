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

#include <arch/drivers/ski.h>
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

#define POLL_INTERVAL  10000  /* 10 ms */

#define SKI_INIT_CONSOLE  20
#define SKI_GETCHAR       21
#define SKI_PUTCHAR       31

static void ski_putchar(outdev_t *, const wchar_t, bool);

static outdev_operations_t skiout_ops = {
	.write = ski_putchar
};

static outdev_t skiout;            /**< Ski output device. */
static bool initialized = false;
static bool kbd_disabled = false;

/** Initialize debug console
 *
 * Issue SSC (Simulator System Call) to
 * to open debug console.
 *
 */
static void ski_init(void)
{
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

static void ski_do_putchar(const wchar_t ch)
{
	asm volatile (
		"mov r15 = %[cmd]\n"
		"mov r32 = %[ch]\n"   /* r32 is in0 */
		"break 0x80000\n"     /* modifies r8 */
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
 * @param dev    Character device.
 * @param ch     Character to be printed.
 * @param silent Whether the output should be silenced.
 *
 */
static void ski_putchar(outdev_t *dev, const wchar_t ch, bool silent)
{
	if (!silent) {
		if (ascii_check(ch)) {
			if (ch == '\n')
				ski_do_putchar('\r');
			
			ski_do_putchar(ch);
		} else
			ski_do_putchar(U_SPECIAL);
	}
}

void skiout_init(void)
{
	ski_init();
	
	outdev_initialize("skiout", &skiout, &skiout_ops);
	stdout = &skiout;
	
	sysinfo_set_item_val("fb", NULL, false);
}

/** Ask debug console if a key was pressed.
 *
 * Use SSC (Simulator System Call) to
 * get character from debug console.
 *
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 *
 */
static wchar_t ski_getchar(void)
{
	uint64_t ch;
	
	asm volatile (
		"mov r15 = %1\n"
		"break 0x80000;;\n"  /* modifies r8 */
		"mov %0 = r8;;\n"
		
		: "=r" (ch)
		: "i" (SKI_GETCHAR)
		: "r15", "r8"
	);
	
	return (wchar_t) ch;
}

/** Ask keyboard if a key was pressed. */
static void poll_keyboard(ski_instance_t *instance)
{
	if (kbd_disabled)
		return;
	
	wchar_t ch = ski_getchar();
	
	if (ch != 0)
		indev_push_character(instance->srlnin, ch);
}

/** Kernel thread for polling keyboard. */
static void kskipoll(void *arg)
{
	ski_instance_t *instance = (ski_instance_t *) arg;
	
	while (true) {
		if (!silent)
			poll_keyboard(instance);
		
		thread_usleep(POLL_INTERVAL);
	}
}

ski_instance_t *skiin_init(void)
{
	ski_init();
	
	ski_instance_t *instance =
	    malloc(sizeof(ski_instance_t), FRAME_ATOMIC);
	
	if (instance) {
		instance->thread = thread_create(kskipoll, instance, TASK, 0,
		    "kskipoll", true);
		
		if (!instance->thread) {
			free(instance);
			return NULL;
		}
		
		instance->srlnin = NULL;
	}
	
	return instance;
}

void skiin_wire(ski_instance_t *instance, indev_t *srlnin)
{
	ASSERT(instance);
	ASSERT(srlnin);
	
	instance->srlnin = srlnin;
	thread_ready(instance->thread);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SKI);
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
