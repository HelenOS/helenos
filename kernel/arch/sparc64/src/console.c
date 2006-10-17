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

#include <arch/drivers/scr.h>
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
#include <genarch/ofw/ofw_tree.h>
#include <arch.h>
#include <panic.h>
#include <print.h>

#define KEYBOARD_POLL_PAUSE	50000	/* 50ms */

/** Initialize kernel console to use framebuffer and keyboard directly. */
void standalone_sparc64_console_init(void)
{
	stdin = NULL;

	ofw_tree_node_t *aliases;
	ofw_tree_property_t *prop;
	ofw_tree_node_t *screen;
	ofw_tree_node_t *keyboard;
	
	aliases = ofw_tree_lookup("/aliases");
	if (!aliases)
		panic("Can't find /aliases.\n");
	
	prop = ofw_tree_getprop(aliases, "screen");
	if (!prop)
		panic("Can't find property \"screen\".\n");
	if (!prop->value)
		panic("Can't find screen alias.\n");
	screen = ofw_tree_lookup(prop->value);
	if (!screen)
		panic("Can't find %s\n", prop->value);

	scr_init(screen);

	prop = ofw_tree_getprop(aliases, "keyboard");
	if (!prop)
		panic("Can't find property \"keyboard\".\n");
	if (!prop->value)
		panic("Can't find keyboard alias.\n");
	keyboard = ofw_tree_lookup(prop->value);
	if (!keyboard)
		panic("Can't find %s\n", prop->value);

	kbd_init(keyboard);
}

/** Kernel thread for polling keyboard.
 *
 * @param arg Ignored.
 */
void kkbdpoll(void *arg)
{
	thread_detach(THREAD);

#ifdef CONFIG_Z8530
	if (kbd_type == KBD_Z8530) {
		/*
		 * The z8530 driver is interrupt-driven.
		 */
		return;
	}
#endif

	while (1) {
#ifdef CONFIG_NS16550
		if (kbd_type == KBD_NS16550)
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
	switch (kbd_type) {
#ifdef CONFIG_Z8530
	case KBD_Z8530:
		z8530_grab();
		break;
#endif
#ifdef CONFIG_NS16550
	case KBD_NS16550:
		ns16550_grab();
		break;
#endif
	default:
		break;
	}
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
	switch (kbd_type) {
#ifdef CONFIG_Z8530
	case KBD_Z8530:
		z8530_release();
		break;
#endif
#ifdef CONFIG_NS16550
	case KBD_NS16550:
		ns16550_release();
		break;
#endif
	default:
		break;
	}
}

/** @}
 */
