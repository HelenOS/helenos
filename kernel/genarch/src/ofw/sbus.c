/*
 * SPDX-FileCopyrightText: 2007 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief	SBUS 'reg' and 'ranges' properties handling.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/sbus.h>
#include <macros.h>

bool ofw_sbus_apply_ranges(ofw_tree_node_t *node, ofw_sbus_reg_t *reg,
    uintptr_t *pa)
{
	ofw_tree_property_t *prop;
	ofw_sbus_range_t *range;
	size_t ranges;

	/*
	 * The SBUS support is very rudimentary in that we simply assume
	 * that the SBUS bus in question is connected directly to the UPA bus.
	 * Should we come across configurations that need more robust support,
	 * the driver will have to be extended to handle different topologies.
	 */
	if (!node->parent || node->parent->parent)
		return false;

	prop = ofw_tree_getprop(node, "ranges");
	if (!prop)
		return false;

	ranges = prop->size / sizeof(ofw_sbus_range_t);
	range = prop->value;

	unsigned int i;

	for (i = 0; i < ranges; i++) {
		if (overlaps(reg->addr, reg->size, range[i].child_base,
		    range[i].size)) {
			*pa = range[i].parent_base +
			    (reg->addr - range[i].child_base);
			return true;
		}
	}

	return false;
}

/** @}
 */
