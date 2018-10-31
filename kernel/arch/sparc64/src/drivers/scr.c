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

#include <arch/drivers/scr.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/pci.h>
#include <genarch/ofw/sbus.h>
#include <genarch/ofw/upa.h>
#include <genarch/fb/fb.h>
#include <abi/fb/visuals.h>
#include <console/chardev.h>
#include <console/console.h>
#include <stdint.h>
#include <str.h>
#include <align.h>
#include <log.h>

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
	ofw_pci_reg_t *pci_reg;
	ofw_pci_reg_t pci_abs_reg;
	ofw_upa_reg_t *upa_reg;
	ofw_sbus_reg_t *sbus_reg;
	const char *name;

	name = ofw_tree_node_name(node);

	if (str_cmp(name, "SUNW,m64B") == 0)
		scr_type = SCR_ATYFB;
	else if (str_cmp(name, "SUNW,XVR-100") == 0)
		scr_type = SCR_XVR;
	else if (str_cmp(name, "SUNW,ffb") == 0)
		scr_type = SCR_FFB;
	else if (str_cmp(name, "cgsix") == 0)
		scr_type = SCR_CGSIX;
	else if (str_cmp(name, "QEMU,VGA") == 0)
		scr_type = SCR_QEMU_VGA;

	if (scr_type == SCR_UNKNOWN) {
		log(LF_ARCH, LVL_ERROR, "Unknown screen device.");
		return;
	}

	uintptr_t fb_addr;
	unsigned int fb_offset = 0;
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
		panic("Cannot find 'reg' property.");

	switch (scr_type) {
	case SCR_ATYFB:
		if (prop->size / sizeof(ofw_pci_reg_t) < 2) {
			log(LF_ARCH, LVL_ERROR, "Too few screen registers.");
			return;
		}

		pci_reg = &((ofw_pci_reg_t *) prop->value)[1];

		if (!ofw_pci_reg_absolutize(node, pci_reg, &pci_abs_reg)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to absolutize fb register.");
			return;
		}

		if (!ofw_pci_apply_ranges(node->parent, &pci_abs_reg,
		    &fb_addr)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to determine screen address.");
			return;
		}

		switch (fb_depth) {
		case 8:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_INDIRECT_8;
			break;
		case 16:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_5_6_5_BE;
			break;
		case 24:
			fb_scanline = fb_linebytes * 4;
			visual = VISUAL_BGR_8_8_8_0;
			break;
		case 32:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_0_8_8_8;
			break;
		default:
			log(LF_ARCH, LVL_ERROR,
			    "Unsupported bits per pixel.");
			return;
		}

		break;
	case SCR_XVR:
		if (prop->size / sizeof(ofw_pci_reg_t) < 2) {
			log(LF_ARCH, LVL_ERROR,
			    "Too few screen registers.");
			return;
		}

		pci_reg = &((ofw_pci_reg_t *) prop->value)[1];

		if (!ofw_pci_reg_absolutize(node, pci_reg, &pci_abs_reg)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to absolutize fb register.");
			return;
		}

		if (!ofw_pci_apply_ranges(node->parent, &pci_abs_reg,
		    &fb_addr)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to determine screen address.");
			return;
		}

		fb_offset = 4 * 0x2000;

		switch (fb_depth) {
		case 8:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_INDIRECT_8;
			break;
		case 16:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_5_6_5_BE;
			break;
		case 24:
			fb_scanline = fb_linebytes * 4;
			visual = VISUAL_BGR_8_8_8_0;
			break;
		case 32:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_0_8_8_8;
			break;
		default:
			log(LF_ARCH, LVL_ERROR,
			    "Unsupported bits per pixel.");
			return;
		}

		break;
	case SCR_FFB:
		fb_scanline = 8192;
		visual = VISUAL_BGR_0_8_8_8;

		upa_reg = &((ofw_upa_reg_t *) prop->value)[FFB_REG_24BPP];
		if (!ofw_upa_apply_ranges(node->parent, upa_reg, &fb_addr)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to determine screen address.");
			return;
		}

		break;
	case SCR_CGSIX:
		switch (fb_depth) {
		case 8:
			fb_scanline = fb_linebytes;
			visual = VISUAL_INDIRECT_8;
			break;
		default:
			log(LF_ARCH, LVL_WARN, "Not implemented.");
			return;
		}

		sbus_reg = &((ofw_sbus_reg_t *) prop->value)[0];
		if (!ofw_sbus_apply_ranges(node->parent, sbus_reg, &fb_addr)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to determine screen address.");
			return;
		}

		break;

	case SCR_QEMU_VGA:
		if (prop->size / sizeof(ofw_pci_reg_t) < 2) {
			log(LF_ARCH, LVL_ERROR, "Too few screen registers.");
			return;
		}

		pci_reg = &((ofw_pci_reg_t *) prop->value)[1];

		if (!ofw_pci_reg_absolutize(node, pci_reg, &pci_abs_reg)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to absolutize fb register.");
			return;
		}

		if (!ofw_pci_apply_ranges(node->parent, &pci_abs_reg,
		    &fb_addr)) {
			log(LF_ARCH, LVL_ERROR,
			    "Failed to determine screen address.");
			return;
		}

		switch (fb_depth) {
		case 8:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_INDIRECT_8;
			break;
		case 16:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_5_6_5_BE;
			break;
		case 24:
			fb_scanline = fb_linebytes * 4;
			visual = VISUAL_BGR_8_8_8_0;
			break;
		case 32:
			fb_scanline = fb_linebytes * (fb_depth >> 3);
			visual = VISUAL_RGB_0_8_8_8;
			break;
		default:
			log(LF_ARCH, LVL_ERROR, "Unsupported bits per pixel.");
			return;
		}
		break;

	default:
		panic("Unexpected type.");
	}

	fb_properties_t props = {
		.addr = fb_addr,
		.offset = fb_offset,
		.x = fb_width,
		.y = fb_height,
		.scan = fb_scanline,
		.visual = visual,
	};

	outdev_t *fbdev = fb_init(&props);
	if (fbdev)
		stdout_wire(fbdev);
}

/** @}
 */
