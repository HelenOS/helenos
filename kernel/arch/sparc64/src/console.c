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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#include <arch/console.h>
#include <arch/types.h>
#include <typedefs.h>
#include <genarch/fb/fb.h>
#include <arch/drivers/fb.h>

#include <arch/drivers/kbd.h>
#ifdef CONFIG_Z8530
#include <genarch/kbd/z8530.h>
#endif
#ifdef CONFIG_NS16550
#include <genarch/kbd/ns16550.h>
#endif

#include <console/chardev.h>
#include <console/console.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <proc/thread.h>
#include <arch/mm/tlb.h>
#include <arch/boot/boot.h>
#include <arch.h>

#define KEYBOARD_POLL_PAUSE	50000	/* 50ms */

/** Initialize kernel console to use framebuffer and keyboard directly. */
void standalone_sparc64_console_init(void)
{
	stdin = NULL;
		
	fb_init(bootinfo.screen.addr, bootinfo.screen.width, bootinfo.screen.height,
		bootinfo.screen.bpp, bootinfo.screen.scanline, true);

#ifdef KBD_ADDR_OVRD
	if (!bootinfo.keyboard.addr)
		bootinfo.keyboard.addr = KBD_ADDR_OVRD;
#endif

	if (bootinfo.keyboard.addr)
		kbd_init();
}

/** Kernel thread for polling keyboard.
 *
 * @param arg Ignored.
 */
void kkbdpoll(void *arg)
{
	thread_detach(THREAD);

	if (!bootinfo.keyboard.addr)
		return;
		
	while (1) {
#ifdef CONFIG_Z8530
		return;
#endif
#ifdef CONFIG_NS16550
		ns16550_poll();
#endif
		thread_usleep(KEYBOARD_POLL_PAUSE);
	}
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
#ifdef CONFIG_Z8530
	z8530_grab();
#endif
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
#ifdef CONFIG_Z8530
	z8530_release();
#endif
}

/** @}
 */
