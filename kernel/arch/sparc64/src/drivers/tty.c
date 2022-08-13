/*
 * SPDX-FileCopyrightText: 2019 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/ebus.h>
#include <console/console.h>
#include <genarch/srln/srln.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <align.h>
#include <str.h>
#include <log.h>

#ifdef CONFIG_SUN_TTY
#include <arch/drivers/tty.h>
#endif

#ifdef CONFIG_NS16550
#include <genarch/drivers/ns16550/ns16550.h>
#endif

#ifdef CONFIG_SUN_TTY

#ifdef CONFIG_NS16550

static bool tty_ns16550_init(ofw_tree_node_t *node)
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

	outdev_t *ns16550_out;
	ns16550_instance_t *ns16550_instance = ns16550_init((ioport8_t *) pa, 0,
	    inr, cir, cir_arg, &ns16550_out);
	if (ns16550_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			ns16550_wire(ns16550_instance, srln);
		}
		stdout_wire(ns16550_out);
	}

	return true;
}

#endif /* CONFIG_NS16550 */

/** Initialize tty.
 *
 * Traverse OpenFirmware device tree in order to find necessary
 * info about the keyboard device.
 *
 * @param node Keyboard device node.
 *
 */
void tty_init(ofw_tree_node_t *node)
{
#ifdef CONFIG_NS16550
	tty_ns16550_init(node);
#endif
}

#endif /* CONFIG_SUN_TTY */

/** @}
 */
