/*
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup genarch
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
