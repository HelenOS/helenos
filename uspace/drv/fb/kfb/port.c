/*
 * Copyright (c) 2021 Jiri Svoboda
 * Copyright (c) 2006 Jakub Vana
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup kfb
 * @{
 */
/**
 * @file
 */

#include <abi/fb/visuals.h>
#include <adt/list.h>
#include <align.h>
#include <as.h>
#include <ddev_srv.h>
#include <ddev/info.h>
#include <ddi.h>
#include <ddf/log.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <io/pixelmap.h>
#include <ipcgfx/server.h>
#include <mem.h>
#include <pixconv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sysinfo.h>

#include "kfb.h"
#include "port.h"

#define FB_POS(fb, x, y)  ((y) * (fb)->scanline + (x) * (fb)->pixel_bytes)

typedef struct {
	ddf_fun_t *fun;

	sysarg_t paddr;
	gfx_rect_t rect;
	gfx_rect_t clip_rect;
	size_t offset;
	size_t scanline;
	visual_t visual;

	pixel2visual_t pixel2visual;
	visual2pixel_t visual2pixel;
	visual_mask_t visual_mask;
	size_t pixel_bytes;

	size_t size;
	uint8_t *addr;

	/** Current drawing color */
	pixel_t color;
} kfb_t;

typedef struct {
	kfb_t *kfb;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t rect;
	gfx_bitmap_flags_t flags;
	pixel_t key_color;
	bool myalloc;
} kfb_bitmap_t;

static errno_t kfb_ddev_get_gc(void *, sysarg_t *, sysarg_t *);
static errno_t kfb_ddev_get_info(void *, ddev_info_t *);

static errno_t kfb_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t kfb_gc_set_color(void *, gfx_color_t *);
static errno_t kfb_gc_fill_rect(void *, gfx_rect_t *);
static errno_t kfb_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t kfb_gc_bitmap_destroy(void *);
static errno_t kfb_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t kfb_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static ddev_ops_t kfb_ddev_ops = {
	.get_gc = kfb_ddev_get_gc,
	.get_info = kfb_ddev_get_info
};

static gfx_context_ops_t kfb_gc_ops = {
	.set_clip_rect = kfb_gc_set_clip_rect,
	.set_color = kfb_gc_set_color,
	.fill_rect = kfb_gc_fill_rect,
	.bitmap_create = kfb_gc_bitmap_create,
	.bitmap_destroy = kfb_gc_bitmap_destroy,
	.bitmap_render = kfb_gc_bitmap_render,
	.bitmap_get_alloc = kfb_gc_bitmap_get_alloc
};

static errno_t kfb_ddev_get_gc(void *arg, sysarg_t *arg2, sysarg_t *arg3)
{
	kfb_t *kfb = (kfb_t *) arg;

	*arg2 = ddf_fun_get_handle(kfb->fun);
	*arg3 = 42;
	return EOK;
}

static errno_t kfb_ddev_get_info(void *arg, ddev_info_t *info)
{
	kfb_t *kfb = (kfb_t *) arg;

	ddev_info_init(info);
	info->rect = kfb->rect;
	return EOK;
}

/** Set clipping rectangle on KFB.
 *
 * @param arg KFB
 * @param rect Rectangle or @c NULL
 *
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	kfb_t *kfb = (kfb_t *) arg;

	if (rect != NULL)
		gfx_rect_clip(rect, &kfb->rect, &kfb->clip_rect);
	else
		kfb->clip_rect = kfb->rect;

	return EOK;
}

/** Set color on KFB.
 *
 * Set drawing color on KFB GC.
 *
 * @param arg KFB
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_set_color(void *arg, gfx_color_t *color)
{
	kfb_t *kfb = (kfb_t *) arg;
	uint16_t r, g, b;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	kfb->color = PIXEL(0, r >> 8, g >> 8, b >> 8);
	return EOK;
}

/** Fill rectangle on KFB.
 *
 * @param arg KFB
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	kfb_t *kfb = (kfb_t *) arg;
	gfx_rect_t crect;
	gfx_coord_t x, y;

	/* Make sure we have a sorted, clipped rectangle */
	gfx_rect_clip(rect, &kfb->rect, &crect);

	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			kfb->pixel2visual(kfb->addr + FB_POS(kfb, x, y),
			    kfb->color);
		}
	}

	return EOK;
}

/** Create bitmap in KFB GC.
 *
 * @param arg KFB
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t kfb_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	kfb_t *kfb = (kfb_t *) arg;
	kfb_bitmap_t *kfbbm = NULL;
	gfx_coord2_t dim;
	errno_t rc;

	/* Check that we support all required flags */
	if ((params->flags & ~(bmpf_color_key | bmpf_colorize)) != 0)
		return ENOTSUP;

	kfbbm = calloc(1, sizeof(kfb_bitmap_t));
	if (kfbbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	kfbbm->rect = params->rect;
	kfbbm->flags = params->flags;
	kfbbm->key_color = params->key_color;

	if (alloc == NULL) {
		kfbbm->alloc.pitch = dim.x * sizeof(uint32_t);
		kfbbm->alloc.off0 = 0;
		kfbbm->alloc.pixels = malloc(kfbbm->alloc.pitch * dim.y);
		kfbbm->myalloc = true;

		if (kfbbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}
	} else {
		kfbbm->alloc = *alloc;
	}

	kfbbm->kfb = kfb;
	*rbm = (void *)kfbbm;
	return EOK;
error:
	if (rbm != NULL)
		free(kfbbm);
	return rc;
}

/** Destroy bitmap in KFB GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_bitmap_destroy(void *bm)
{
	kfb_bitmap_t *kfbbm = (kfb_bitmap_t *)bm;
	if (kfbbm->myalloc)
		free(kfbbm->alloc.pixels);
	free(kfbbm);
	return EOK;
}

/** Render bitmap in KFB GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	kfb_bitmap_t *kfbbm = (kfb_bitmap_t *)bm;
	kfb_t *kfb = kfbbm->kfb;
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
		gfx_rect_clip(srect0, &kfbbm->rect, &srect);
	else
		srect = kfbbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);
	gfx_coord2_subtract(&drect.p1, &drect.p0, &dim);
	gfx_coord2_subtract(&kfbbm->rect.p1, &kfbbm->rect.p0, &bmdim);

	pbm.width = bmdim.x;
	pbm.height = bmdim.y;
	pbm.data = kfbbm->alloc.pixels;

	/* Transform KFB bounding rectangle back to bitmap coordinate system */
	gfx_rect_rtranslate(&offs, &kfb->rect, &skfbrect);

	/*
	 * Make sure we have a sorted source rectangle, clipped so that
	 * destination lies within KFB bounding rectangle
	 */
	gfx_rect_clip(&srect, &skfbrect, &crect);

	if ((kfbbm->flags & bmpf_color_key) != 0) {
		/* Simple copy */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &kfbbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				if (color != kfbbm->key_color) {
					kfb->pixel2visual(kfb->addr +
					    FB_POS(kfb, dp.x, dp.y), color);
				}
			}
		}
	} else if ((kfbbm->flags & bmpf_colorize) != 0) {
		/* Color key */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &kfbbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				kfb->pixel2visual(kfb->addr +
				    FB_POS(kfb, dp.x, dp.y), color);
			}
		}
	} else {
		/* Color key & colorize */
		for (pos.y = crect.p0.y; pos.y < crect.p1.y; pos.y++) {
			for (pos.x = crect.p0.x; pos.x < crect.p1.x; pos.x++) {
				gfx_coord2_subtract(&pos, &kfbbm->rect.p0, &sp);
				gfx_coord2_add(&pos, &offs, &dp);

				color = pixelmap_get_pixel(&pbm, sp.x, sp.y);
				kfb->pixel2visual(kfb->addr +
				    FB_POS(kfb, dp.x, dp.y), color);
			}
		}
	}

	return EOK;
}

/** Get allocation info for bitmap in KFB GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t kfb_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	kfb_bitmap_t *kfbbm = (kfb_bitmap_t *)bm;
	*alloc = kfbbm->alloc;
	return EOK;
}

static void kfb_client_conn(ipc_call_t *icall, void *arg)
{
	kfb_t *kfb;
	ddev_srv_t srv;
	sysarg_t gc_id;
	gfx_context_t *gc;
	errno_t rc;

	kfb = (kfb_t *) ddf_fun_data_get((ddf_fun_t *) arg);

	gc_id = ipc_get_arg3(icall);

	if (gc_id == 0) {
		/* Set up protocol structure */
		ddev_srv_initialize(&srv);
		srv.ops = &kfb_ddev_ops;
		srv.arg = kfb;

		/* Handle connection */
		ddev_conn(icall, &srv);
	} else {
		assert(gc_id == 42);

		if (kfb->addr != AS_AREA_ANY) {
			/* This means there already is a GC connection */
			async_answer_0(icall, EBUSY);
			return;
		}

		rc = physmem_map(kfb->paddr + kfb->offset,
		    ALIGN_UP(kfb->size, PAGE_SIZE) >> PAGE_WIDTH,
		    AS_AREA_READ | AS_AREA_WRITE, (void *) &kfb->addr);
		if (rc != EOK)
			goto error;

		rc = gfx_context_new(&kfb_gc_ops, kfb, &gc);
		if (rc != EOK)
			goto error;

		/* GC connection */
		gc_conn(icall, gc);

		rc = physmem_unmap(kfb->addr);
		if (rc == EOK)
			kfb->addr = AS_AREA_ANY;
	}

	return;
error:
	if (kfb->addr != AS_AREA_ANY) {
		if (physmem_unmap(kfb->addr) == EOK)
			kfb->addr = AS_AREA_ANY;
	}

	async_answer_0(icall, rc);
}

errno_t port_init(ddf_dev_t *dev)
{
	ddf_fun_t *fun = NULL;
	kfb_t *kfb = NULL;
	errno_t rc;

	fun = ddf_fun_create(dev, fun_exposed, "kfb");
	if (fun == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_conn_handler(fun, &kfb_client_conn);

	kfb = ddf_fun_data_alloc(fun, sizeof(kfb_t));
	if (kfb == NULL) {
		rc = ENOMEM;
		goto error;
	}

	sysarg_t present;
	rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;

	if (!present) {
		rc = ENOENT;
		goto error;
	}

	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;

	if (kind != 1) {
		rc = EINVAL;
		goto error;
	}

	sysarg_t paddr;
	rc = sysinfo_get_value("fb.address.physical", &paddr);
	if (rc != EOK)
		goto error;

	sysarg_t offset;
	rc = sysinfo_get_value("fb.offset", &offset);
	if (rc != EOK)
		offset = 0;

	sysarg_t width;
	rc = sysinfo_get_value("fb.width", &width);
	if (rc != EOK)
		goto error;

	sysarg_t height;
	rc = sysinfo_get_value("fb.height", &height);
	if (rc != EOK)
		goto error;

	sysarg_t scanline;
	rc = sysinfo_get_value("fb.scanline", &scanline);
	if (rc != EOK)
		goto error;

	sysarg_t visual;
	rc = sysinfo_get_value("fb.visual", &visual);
	if (rc != EOK)
		goto error;

	kfb->fun = fun;

	kfb->rect.p0.x = 0;
	kfb->rect.p0.y = 0;
	kfb->rect.p1.x = width;
	kfb->rect.p1.y = height;

	kfb->clip_rect = kfb->rect;

	kfb->paddr = paddr;
	kfb->offset = offset;
	kfb->scanline = scanline;
	kfb->visual = visual;

	switch (visual) {
	case VISUAL_INDIRECT_8:
		kfb->pixel2visual = pixel2bgr_323;
		kfb->visual2pixel = bgr_323_2pixel;
		kfb->visual_mask = visual_mask_323;
		kfb->pixel_bytes = 1;
		break;
	case VISUAL_RGB_5_5_5_LE:
		kfb->pixel2visual = pixel2rgb_555_le;
		kfb->visual2pixel = rgb_555_le_2pixel;
		kfb->visual_mask = visual_mask_555;
		kfb->pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_5_5_BE:
		kfb->pixel2visual = pixel2rgb_555_be;
		kfb->visual2pixel = rgb_555_be_2pixel;
		kfb->visual_mask = visual_mask_555;
		kfb->pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_LE:
		kfb->pixel2visual = pixel2rgb_565_le;
		kfb->visual2pixel = rgb_565_le_2pixel;
		kfb->visual_mask = visual_mask_565;
		kfb->pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_BE:
		kfb->pixel2visual = pixel2rgb_565_be;
		kfb->visual2pixel = rgb_565_be_2pixel;
		kfb->visual_mask = visual_mask_565;
		kfb->pixel_bytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		kfb->pixel2visual = pixel2rgb_888;
		kfb->visual2pixel = rgb_888_2pixel;
		kfb->visual_mask = visual_mask_888;
		kfb->pixel_bytes = 3;
		break;
	case VISUAL_BGR_8_8_8:
		kfb->pixel2visual = pixel2bgr_888;
		kfb->visual2pixel = bgr_888_2pixel;
		kfb->visual_mask = visual_mask_888;
		kfb->pixel_bytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		kfb->pixel2visual = pixel2rgb_8880;
		kfb->visual2pixel = rgb_8880_2pixel;
		kfb->visual_mask = visual_mask_8880;
		kfb->pixel_bytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		kfb->pixel2visual = pixel2rgb_0888;
		kfb->visual2pixel = rgb_0888_2pixel;
		kfb->visual_mask = visual_mask_0888;
		kfb->pixel_bytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		kfb->pixel2visual = pixel2bgr_0888;
		kfb->visual2pixel = bgr_0888_2pixel;
		kfb->visual_mask = visual_mask_0888;
		kfb->pixel_bytes = 4;
		break;
	case VISUAL_BGR_8_8_8_0:
		kfb->pixel2visual = pixel2bgr_8880;
		kfb->visual2pixel = bgr_8880_2pixel;
		kfb->visual_mask = visual_mask_8880;
		kfb->pixel_bytes = 4;
		break;
	default:
		return EINVAL;
	}

	kfb->size = scanline * height;
	kfb->addr = AS_AREA_ANY;

	rc = ddf_fun_bind(fun);
	if (rc != EOK)
		goto error;

	rc = ddf_fun_add_to_category(fun, "display-device");
	if (rc != EOK) {
		ddf_fun_unbind(fun);
		goto error;
	}

	return EOK;
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	return rc;
}

/** @}
 */
