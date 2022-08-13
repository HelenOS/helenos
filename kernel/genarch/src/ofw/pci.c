/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief	PCI 'reg' and 'ranges' properties handling.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/pci.h>
#include <arch/drivers/pci.h>
#include <arch/trap/interrupt.h>
#include <str.h>
#include <panic.h>
#include <macros.h>

#define PCI_SPACE_MASK		0x03000000
#define PCI_ABS_MASK		0x80000000
#define PCI_REG_MASK		0x000000ff

#define PCI_IGN			0x1f

bool
ofw_pci_apply_ranges(ofw_tree_node_t *node, ofw_pci_reg_t *reg, uintptr_t *pa)
{
	ofw_tree_property_t *prop;
	ofw_pci_range_t *range;
	size_t ranges;

	prop = ofw_tree_getprop(node, "ranges");
	if (!prop) {
		if (str_cmp(ofw_tree_node_name(node->parent), "pci") == 0)
			return ofw_pci_apply_ranges(node->parent, reg, pa);
		return false;
	}

	ranges = prop->size / sizeof(ofw_pci_range_t);
	range = prop->value;

	unsigned int i;

	for (i = 0; i < ranges; i++) {
		if ((reg->space & PCI_SPACE_MASK) !=
		    (range[i].space & PCI_SPACE_MASK))
			continue;
		if (overlaps(reg->addr, reg->size, range[i].child_base,
		    range[i].size)) {
			*pa = range[i].parent_base +
			    (reg->addr - range[i].child_base);
			return true;
		}
	}

	return false;
}

bool
ofw_pci_reg_absolutize(ofw_tree_node_t *node, ofw_pci_reg_t *reg,
    ofw_pci_reg_t *out)
{
	if (reg->space & PCI_ABS_MASK) {
		/* already absolute */
		out->space = reg->space;
		out->addr = reg->addr;
		out->size = reg->size;
		return true;
	}

	ofw_tree_property_t *prop;
	ofw_pci_reg_t *assigned_address;
	size_t assigned_addresses;

	prop = ofw_tree_getprop(node, "assigned-addresses");
	if (!prop)
		panic("Cannot find 'assigned-addresses' property.");

	assigned_addresses = prop->size / sizeof(ofw_pci_reg_t);
	assigned_address = prop->value;

	unsigned int i;

	for (i = 0; i < assigned_addresses; i++) {
		if ((assigned_address[i].space & PCI_REG_MASK) ==
		    (reg->space & PCI_REG_MASK)) {
			out->space = assigned_address[i].space;
			out->addr = reg->addr + assigned_address[i].addr;
			out->size = reg->size;
			return true;
		}
	}

	return false;
}

/** Map PCI interrupt.
 *
 * So far, we only know how to map interrupts of non-PCI devices connected
 * to a PCI bridge.
 */
bool
ofw_pci_map_interrupt(ofw_tree_node_t *node, ofw_pci_reg_t *reg, int ino,
    int *inr, cir_t *cir, void **cir_arg)
{
	pci_t *pci = node->device;
	if (!pci) {
		pci = pci_init(node);
		if (!pci)
			return false;
		node->device = pci;
	}

	pci_enable_interrupt(pci, ino);

	*inr = (PCI_IGN << IGN_SHIFT) | ino;
	*cir = pci_clear_interrupt;
	*cir_arg = pci;

	return true;
}

/** @}
 */
