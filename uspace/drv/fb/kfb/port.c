/*
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
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <stdlib.h>
#include <mem.h>
#include <as.h>
#include <align.h>

#include <sysinfo.h>
#include <ddi.h>

#include <adt/list.h>

#include <io/mode.h>
#include <io/pixelmap.h>
#include <io/chargrid.h>

#include <pixconv.h>

#include <graph.h>

#include "kfb.h"
#include "port.h"

#define FB_POS(x, y)  ((y) * kfb.scanline + (x) * kfb.pixel_bytes)

typedef struct {
	sysarg_t paddr;
	sysarg_t width;
	sysarg_t height;
	size_t offset;
	size_t scanline;
	visual_t visual;

	pixel2visual_t pixel2visual;
	visual2pixel_t visual2pixel;
	visual_mask_t visual_mask;
	size_t pixel_bytes;

	size_t size;
	uint8_t *addr;
} kfb_t;

static kfb_t kfb;

static vslmode_list_element_t pixel_mode;

static errno_t kfb_claim(visualizer_t *vs)
{
	return physmem_map(kfb.paddr + kfb.offset,
	    ALIGN_UP(kfb.size, PAGE_SIZE) >> PAGE_WIDTH,
	    AS_AREA_READ | AS_AREA_WRITE, (void *) &kfb.addr);
}

static errno_t kfb_yield(visualizer_t *vs)
{
	errno_t rc;

	if (vs->mode_set) {
		vs->ops.handle_damage = NULL;
	}

	rc = physmem_unmap(kfb.addr);
	if (rc != EOK)
		return rc;

	kfb.addr = NULL;
	return EOK;
}

static errno_t kfb_handle_damage_pixels(visualizer_t *vs,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset)
{
	pixelmap_t *map = &vs->cells;

	if (x_offset == 0 && y_offset == 0) {
		/* Faster damage routine ignoring offsets. */
		for (sysarg_t y = y0; y < height + y0; ++y) {
			pixel_t *pixel = pixelmap_pixel_at(map, x0, y);
			for (sysarg_t x = x0; x < width + x0; ++x) {
				kfb.pixel2visual(kfb.addr + FB_POS(x, y), *pixel++);
			}
		}
	} else {
		for (sysarg_t y = y0; y < height + y0; ++y) {
			for (sysarg_t x = x0; x < width + x0; ++x) {
				kfb.pixel2visual(kfb.addr + FB_POS(x, y),
				    *pixelmap_pixel_at(map,
				    (x + x_offset) % map->width,
				    (y + y_offset) % map->height));
			}
		}
	}

	return EOK;
}

static errno_t kfb_change_mode(visualizer_t *vs, vslmode_t new_mode)
{
	vs->ops.handle_damage = kfb_handle_damage_pixels;
	return EOK;
}

static errno_t kfb_suspend(visualizer_t *vs)
{
	return EOK;
}

static errno_t kfb_wakeup(visualizer_t *vs)
{
	return EOK;
}

static visualizer_ops_t kfb_ops = {
	.claim = kfb_claim,
	.yield = kfb_yield,
	.change_mode = kfb_change_mode,
	.handle_damage = NULL,
	.suspend = kfb_suspend,
	.wakeup = kfb_wakeup
};

static void graph_vsl_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	visualizer_t *vsl;
	errno_t rc;

	vsl = (visualizer_t *) ddf_fun_data_get((ddf_fun_t *)arg);
	graph_visualizer_connection(vsl, icall_handle, icall, NULL);

	if (kfb.addr != NULL) {
		rc = physmem_unmap(kfb.addr);
		if (rc == EOK)
			kfb.addr = NULL;
	}
}

errno_t port_init(ddf_dev_t *dev)
{
	sysarg_t present;
	errno_t rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;

	if (!present)
		return ENOENT;

	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;

	if (kind != 1)
		return EINVAL;

	sysarg_t paddr;
	rc = sysinfo_get_value("fb.address.physical", &paddr);
	if (rc != EOK)
		return rc;

	sysarg_t offset;
	rc = sysinfo_get_value("fb.offset", &offset);
	if (rc != EOK)
		offset = 0;

	sysarg_t width;
	rc = sysinfo_get_value("fb.width", &width);
	if (rc != EOK)
		return rc;

	sysarg_t height;
	rc = sysinfo_get_value("fb.height", &height);
	if (rc != EOK)
		return rc;

	sysarg_t scanline;
	rc = sysinfo_get_value("fb.scanline", &scanline);
	if (rc != EOK)
		return rc;

	sysarg_t visual;
	rc = sysinfo_get_value("fb.visual", &visual);
	if (rc != EOK)
		return rc;

	kfb.width = width;
	kfb.height = height;
	kfb.paddr = paddr;
	kfb.offset = offset;
	kfb.scanline = scanline;
	kfb.visual = visual;

	switch (visual) {
	case VISUAL_INDIRECT_8:
		kfb.pixel2visual = pixel2bgr_323;
		kfb.visual2pixel = bgr_323_2pixel;
		kfb.visual_mask = visual_mask_323;
		kfb.pixel_bytes = 1;
		break;
	case VISUAL_RGB_5_5_5_LE:
		kfb.pixel2visual = pixel2rgb_555_le;
		kfb.visual2pixel = rgb_555_le_2pixel;
		kfb.visual_mask = visual_mask_555;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_5_5_BE:
		kfb.pixel2visual = pixel2rgb_555_be;
		kfb.visual2pixel = rgb_555_be_2pixel;
		kfb.visual_mask = visual_mask_555;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_LE:
		kfb.pixel2visual = pixel2rgb_565_le;
		kfb.visual2pixel = rgb_565_le_2pixel;
		kfb.visual_mask = visual_mask_565;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_BE:
		kfb.pixel2visual = pixel2rgb_565_be;
		kfb.visual2pixel = rgb_565_be_2pixel;
		kfb.visual_mask = visual_mask_565;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		kfb.pixel2visual = pixel2rgb_888;
		kfb.visual2pixel = rgb_888_2pixel;
		kfb.visual_mask = visual_mask_888;
		kfb.pixel_bytes = 3;
		break;
	case VISUAL_BGR_8_8_8:
		kfb.pixel2visual = pixel2bgr_888;
		kfb.visual2pixel = bgr_888_2pixel;
		kfb.visual_mask = visual_mask_888;
		kfb.pixel_bytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		kfb.pixel2visual = pixel2rgb_8880;
		kfb.visual2pixel = rgb_8880_2pixel;
		kfb.visual_mask = visual_mask_8880;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		kfb.pixel2visual = pixel2rgb_0888;
		kfb.visual2pixel = rgb_0888_2pixel;
		kfb.visual_mask = visual_mask_0888;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		kfb.pixel2visual = pixel2bgr_0888;
		kfb.visual2pixel = bgr_0888_2pixel;
		kfb.visual_mask = visual_mask_0888;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_BGR_8_8_8_0:
		kfb.pixel2visual = pixel2bgr_8880;
		kfb.visual2pixel = bgr_8880_2pixel;
		kfb.visual_mask = visual_mask_8880;
		kfb.pixel_bytes = 4;
		break;
	default:
		return EINVAL;
	}

	kfb.size = scanline * height;
	kfb.addr = AS_AREA_ANY;

	ddf_fun_t *fun_vs = ddf_fun_create(dev, fun_exposed, "vsl0");
	if (fun_vs == NULL) {
		as_area_destroy(kfb.addr);
		return ENOMEM;
	}
	ddf_fun_set_conn_handler(fun_vs, &graph_vsl_connection);

	visualizer_t *vs = ddf_fun_data_alloc(fun_vs, sizeof(visualizer_t));
	if (vs == NULL) {
		as_area_destroy(kfb.addr);
		return ENOMEM;
	}
	graph_init_visualizer(vs);

	pixel_mode.mode.index = 0;
	pixel_mode.mode.version = 0;
	pixel_mode.mode.refresh_rate = 0;
	pixel_mode.mode.screen_aspect.width = width;
	pixel_mode.mode.screen_aspect.height = height;
	pixel_mode.mode.screen_width = width;
	pixel_mode.mode.screen_height = height;
	pixel_mode.mode.cell_aspect.width = 1;
	pixel_mode.mode.cell_aspect.height = 1;
	pixel_mode.mode.cell_visual.pixel_visual = visual;

	link_initialize(&pixel_mode.link);
	list_append(&pixel_mode.link, &vs->modes);

	vs->def_mode_idx = 0;

	vs->ops = kfb_ops;
	vs->dev_ctx = NULL;

	rc = ddf_fun_bind(fun_vs);
	if (rc != EOK) {
		list_remove(&pixel_mode.link);
		ddf_fun_destroy(fun_vs);
		as_area_destroy(kfb.addr);
		return rc;
	}

	vs->reg_svc_handle = ddf_fun_get_handle(fun_vs);
	ddf_fun_add_to_category(fun_vs, "visualizer");

	return EOK;
}

/** @}
 */
