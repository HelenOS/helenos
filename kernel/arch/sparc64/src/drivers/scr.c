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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#include <arch/drivers/scr.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/visuals.h>
#include <arch/types.h>
#include <typedefs.h>
#include <func.h>
#include <align.h>
#include <print.h>

#define FFB_REG_24BPP	7

scr_type_t scr_type = SCR_UNKNOWN;

/** Initialize screen.
 *
 * Traverse OpenFirmware device tree in order to find necessary
 * info about the screen device.
 *
 * @param node Screen device node.
 */
void scr_init(ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop;
	const char *name;
	
	name = ofw_tree_node_name(node);
	
	if (strcmp(name, "SUNW,m64B") == 0)
		scr_type = SCR_ATYFB;
	else if (strcmp(name, "SUNW,ffb") == 0)
		scr_type = SCR_FFB;
	
	if (scr_type == SCR_UNKNOWN) {
		printf("Unknown keyboard device.\n");
		return;
	}
	
	uintptr_t fb_addr;
	uint32_t fb_width = 0;
	uint32_t fb_height = 0;
	uint32_t fb_depth = 0;
	uint32_t fb_linebytes = 0;
	uint32_t fb_scanline = 0;
	unsigned int visual;

	prop = ofw_tree_getprop(node, "width");
	if (prop && prop->value)
		fb_width = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "height");
	if (prop && prop->value)
		fb_height = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "depth");
	if (prop && prop->value)
		fb_depth = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "linebytes");
	if (prop && prop->value)
		fb_linebytes = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "reg");
	if (!prop)
		panic("Can't find \"reg\" property.\n");

	switch (scr_type) {
	case SCR_ATYFB:
		if (prop->size / sizeof(ofw_pci_reg_t) < 2) {
			printf("Too few screen registers.\n");
			return;
		}
	
		ofw_pci_reg_t *fb_reg = &((ofw_pci_reg_t *) prop->value)[1];
		ofw_pci_reg_t abs_reg;
		
		if (!ofw_pci_reg_absolutize(node, fb_reg, &abs_reg)) {
			printf("Failed to absolutize fb register.\n");
			return;
		}
	
		if (!ofw_pci_apply_ranges(node->parent, &abs_reg , &fb_addr)) {
			printf("Failed to determine screen address.\n");
			return;
		}
		
		switch (fb_depth) {
		case 8:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_INDIRECT_8;
			break;
		case 16:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_5_6_5;
			break;
		case 24:
			fb_scanline = fb_linebytes * 4;
			visual = VISUAL_RGB_8_8_8_0;
			break;
		case 32:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_0_8_8_8;
			break;
		default:
			printf("Unsupported bits per pixel.\n");
			return;
		}
		
		break;
	case SCR_FFB:	
		fb_scanline = 8192;
		visual = VISUAL_BGR_0_8_8_8;

		ofw_upa_reg_t *reg = &((ofw_upa_reg_t *) prop->value)[FFB_REG_24BPP];
		if (!ofw_upa_apply_ranges(node->parent, reg, &fb_addr)) {
			printf("Failed to determine screen address.\n");
			return;
		}

		break;
	default:
		panic("Unexpected type.\n");
	}

	fb_init(fb_addr, fb_width, fb_height, fb_scanline, visual);
}

/** @}
 */
