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
 * @brief	EBUS 'reg' and 'ranges' properties handling.
 *
 */

#include <assert.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/ebus.h>
#include <genarch/ofw/pci.h>
#include <str.h>
#include <panic.h>
#include <macros.h>

/** Apply EBUS ranges to EBUS register. */
bool
ofw_ebus_apply_ranges(ofw_tree_node_t *node, ofw_ebus_reg_t *reg, uintptr_t *pa)
{
	ofw_tree_property_t *prop;
	ofw_ebus_range_t *range;
	size_t ranges;

	prop = ofw_tree_getprop(node, "ranges");
	if (!prop)
		return false;

	ranges = prop->size / sizeof(ofw_ebus_range_t);
	range = prop->value;

	unsigned int i;

	for (i = 0; i < ranges; i++) {
		if (reg->space != range[i].child_space)
			continue;
		if (overlaps(reg->addr, reg->size, range[i].child_base,
		    range[i].size)) {
			ofw_pci_reg_t pci_reg;

			pci_reg.space = range[i].parent_space;
			pci_reg.addr = range[i].parent_base +
			    (reg->addr - range[i].child_base);
			pci_reg.size = reg->size;

			return ofw_pci_apply_ranges(node->parent, &pci_reg, pa);
		}
	}

	return false;
}

bool
ofw_ebus_map_interrupt(ofw_tree_node_t *node, ofw_ebus_reg_t *reg,
    uint32_t interrupt, int *inr, cir_t *cir, void **cir_arg)
{
	ofw_tree_property_t *prop;
	ofw_tree_node_t *controller;

	prop = ofw_tree_getprop(node, "interrupt-map");
	if (!prop || !prop->value)
		return false;

	ofw_ebus_intr_map_t *intr_map = prop->value;
	size_t count = prop->size / sizeof(ofw_ebus_intr_map_t);

	assert(count);

	prop = ofw_tree_getprop(node, "interrupt-map-mask");
	if (!prop || !prop->value)
		return false;

	ofw_ebus_intr_mask_t *intr_mask = prop->value;

	assert(prop->size == sizeof(ofw_ebus_intr_mask_t));

	uint32_t space = reg->space & intr_mask->space_mask;
	uint32_t addr = reg->addr & intr_mask->addr_mask;
	uint32_t intr = interrupt & intr_mask->intr_mask;

	unsigned int i;
	for (i = 0; i < count; i++) {
		if ((intr_map[i].space == space) &&
		    (intr_map[i].addr == addr) && (intr_map[i].intr == intr))
			goto found;
	}
	return false;

found:
	/*
	 * We found the device that functions as an interrupt controller
	 * for the interrupt. We also found partial mapping from interrupt to
	 * INO.
	 */

	controller = ofw_tree_find_node_by_handle(ofw_tree_lookup("/"),
	    intr_map[i].controller_handle);
	if (!controller)
		return false;

	if (str_cmp(ofw_tree_node_name(controller), "pci") != 0) {
		/*
		 * This is not a PCI node.
		 */
		return false;
	}

	/*
	 * Let the PCI do the next step in mapping the interrupt.
	 */
	if (!ofw_pci_map_interrupt(controller, NULL, intr_map[i].controller_ino,
	    inr, cir, cir_arg))
		return false;

	return true;
}

/** @}
 */
