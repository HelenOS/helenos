/*
 * Copyright (c) 2006 Jakub Jermar
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

#include <arch/drivers/kbd.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/ebus.h>
#include <console/console.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <align.h>
#include <str.h>
#include <log.h>
#include <sysinfo/sysinfo.h>

#ifdef CONFIG_SUN_KBD
#include <genarch/kbrd/kbrd.h>
#endif

#ifdef CONFIG_NS16550
#include <genarch/drivers/ns16550/ns16550.h>
#endif

#ifdef CONFIG_SUN_KBD

#ifdef CONFIG_NS16550

static bool kbd_ns16550_init(ofw_tree_node_t *node)
{
	const char *name = ofw_tree_node_name(node);

	if (str_cmp(name, "su") != 0)
		return false;

	/*
	 * Read 'interrupts' property.
	 */
	ofw_tree_property_t *prop = ofw_tree_getprop(node, "interrupts");
	if ((!prop) || (!prop->value)) {
		log(LF_ARCH, LVL_ERROR,
		    "ns16550: Unable to find interrupts property");
		return false;
	}

	uint32_t interrupts = *((uint32_t *) prop->value);

	/*
	 * Read 'reg' property.
	 */
	prop = ofw_tree_getprop(node, "reg");
	if ((!prop) || (!prop->value)) {
		log(LF_ARCH, LVL_ERROR,
		    "ns16550: Unable to find reg property");
		return false;
	}

	uintptr_t pa = 0; // Prevent -Werror=maybe-uninitialized
	if (!ofw_ebus_apply_ranges(node->parent,
	    ((ofw_ebus_reg_t *) prop->value), &pa)) {
		log(LF_ARCH, LVL_ERROR,
		    "ns16550: Failed to determine address");
		return false;
	}

	inr_t inr;
	cir_t cir;
	void *cir_arg;
	if (!ofw_ebus_map_interrupt(node->parent,
	    ((ofw_ebus_reg_t *) prop->value), interrupts, &inr, &cir,
	    &cir_arg)) {
		log(LF_ARCH, LVL_ERROR,
		    "ns16550: Failed to determine interrupt");
		return false;
	}

	ns16550_instance_t *ns16550_instance = ns16550_init((ioport8_t *) pa, 0,
	    inr, cir, cir_arg, NULL);
	if (ns16550_instance) {
		kbrd_instance_t *kbrd_instance = kbrd_init();
		if (kbrd_instance) {
			indev_t *sink = stdin_wire();
			indev_t *kbrd = kbrd_wire(kbrd_instance, sink);
			ns16550_wire(ns16550_instance, kbrd);
		}
	}

	/*
	 * This is the necessary evil until the userspace drivers are
	 * entirely self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, inr);
	sysinfo_set_item_val("kbd.address.physical", NULL, pa);
	sysinfo_set_item_val("kbd.type.ns16550", NULL, true);

	return true;
}

#endif /* CONFIG_NS16550 */

/** Initialize keyboard.
 *
 * Traverse OpenFirmware device tree in order to find necessary
 * info about the keyboard device.
 *
 * @param node Keyboard device node.
 *
 */
void kbd_init(ofw_tree_node_t *node)
{
#ifdef CONFIG_NS16550
	kbd_ns16550_init(node);
#endif
}

#endif /* CONFIG_SUN_KBD */

/** @}
 */
