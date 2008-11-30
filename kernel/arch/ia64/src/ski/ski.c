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
#include <arch/interrupt.h>
#include <sysinfo/sysinfo.h>
#include <arch/types.h>
#include <ddi/device.h>
#include <ddi/irq.h>
#include <ipc/irq.h>
#include <proc/thread.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch/drivers/kbd.h>

#define SKI_KBD_INR	0

static irq_t ski_kbd_irq;
static devno_t ski_kbd_devno;

chardev_t ski_console;
chardev_t ski_uconsole;

static bool kbd_disabled;

static void ski_putchar(chardev_t *d, const char ch);
static int32_t ski_getchar(void);

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
	asm volatile (
		"mov r15 = %0\n"
		"mov r32 = %1\n"	/* r32 is in0 */
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
int32_t ski_getchar(void)
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

/**
 * This is a blocking wrapper for ski_getchar().
 * To be used when the kernel crashes. 
 */
static char ski_getchar_blocking(chardev_t *d)
{
	int ch;

	while(!(ch = ski_getchar()))
		;
	if(ch == '\r')
		ch = '\n'; 
	return (char) ch;
}

/** Ask keyboard if a key was pressed. */
static void poll_keyboard(void)
{
	char ch;
	static char last; 
	ipl_t ipl;

	ipl = interrupts_disable();

	if (kbd_disabled) {
		interrupts_restore(ipl);
		return;
	}
		
	spinlock_lock(&ski_kbd_irq.lock);

	ch = ski_getchar();
	if(ch == '\r')
		ch = '\n'; 
	if (ch) {
		if (ski_kbd_irq.notif_cfg.notify && ski_kbd_irq.notif_cfg.answerbox) {
			chardev_push_character(&ski_uconsole, ch);
			ipc_irq_send_notif(&ski_kbd_irq);
		} else {
			chardev_push_character(&ski_console, ch);
		}	
		last = ch;
		spinlock_unlock(&ski_kbd_irq.lock);
		interrupts_restore(ipl);
		return;
	}

	if (last) {
		if (ski_kbd_irq.notif_cfg.notify && ski_kbd_irq.notif_cfg.answerbox) {
			chardev_push_character(&ski_uconsole, 0);
			ipc_irq_send_notif(&ski_kbd_irq);
		}
		last = 0;
	}

	spinlock_unlock(&ski_kbd_irq.lock);
	interrupts_restore(ipl);
}

/* Called from getc(). */
static void ski_kbd_enable(chardev_t *d)
{
	kbd_disabled = false;
}

/* Called from getc(). */
static void ski_kbd_disable(chardev_t *d)
{
	kbd_disabled = true;	
}

/** Decline to service hardware IRQ.
 *
 * This is only a virtual IRQ, so always decline.
 *
 * @return Always IRQ_DECLINE.
 */
static irq_ownership_t ski_kbd_claim(void)
{
	return IRQ_DECLINE;
}

static chardev_operations_t ski_ops = {
	.resume = ski_kbd_enable,
	.suspend = ski_kbd_disable,
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
	asm volatile (
		"mov r15 = %0\n"
		"break 0x80000\n"
		:
		: "i" (SKI_INIT_CONSOLE)
		: "r15", "r8"
	);

	chardev_initialize("ski_console", &ski_console, &ski_ops);
	chardev_initialize("ski_uconsole", &ski_uconsole, &ski_ops);
	stdin = &ski_console;
	stdout = &ski_console;

	ski_kbd_devno = device_assign_devno();
	
	irq_initialize(&ski_kbd_irq);
	ski_kbd_irq.inr = SKI_KBD_INR;
	ski_kbd_irq.devno = ski_kbd_devno;
	ski_kbd_irq.claim = ski_kbd_claim;
	irq_register(&ski_kbd_irq);

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, SKI_KBD_INR);
	sysinfo_set_item_val("kbd.devno", NULL, ski_kbd_devno);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SKI);
}

void ski_kbd_grab(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&ski_kbd_irq.lock);
	ski_kbd_irq.notif_cfg.notify = false;
	spinlock_unlock(&ski_kbd_irq.lock);
	interrupts_restore(ipl);
}

void ski_kbd_release(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&ski_kbd_irq.lock);
	if (ski_kbd_irq.notif_cfg.answerbox)
		ski_kbd_irq.notif_cfg.notify = true;
	spinlock_unlock(&ski_kbd_irq.lock);
	interrupts_restore(ipl);
}


#define POLL_INTERVAL		50000		/* 50 ms */

/** Kernel thread for polling keyboard. */
void kkbdpoll(void *arg)
{
	while (1) {
		poll_keyboard();
		thread_usleep(POLL_INTERVAL);
	}
}

/** @}
 */
