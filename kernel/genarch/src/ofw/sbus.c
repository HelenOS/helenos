/*
 * Copyright (c) 2007 Jakub Jermar
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

/** @addtogroup ofw
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
