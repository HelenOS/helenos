/*
 * SPDX-FileCopyrightText: 2006 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Boot framebuffer driver.
 */

#include <debug.h>
#include <typedefs.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/bfb.h>
#include <console/console.h>

uintptr_t bfb_addr = 0;
uint32_t bfb_width = 0;
uint32_t bfb_height = 0;
uint16_t bfb_bpp = 0;
uint32_t bfb_scanline = 0;

uint8_t bfb_red_pos = 0;
uint8_t bfb_red_size = 0;

uint8_t bfb_green_pos = 0;
uint8_t bfb_green_size = 0;

uint8_t bfb_blue_pos = 0;
uint8_t bfb_blue_size = 0;

bool bfb_init(void)
{
	if ((bfb_addr == 0) || (bfb_width == 0) || (bfb_height == 0) ||
	    (bfb_bpp == 0) || (bfb_scanline == 0))
		return false;

	fb_properties_t bfb_props = {
		.addr = bfb_addr,
		.offset = 0,
		.x = bfb_width,
		.y = bfb_height,
		.scan = bfb_scanline
	};

	switch (bfb_bpp) {
	case 8:
		bfb_props.visual = VISUAL_INDIRECT_8;
		break;
	case 16:
		if ((bfb_red_pos == 10) && (bfb_red_size == 5) &&
		    (bfb_green_pos == 5) && (bfb_green_size == 5) &&
		    (bfb_blue_pos == 0) && (bfb_blue_size == 5))
			bfb_props.visual = VISUAL_RGB_5_5_5_LE;
		else
			bfb_props.visual = VISUAL_RGB_5_6_5_LE;
		break;
	case 24:
		bfb_props.visual = VISUAL_BGR_8_8_8;
		break;
	case 32:
		bfb_props.visual = VISUAL_BGR_8_8_8_0;
		break;
	default:
		LOG("Unsupported bits per pixel.");
		return false;
	}

	outdev_t *fbdev = fb_init(&bfb_props);
	if (!fbdev)
		return false;

	stdout_wire(fbdev);
	return true;
}

/** @}
 */
