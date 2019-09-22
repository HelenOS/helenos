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

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <arch/console.h>

#include <arch/drivers/scr.h>
#include <arch/drivers/kbd.h>
#include <arch/drivers/tty.h>
#include <genarch/srln/srln.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <proc/thread.h>
#include <arch/mm/tlb.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch.h>
#include <panic.h>
#include <str.h>

#define KEYBOARD_POLL_PAUSE	50000	/* 50ms */

/**
 * Initialize kernel console to use framebuffer and keyboard directly.
 * Called on UltraSPARC machines with standard keyboard and framebuffer.
 *
 * @param aliases	the "/aliases" OBP node
 */
static void standard_console_init(ofw_tree_node_t *aliases)
{
#ifdef CONFIG_FB
	ofw_tree_property_t *prop_scr = ofw_tree_getprop(aliases, "screen");
	if (!prop_scr)
		panic("Cannot find property 'screen'.");
	if (!prop_scr->value)
		panic("Cannot find screen alias.");
	ofw_tree_node_t *screen = ofw_tree_lookup(prop_scr->value);
	if (!screen)
		panic("Cannot find %s.", (char *) prop_scr->value);

	scr_init(screen);
#endif

#ifdef CONFIG_SUN_KBD
	ofw_tree_property_t *prop_kbd = ofw_tree_getprop(aliases, "keyboard");
	if (!prop_kbd)
		panic("Cannot find property 'keyboard'.");
	if (!prop_kbd->value)
		panic("Cannot find keyboard alias.");
	ofw_tree_node_t *keyboard = ofw_tree_lookup(prop_kbd->value);
	if (!keyboard)
		panic("Cannot find %s.", (char *) prop_kbd->value);

	kbd_init(keyboard);
#endif

#ifdef CONFIG_SUN_TTY
	ofw_tree_property_t *prop_tty = ofw_tree_getprop(aliases, "ttya");
	if (prop_tty && prop_tty->value) {
		ofw_tree_node_t *tty = ofw_tree_lookup(prop_tty->value);
		if (tty)
			tty_init(tty);
	}
#endif
}

/**
 * Initialize input/output. Auto-detects the type of machine
 * and calls the appropriate I/O init routine.
 */
void standalone_sparc64_console_init(void)
{
	ofw_tree_node_t *aliases;
	ofw_tree_property_t *prop;

	aliases = ofw_tree_lookup("/aliases");
	if (!aliases)
		panic("Cannot find '/aliases'.");

	/* "def-cn" = "default console" */
	prop = ofw_tree_getprop(aliases, "def-cn");

	if ((!prop) || (!prop->value))
		standard_console_init(aliases);
}

/** @}
 */
