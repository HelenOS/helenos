/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup amdm37x_dispc
 * @{
 */
/**
 * @file
 */

#include <align.h>
#include <as.h>
#include <assert.h>
#include <errno.h>
#include <ddev_srv.h>
#include <ddev/info.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <gfx/color.h>
#include <io/pixelmap.h>

#include "amdm37x_dispc.h"

#ifndef CONFIG_BFB_BPP
#define CONFIG_BFB_BPP 24
#endif

#ifndef CONFIG_BFB_WIDTH
#define CONFIG_BFB_WIDTH 1024
#endif

#ifndef CONFIG_BFB_HEIGHT
#define CONFIG_BFB_HEIGHT 768
#endif

static errno_t amdm37x_change_mode(amdm37x_dispc_t *, unsigned, unsigned,
    visual_t);

static errno_t amdm37x_ddev_get_gc(void *, sysarg_t *, sysarg_t *);
static errno_t amdm37x_ddev_get_info(void *, ddev_info_t *);

static errno_t amdm37x_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t amdm37x_gc_set_color(void *, gfx_color_t *);
static errno_t amdm37x_gc_fill_rect(void *, gfx_rect_t *);
static errno_t amdm37x_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t amdm37x_gc_bitmap_destroy(void *);
static errno_t amdm37x_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t amdm37x_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

ddev_ops_t amdm37x_ddev_ops = {
	.get_gc = amdm37x_ddev_get_gc,
	.get_info = amdm37x_ddev_get_info
};

gfx_context_ops_t amdm37x_gc_ops = {
	.set_clip_rect = amdm37x_gc_set_clip_rect,
	.set_color = amdm37x_gc_set_color,
	.fill_rect = amdm37x_gc_fill_rect,
	.bitmap_create = amdm37x_gc_bitmap_create,
	.bitmap_destroy = amdm37x_gc_bitmap_destroy,
	.bitmap_render = amdm37x_gc_bitmap_render,
	.bitmap_get_alloc = amdm37x_gc_bitmap_get_alloc
};

static const struct {
	unsigned bpp;
	pixel2visual_t func;
} pixel2visual_table[] = {
	[VISUAL_INDIRECT_8] = { .bpp = 1, .func = pixel2bgr_323 },
	[VISUAL_RGB_5_5_5_LE] = { .bpp = 2, .func = pixel2rgb_555_le },
	[VISUAL_RGB_5_5_5_BE] = { .bpp = 2, .func = pixel2rgb_555_be },
	[VISUAL_RGB_5_6_5_LE] = { .bpp = 2, .func = pixel2rgb_565_le },
	[VISUAL_RGB_5_6_5_BE] = { .bpp = 2, .func = pixel2rgb_565_be },
	[VISUAL_BGR_8_8_8] = { .bpp = 3, .func = pixel2bgr_888 },
	[VISUAL_RGB_8_8_8] = { .bpp = 3, .func = pixel2rgb_888 },
	[VISUAL_BGR_0_8_8_8] = { .bpp = 4, .func = pixel2rgb_0888 },
	[VISUAL_BGR_8_8_8_0] = { .bpp = 4, .func = pixel2bgr_8880 },
	[VISUAL_ABGR_8_8_8_8] = { .bpp = 4, .func = pixel2abgr_8888 },
	[VISUAL_BGRA_8_8_8_8] = { .bpp = 4, .func = pixel2bgra_8888 },
	[VISUAL_RGB_0_8_8_8] = { .bpp = 4, .func = pixel2rgb_0888 },
	[VISUAL_RGB_8_8_8_0] = { .bpp = 4, .func = pixel2rgb_8880 },
	[VISUAL_ARGB_8_8_8_8] = { .bpp = 4, .func = pixel2argb_8888 },
	[VISUAL_RGBA_8_8_8_8] = { .bpp = 4, .func = pixel2rgba_8888 },
};

errno_t amdm37x_dispc_init(amdm37x_dispc_t *instance, ddf_fun_t *fun)
{
	instance->fun = fun;
	instance->fb_data = NULL;
	instance->size = 0;

	/* Default is 24bpp, use config option if available */
	visual_t visual = VISUAL_BGR_8_8_8;
	switch (CONFIG_BFB_BPP)	{
	case 8:
		visual = VISUAL_INDIRECT_8;
		break;
	case 16:
		visual = VISUAL_RGB_5_6_5_LE;
		break;
	case 24:
		visual = VISUAL_BGR_8_8_8;
		break;
	case 32:
		visual = VISUAL_RGB_8_8_8_0;
		break;
	default:
		return EINVAL;
	}

	errno_t ret = pio_enable((void *)AMDM37x_DISPC_BASE_ADDRESS,
	    AMDM37x_DISPC_SIZE, (void **)&instance->regs);
	if (ret != EOK) {
		return EIO;
	}

	ret = amdm37x_change_mode(instance, CONFIG_BFB_WIDTH,
	    CONFIG_BFB_HEIGHT, visual);
	if (ret != EOK)
		return EIO;

	return EOK;
}

errno_t amdm37x_dispc_fini(amdm37x_dispc_t *instance)
{
	return EOK;
}

static errno_t amdm37x_dispc_setup_fb(amdm37x_dispc_regs_t *regs,
    unsigned x, unsigned y, unsigned bpp, uint32_t pa)
{
	assert(regs);
	/*
	 * Init sequence for dispc is in chapter 7.6.5.1.4 p. 1810,
	 * no idea what parts of that work.
	 */

	/* Disable all interrupts */
	regs->irqenable = 0;

	/* Pixel format specifics */
	uint32_t attrib_pixel_format = 0;
	uint32_t control_data_lanes = 0;
	switch (bpp) {
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
	    (((x - 1) & AMDM37X_DISPC_SIZE_WIDTH_MASK) <<
	    AMDM37X_DISPC_SIZE_WIDTH_SHIFT) |
	    (((y - 1) & AMDM37X_DISPC_SIZE_HEIGHT_MASK) <<
	    AMDM37X_DISPC_SIZE_HEIGHT_SHIFT);

	/* modes taken from u-boot, for 1024x768 */
	// TODO replace magic values with actual correct values
#if 0
	regs->timing_h = 0x1a4024c9;
	regs->timing_v = 0x02c00509;
	regs->pol_freq = 0x00007028;
	regs->divisor  = 0x00010001;
#endif

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
	uint32_t config = (AMDM37X_DISPC_CONFIG_LOADMODE_DATAEVERYFRAME <<
	    AMDM37X_DISPC_CONFIG_LOADMODE_SHIFT);
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
	/*
	 * This value should be stride - width, 1 means next pixel i.e.
	 * stride == width
	 */
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
	return EOK;
}

static errno_t amdm37x_change_mode(amdm37x_dispc_t *dispc, unsigned x,
    unsigned y, visual_t visual)
{
	assert((size_t)visual < sizeof(pixel2visual_table) / sizeof(pixel2visual_table[0]));
	const unsigned bpp = pixel2visual_table[visual].bpp;
	pixel2visual_t p2v = pixel2visual_table[visual].func;
	ddf_log_note("Setting mode: %ux%ux%u\n", x, y, bpp * 8);
	const size_t size = ALIGN_UP(x * y * bpp, PAGE_SIZE);
	uintptr_t pa;
	void *buffer = AS_AREA_ANY;
	errno_t ret = dmamem_map_anonymous(size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &pa, &buffer);
	if (ret != EOK) {
		ddf_log_error("Failed to get new FB\n");
		return ret;
	}
	if (dispc->fb_data)
		dmamem_unmap_anonymous(dispc->fb_data);

	dispc->fb_data = buffer;
	amdm37x_dispc_setup_fb(dispc->regs, x, y, bpp * 8, (uint32_t)pa);
	dispc->active_fb.width = x;
	dispc->active_fb.height = y;
	dispc->active_fb.pitch = 0;
	dispc->active_fb.bpp = bpp;
	dispc->active_fb.pixel2visual = p2v;
	dispc->rect.p0.x = 0;
	dispc->rect.p0.y = 0;
	dispc->rect.p1.x = x;
	dispc->rect.p1.y = y;
	dispc->clip_rect = dispc->rect;
	dispc->size = size;

	return EOK;
}

#define FB_POS(d, x, y) \
    (((y) * ((d)->active_fb.width + (d)->active_fb.pitch) + (x)) \
    * (d)->active_fb.bpp)

static errno_t amdm37x_ddev_get_gc(void *arg, sysarg_t *arg2, sysarg_t *arg3)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;

	*arg2 = ddf_fun_get_handle(dispc->fun);
	*arg3 = 42;
	return EOK;
}

static errno_t amdm37x_ddev_get_info(void *arg, ddev_info_t *info)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;

	ddev_info_init(info);
	info->rect.p0.x = 0;
	info->rect.p0.y = 0;
	info->rect.p1.x = dispc->active_fb.width;
	info->rect.p1.y = dispc->active_fb.height;
	return EOK;
}

/** Set clipping rectangle on AMDM37x display controller.
 *
 * @param arg AMDM37x display controller
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;

	if (rect != NULL)
		gfx_rect_clip(rect, &dispc->rect, &dispc->clip_rect);
	else
		dispc->clip_rect = dispc->rect;

	return EOK;
}

/** Set color on AMDM37x display controller.
 *
 * Set drawing color on AMDM37x GC.
 *
 * @param arg AMDM37x display controller
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_set_color(void *arg, gfx_color_t *color)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;
	uint16_t r, g, b;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	dispc->color = PIXEL(0, r >> 8, g >> 8, b >> 8);
	return EOK;
}

/** Fill rectangle on AMDM37x display controller.
 *
 * @param arg AMDM37x display controller
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;
	gfx_rect_t crect;
	gfx_coord_t x, y;

	/* Make sure we have a sorted, clipped rectangle */
	gfx_rect_clip(rect, &dispc->clip_rect, &crect);

	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			dispc->active_fb.pixel2visual(dispc->fb_data +
			    FB_POS(dispc, x, y), dispc->color);
		}
	}

	return EOK;
}

/** Create bitmap in AMDM37x GC.
 *
 * @param arg AMDM37x display controller
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t amdm37x_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	amdm37x_dispc_t *dispc = (amdm37x_dispc_t *) arg;
	amdm37x_bitmap_t *dcbm = NULL;
	gfx_coord2_t dim;
	errno_t rc;

	/* Check that we support all required flags */
	if ((params->flags & ~(bmpf_color_key | bmpf_colorize)) != 0)
		return ENOTSUP;

	dcbm = calloc(1, sizeof(amdm37x_bitmap_t));
	if (dcbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	dcbm->rect = params->rect;
	dcbm->flags = params->flags;

	if (alloc == NULL) {
		dcbm->alloc.pitch = dim.x * sizeof(uint32_t);
		dcbm->alloc.off0 = 0;
		dcbm->alloc.pixels = malloc(dcbm->alloc.pitch * dim.y);
		dcbm->myalloc = true;

		if (dcbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}
	} else {
		dcbm->alloc = *alloc;
	}

	dcbm->dispc = dispc;
	*rbm = (void *)dcbm;
	return EOK;
error:
	if (rbm != NULL)
		free(dcbm);
	return rc;
}

/** Destroy bitmap in AMDM37x GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_bitmap_destroy(void *bm)
{
	amdm37x_bitmap_t *dcbm = (amdm37x_bitmap_t *)bm;
	if (dcbm->myalloc)
		free(dcbm->alloc.pixels);
	free(dcbm);
	return EOK;
}

/** Render bitmap in AMDM37x GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	amdm37x_bitmap_t *dcbm = (amdm37x_bitmap_t *)bm;
	amdm37x_dispc_t *dispc = dcbm->dispc;
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_rect_t skfbrect;
	gfx_rect_t crect;
	gfx_coord2_t offs;
	gfx_coord2_t bmdim;
	gfx_coord2_t dim;
	gfx_coord2_t sp;
	gfx_coord2_t dp;
	gfx_coord2_t pos;
	pixelmap_t pbm;
	pixel_t color;

	/* Clip source rectangle to bitmap bounds */

	if (srect0 != NULL)
		gfx_rect_clip(srect0, &dcbm->rect, &srect);
	else
		srect = dcbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);
	gfx_coord2_subtract(&drect.p1, &drect.p0, &dim);
	gfx_coord2_subtract(&dcbm->rect.p1, &dcbm->rect.p0, &bmdim);

	pbm.width = bmdim.x;
	pbm.height = bmdim.y;
	pbm.data = dcbm->alloc.pixels;

	/* Transform AMDM37x clipping rectangle back to bitmap coordinate system */
	gfx_rect_rtranslate(&offs, &dispc->clip_rect, &skfbrect);

	/*
	 * Make sure we have a sorted source rectangle, clipped so that
	 * destination lies within AMDM37x bounding rectangle
	 */
	gfx_rect_clip(&srect, &skfbrect, &crect);

	if ((dcbm->flags & bmpf_color_key) == 0) {
		/* Simple copy */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &dcbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				dispc->active_fb.pixel2visual(dispc->fb_data +
				    FB_POS(dispc, dp.x, dp.y), color);
			}
		}
	} else if ((dcbm->flags & bmpf_colorize) == 0) {
		/* Color key */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &dcbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				if (color != dcbm->key_color) {
					dispc->active_fb.pixel2visual(dispc->fb_data +
					    FB_POS(dispc, dp.x, dp.y), color);
				}
			}
		}
	} else {
		/* Color key & colorize */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &dcbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				if (color != dcbm->key_color) {
					dispc->active_fb.pixel2visual(dispc->fb_data +
					    FB_POS(dispc, dp.x, dp.y),
					    dcbm->dispc->color);
				}
			}
		}
	}

	return EOK;
}

/** Get allocation info for bitmap in AMDM37x GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t amdm37x_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	amdm37x_bitmap_t *dcbm = (amdm37x_bitmap_t *)bm;
	*alloc = dcbm->alloc;
	return EOK;
}

/**
 * @}
 */
