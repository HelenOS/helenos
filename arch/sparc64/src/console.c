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

#include <arch/console.h>
#include <arch/types.h>
#include <typedefs.h>
#include <genarch/fb/fb.h>
#include <arch/drivers/fb.h>
#include <arch/drivers/i8042.h>
#include <genarch/i8042/i8042.h>
#include <genarch/ofw/ofw.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <proc/thread.h>
#include <synch/mutex.h>
#include <arch/mm/tlb.h>

#define KEYBOARD_POLL_PAUSE	50000	/* 50ms */

static void ofw_sparc64_putchar(chardev_t *d, const char ch);
static char ofw_sparc64_getchar(chardev_t *d);
static void ofw_sparc64_suspend(chardev_t *d);
static void ofw_sparc64_resume(chardev_t *d);

mutex_t canwork;

static volatile int ofw_console_active;

static chardev_t ofw_sparc64_console;
static chardev_operations_t ofw_sparc64_console_ops = {
	.write = ofw_sparc64_putchar,
	.read = ofw_sparc64_getchar,
	.resume = ofw_sparc64_resume,
	.suspend = ofw_sparc64_suspend
};

/** Initialize kernel console to use OpenFirmware services. */
void ofw_sparc64_console_init(void)
{
	chardev_initialize("ofw_sparc64_console", &ofw_sparc64_console, &ofw_sparc64_console_ops);
	stdin = &ofw_sparc64_console;
	stdout = &ofw_sparc64_console;
	mutex_initialize(&canwork);
	ofw_console_active = 1;
}

void fb_map_arch(__address virtaddr, __address physaddr, size_t size)
{
	dtlb_insert_mapping(virtaddr, physaddr, PAGESIZE_512K, true, false);
	dtlb_insert_mapping(virtaddr + 512*1024, physaddr + 512*1024, PAGESIZE_512K, true, false);
}

/** Initialize kernel console to use framebuffer and keyboard directly. */
void standalone_sparc64_console_init(void)
{
	ofw_console_active = 0;
	stdin = NULL;

	dtlb_insert_mapping(KBD_VIRT_ADDRESS, KBD_PHYS_ADDRESS, PAGESIZE_8K, true, false);

	fb_init(FB_PHYS_ADDRESS, FB_X_RES, FB_Y_RES, FB_COLOR_DEPTH, FB_X_RES * FB_COLOR_DEPTH / 8);
	i8042_init();
}

/** Write one character using OpenFirmware.
 *
 * @param d Character device (ignored).
 * @param ch Character to be written.
 */
void ofw_sparc64_putchar(chardev_t *d, const char ch)
{
	pstate_reg_t pstate;

	/*
	 * 32-bit OpenFirmware depends on PSTATE.AM bit set.
	 */	
	pstate.value = pstate_read();
	pstate.am = true;
	pstate_write(pstate.value);

	if (ch == '\n')
		ofw_putchar('\r');
	ofw_putchar(ch);
	
	pstate.am = false;
	pstate_write(pstate.value);
}

/** Read one character using OpenFirmware.
 *
 * The call is non-blocking.
 *
 * @param d Character device (ignored).
 * @return Character read or zero if no character was read.
 */
char ofw_sparc64_getchar(chardev_t *d)
{
	char ch;
	pstate_reg_t pstate;

	/*
	 * 32-bit OpenFirmware depends on PSTATE.AM bit set.
	 */	
	pstate.value = pstate_read();
	pstate.am = true;
	pstate_write(pstate.value);

	ch = ofw_getchar();
	
	pstate.am = false;
	pstate_write(pstate.value);
	
	return ch;
}

void ofw_sparc64_suspend(chardev_t *d)
{
	mutex_lock(&canwork);
}

void ofw_sparc64_resume(chardev_t *d)
{
	mutex_unlock(&canwork);
}

/** Kernel thread for pushing characters read from OFW to input buffer.
 *
 * @param arg Ignored.
 */
void kofwinput(void *arg)
{

	while (ofw_console_active) {
		char ch = 0;
		
		mutex_lock(&canwork);
		mutex_unlock(&canwork);
		
		ch = ofw_sparc64_getchar(NULL);
		if (ch) {
			if (ch == '\r')
				ch = '\n';
			chardev_push_character(&ofw_sparc64_console, ch);
		}
		thread_usleep(KEYBOARD_POLL_PAUSE);
	}
}

/** Kernel thread for polling keyboard.
 *
 * @param arg Ignored.
 */
void kkbdpoll(void *arg)
{
	while (1) {
		i8042_poll();		
		thread_usleep(KEYBOARD_POLL_PAUSE);
	}
}
