/*
 * Copyright (C) 2006 Jakub Jermar
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
 * @brief	FHC 'reg' and 'ranges' properties handling.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <arch/memstr.h>
#include <func.h>
#include <panic.h>
#include <macros.h>

bool ofw_fhc_apply_ranges(ofw_tree_node_t *node, ofw_fhc_reg_t *reg, uintptr_t *pa)
{
	ofw_tree_property_t *prop;
	ofw_fhc_range_t *range;
	count_t ranges;

	prop = ofw_tree_getprop(node, "ranges");
	if (!prop)
		return false;
		
	ranges = prop->size / sizeof(ofw_fhc_range_t);
	range = prop->value;
	
	int i;
	
	for (i = 0; i < ranges; i++) {
		if (overlaps(reg->addr, reg->size, range[i].child_base, range[i].size)) {
			uintptr_t addr;
			
			addr = range[i].parent_base + (reg->addr - range[i].child_base);
			if (!node->parent->parent) {
				*pa = addr;
				return true;
			}
			if (strcmp(ofw_tree_node_name(node->parent), "central") != 0)
				panic("Unexpected parent node: %s.\n", ofw_tree_node_name(node->parent));
			
			ofw_central_reg_t central_reg;
			
			central_reg.addr = addr;
			central_reg.size = reg->size;
			
			return ofw_central_apply_ranges(node->parent, &central_reg, pa);
		}
	}

	return false;
}

bool ofw_central_apply_ranges(ofw_tree_node_t *node, ofw_central_reg_t *reg, uintptr_t *pa)
{
	if (node->parent->parent)
		panic("Unexpected parent node: %s.\n", ofw_tree_node_name(node->parent));
	
	ofw_tree_property_t *prop;
	ofw_central_range_t *range;
	count_t ranges;
	
	prop = ofw_tree_getprop(node, "ranges");
	if (!prop)
		return false;
		
	ranges = prop->size / sizeof(ofw_central_range_t);
	range = prop->value;
	
	int i;
	
	for (i = 0; i < ranges; i++) {
		if (overlaps(reg->addr, reg->size, range[i].child_base, range[i].size)) {
			*pa = range[i].parent_base + (reg->addr - range[i].child_base);
			return true;
		}
	}
	
	return false;
}

/** @}
 */
