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

/** @addtogroup ia32
 * @{
 */
/**
 * @file
 * @brief VESA frame buffer driver.
 */

#ifdef CONFIG_FB

#include <genarch/fb/fb.h>
#include <arch/drivers/vesa.h>
#include <console/chardev.h>
#include <console/console.h>
#include <putchar.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <typedefs.h>
#include <memstr.h>
#include <bitops.h>

uint32_t vesa_ph_addr;
uint16_t vesa_width;
uint16_t vesa_height;
uint16_t vesa_bpp;
uint16_t vesa_scanline;

uint8_t vesa_red_mask;
uint8_t vesa_red_pos;

uint8_t vesa_green_mask;
uint8_t vesa_green_pos;

uint8_t vesa_blue_mask;
uint8_t vesa_blue_pos;

bool vesa_init(void)
{
	if ((vesa_width == 0xffffU) || (vesa_height == 0xffffU))
		return false;
	
	visual_t visual;
	
	switch (vesa_bpp) {
	case 8:
		visual = VISUAL_INDIRECT_8;
		break;
	case 16:
		if ((vesa_red_mask == 5) && (vesa_red_pos == 10)
		    && (vesa_green_mask == 5) && (vesa_green_pos == 5)
		    && (vesa_blue_mask == 5) && (vesa_blue_pos == 0))
			visual = VISUAL_RGB_5_5_5_LE;
		else
			visual = VISUAL_RGB_5_6_5_LE;
		break;
	case 24:
		visual = VISUAL_BGR_8_8_8;
		break;
	case 32:
		visual = VISUAL_BGR_8_8_8_0;
		break;
	default:
		LOG("Unsupported bits per pixel.");
		return false;
	}
	
	fb_properties_t vesa_props = {
		.addr = vesa_ph_addr,
		.offset = 0,
		.x = vesa_width,
		.y = vesa_height,
		.scan = vesa_scanline,
		.visual = visual,
	};
	
	outdev_t *fbdev = fb_init(&vesa_props);
	if (!fbdev)
		return false;
	
	stdout_wire(fbdev);
	return true;
}

#endif

/** @}
 */
