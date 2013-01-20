/*
 * Copyright (c) 2013 Jan Vesely
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

/** @addtogroup amdm37x
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <errno.h>
#include <ddf/log.h>
#include <ddi.h>
#include <abi/mm/as.h>

#include "amdm37x_dispc.h"


static int change_mode(visualizer_t *vis, vslmode_t mode);
static int handle_damage(visualizer_t *vs,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset);

static const visualizer_ops_t amdm37x_dispc_vis_ops = {
	.change_mode = change_mode,
	.handle_damage = handle_damage,
	// TODO DO we need dummy implementations of stuff like claim, yield, ...
};


static unsigned visual2bpp(visual_t visual)
{
	switch(visual) {
	case VISUAL_INDIRECT_8:
		return 1;
	case VISUAL_RGB_5_5_5_LE:
	case VISUAL_RGB_5_5_5_BE:
	case VISUAL_RGB_5_6_5_LE:
	case VISUAL_RGB_5_6_5_BE:
		return 2;
	case VISUAL_BGR_8_8_8:
	case VISUAL_RGB_8_8_8:
		return 3;
	case VISUAL_BGR_0_8_8_8:
	case VISUAL_BGR_8_8_8_0:
	case VISUAL_ABGR_8_8_8_8:
	case VISUAL_BGRA_8_8_8_8:
	case VISUAL_RGB_0_8_8_8:
	case VISUAL_RGB_8_8_8_0:
	case VISUAL_ARGB_8_8_8_8:
	case VISUAL_RGBA_8_8_8_8:
		return 4;
	case VISUAL_UNKNOWN:
		return 0;
	}
	return 0;
}

static void mode_init(vslmode_list_element_t *mode,
    unsigned width, unsigned height, visual_t visual)
{
	mode->mode.index = 0;
	mode->mode.version = 0;
	mode->mode.refresh_rate = 0;
	mode->mode.screen_aspect.width = width;
	mode->mode.screen_aspect.height = height;
	mode->mode.screen_width = width;
	mode->mode.screen_height = height;
	mode->mode.cell_aspect.width = 1;
	mode->mode.cell_aspect.height = 1;
	mode->mode.cell_visual.pixel_visual = visual;

	link_initialize(&mode->link);

}

int amdm37x_dispc_init(amdm37x_dispc_t *instance, visualizer_t *vis)
{
	assert(instance);
	assert(vis);

	instance->fb_data = NULL;
	instance->size = 0;

	visual_t visual = VISUAL_UNKNOWN;

	switch (CONFIG_BFB_BPP)	{
	case 8: visual = VISUAL_INDIRECT_8; break;
	case 16: visual = VISUAL_RGB_5_6_5_LE; break;
	case 24: visual = VISUAL_BGR_8_8_8; break;
	case 32: visual = VISUAL_RGB_8_8_8_0; break;
	default:
		return EINVAL;
	}

	int ret = pio_enable((void*)AMDM37x_DISPC_BASE_ADDRESS,
	    AMDM37x_DISPC_SIZE, (void**)&instance->regs);
	if (ret != EOK) {
		return EIO;
	}

	mode_init(&instance->modes[0],
	    CONFIG_BFB_WIDTH, CONFIG_BFB_HEIGHT, visual);

	/* Handle vis stuff */
	vis->dev_ctx = instance;
	vis->def_mode_idx = 0;
	list_append(&instance->modes[0].link, &vis->modes);

	return EOK;
};

int amdm37x_dispc_fini(amdm37x_dispc_t *instance)
{
	return EOK;
};

static int amdm37x_dispc_setup_fb(amdm37x_dispc_regs_t *regs,
    unsigned x, unsigned y, unsigned bpp, uint32_t pa)
{
	assert(regs);
	/* Init sequence for dispc is in chapter 7.6.5.1.4 p. 1810,
	 * no idea what parts of that work. */

	/* Disable all interrupts */
	regs->irqenable = 0;

	/* Pixel format specifics*/
	uint32_t attrib_pixel_format = 0;
	uint32_t control_data_lanes = 0;
	switch (bpp)
	{
	case 32:
		attrib_pixel_format = AMDM37X_DISPC_GFX_ATTRIBUTES_FORMAT_RGBX;
		control_data_lanes = AMDM37X_DISPC_CONTROL_TFTDATALINES_24B;
		break;
	case 24:
		attrib_pixel_format = AMDM37X_DISPC_GFX_ATTRIBUTES_FORMAT_RGB24;
		control_data_lanes = AMDM37X_DISPC_CONTROL_TFTDATALINES_24B;
		break;
	case 16:
		attrib_pixel_format = AMDM37X_DISPC_GFX_ATTRIBUTES_FORMAT_RGB16;
		control_data_lanes = AMDM37X_DISPC_CONTROL_TFTDATALINES_16B;
		break;
	default:
		return EINVAL;
	}

	/* Prepare sizes */
	const uint32_t size_reg =
	    (((x - 1) & AMDM37X_DISPC_SIZE_WIDTH_MASK)
	        << AMDM37X_DISPC_SIZE_WIDTH_SHIFT) |
	    (((y - 1) & AMDM37X_DISPC_SIZE_HEIGHT_MASK)
	        << AMDM37X_DISPC_SIZE_HEIGHT_SHIFT);

	/* modes taken from u-boot, for 1024x768 */
	// TODO replace magic values with actual correct values
//	regs->timing_h = 0x1a4024c9;
//	regs->timing_v = 0x02c00509;
//	regs->pol_freq = 0x00007028;
//	regs->divisor  = 0x00010001;

	/* setup output */
	regs->size_lcd = size_reg;
	regs->size_dig = size_reg;

	/* Nice blue default color */
	regs->default_color[0] = 0x0000ff;
	regs->default_color[1] = 0x0000ff;

	/* Setup control register */
	uint32_t control = 0 |
		AMDM37X_DISPC_CONTROL_PCKFREEENABLE_FLAG |
		(control_data_lanes << AMDM37X_DISPC_CONTROL_TFTDATALINES_SHIFT) |
		AMDM37X_DISPC_CONTROL_GPOUT0_FLAG |
		AMDM37X_DISPC_CONTROL_GPOUT1_FLAG;
	regs->control = control;

	/* No gamma stuff only data */
	uint32_t config = (AMDM37X_DISPC_CONFIG_LOADMODE_DATAEVERYFRAME
	            << AMDM37X_DISPC_CONFIG_LOADMODE_SHIFT);
	regs->config = config;


	/* Set framebuffer base address */
	regs->gfx.ba[0] = pa;
	regs->gfx.ba[1] = pa;
	regs->gfx.position = 0;

	/* Setup fb size */
	regs->gfx.size = size_reg;

	/* Set pixel format */
	uint32_t attribs = 0 |
	    (attrib_pixel_format << AMDM37X_DISPC_GFX_ATTRIBUTES_FORMAT_SHIFT);
	regs->gfx.attributes = attribs;

	/* 0x03ff03c0 is the default */
	regs->gfx.fifo_threshold = 0x03ff03c0;
	/* This value should be stride - width, 1 means next pixel i.e.
	 * stride == width */
	regs->gfx.row_inc = 1;
	/* number of bytes to next pixel in BPP multiples */
	regs->gfx.pixel_inc = 1;
	/* only used if video is played over fb */
	regs->gfx.window_skip = 0;
	/* Gamma and palette table */
	regs->gfx.table_ba = 0;

	/* enable frame buffer graphics */
	regs->gfx.attributes |= AMDM37X_DISPC_GFX_ATTRIBUTES_ENABLE_FLAG;
	/* Update register values */
	regs->control |= AMDM37X_DISPC_CONTROL_GOLCD_FLAG;
	regs->control |= AMDM37X_DISPC_CONTROL_GODIGITAL_FLAG;
	/* Enable output */
	regs->control |= AMDM37X_DISPC_CONTROL_LCD_ENABLE_FLAG;
	regs->control |= AMDM37X_DISPC_CONTROL_DIGITAL_ENABLE_FLAG;
}

static int handle_damage(visualizer_t *vs,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset)
{
	ddf_log_fatal("Handling damage\n");
	return EOK;
}

static int change_mode(visualizer_t *vis, vslmode_t mode)
{
	assert(vis);
	assert(vis->dev_ctx);

	amdm37x_dispc_t *dispc = vis->dev_ctx;
	const unsigned bpp = visual2bpp(mode.cell_visual.pixel_visual);
	const unsigned x = mode.screen_width;
	const unsigned y = mode.screen_height;
	ddf_log_note("Setting mode: %ux%ux%u\n", x, y, bpp);
	const size_t size = x * y * bpp;
	void *buffer, *pa;
	int ret = dmamem_map_anonymous(size, AS_AREA_READ | AS_AREA_WRITE,
	    0, &pa, &buffer);
	if (ret != EOK) {
		ddf_log_error("Failed to get new FB\n");
		return ret;
	}
	amdm37x_dispc_setup_fb(dispc->regs, x, y, bpp, (uint32_t)pa);

	if (dispc->fb_data)
		dmamem_unmap_anonymous(dispc->fb_data);
	return EOK;
}
