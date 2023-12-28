/*
 * Copyright (c) 2023 Jiri Svoboda
 * Copyright (c) 2013 Martin Sucha
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

#include <ddev/info.h>
#include <ddev_srv.h>
#include <errno.h>
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <inttypes.h>
#include <io/log.h>
#include <io/pixelmap.h>
#include <ipcgfx/server.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "rfb.h"

#define NAME "rfb"

static errno_t rfb_ddev_get_gc(void *, sysarg_t *, sysarg_t *);
static errno_t rfb_ddev_get_info(void *, ddev_info_t *);

static errno_t rfb_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t rfb_gc_set_color(void *, gfx_color_t *);
static errno_t rfb_gc_fill_rect(void *, gfx_rect_t *);
static errno_t rfb_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t rfb_gc_bitmap_destroy(void *);
static errno_t rfb_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t rfb_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static ddev_ops_t rfb_ddev_ops = {
	.get_gc = rfb_ddev_get_gc,
	.get_info = rfb_ddev_get_info
};

typedef struct {
	rfb_t rfb;
	pixel_t color;
	gfx_rect_t rect;
	gfx_rect_t clip_rect;
} rfb_gc_t;

typedef struct {
	rfb_gc_t *rfb;
	gfx_bitmap_alloc_t alloc;
	gfx_rect_t rect;
	gfx_bitmap_flags_t flags;
	pixel_t key_color;
	bool myalloc;
} rfb_bitmap_t;

static gfx_context_ops_t rfb_gc_ops = {
	.set_clip_rect = rfb_gc_set_clip_rect,
	.set_color = rfb_gc_set_color,
	.fill_rect = rfb_gc_fill_rect,
	.bitmap_create = rfb_gc_bitmap_create,
	.bitmap_destroy = rfb_gc_bitmap_destroy,
	.bitmap_render = rfb_gc_bitmap_render,
	.bitmap_get_alloc = rfb_gc_bitmap_get_alloc
};

static void rfb_gc_invalidate_rect(rfb_gc_t *rfbgc, gfx_rect_t *rect)
{
	rfb_t *rfb = &rfbgc->rfb;
	gfx_rect_t old_rect;
	gfx_rect_t new_rect;

	if (gfx_rect_is_empty(rect))
		return;

	if (!rfb->damage_valid) {
		old_rect.p0.x = old_rect.p0.y = 0;
		old_rect.p1.x = old_rect.p1.y = 0;
	} else {
		old_rect.p0.x = rfb->damage_rect.x;
		old_rect.p0.y = rfb->damage_rect.y;
		old_rect.p1.x = rfb->damage_rect.x + rfb->damage_rect.width;
		old_rect.p1.y = rfb->damage_rect.y + rfb->damage_rect.height;
	}

	gfx_rect_envelope(&old_rect, rect, &new_rect);

	rfb->damage_rect.x = new_rect.p0.x;
	rfb->damage_rect.y = new_rect.p0.y;
	rfb->damage_rect.width = new_rect.p1.x - new_rect.p0.x;
	rfb->damage_rect.height = new_rect.p1.y - new_rect.p0.y;
}

static errno_t rfb_ddev_get_gc(void *arg, sysarg_t *arg2, sysarg_t *arg3)
{
	*arg2 = 0;
	*arg3 = 42;
	return EOK;
}

static errno_t rfb_ddev_get_info(void *arg, ddev_info_t *info)
{
	rfb_t *rfb = (rfb_t *) arg;

	ddev_info_init(info);

	info->rect.p0.x = 0;
	info->rect.p0.y = 0;
	info->rect.p1.x = rfb->width;
	info->rect.p1.y = rfb->height;

	return EOK;
}

/** Create RFB GC.
 *
 * @param rrgb Place to store pointer to new RFB GC
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t rgb_gc_create(rfb_gc_t **rrfb)
{
	rfb_gc_t *rfb;

	rfb = calloc(1, sizeof(rfb_gc_t));
	if (rfb == NULL)
		return ENOMEM;

	*rrfb = rfb;
	return EOK;
}

/** Destroy RFB GC.
 *
 * @param rfb RFB GC
 */
static void rfb_gc_destroy(rfb_gc_t *rfb)
{
	free(rfb);
}

/** Set clipping rectangle on RFB.
 *
 * @param arg RFB
 * @param rect Rectangle or @c NULL
 *
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	rfb_gc_t *rfb = (rfb_gc_t *) arg;

	if (rect != NULL)
		gfx_rect_clip(rect, &rfb->rect, &rfb->clip_rect);
	else
		rfb->clip_rect = rfb->rect;

	return EOK;
}

/** Set color on RFB.
 *
 * Set drawing color on RFB GC.
 *
 * @param arg RFB
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_set_color(void *arg, gfx_color_t *color)
{
	rfb_gc_t *rfb = (rfb_gc_t *) arg;
	uint16_t r, g, b;

	gfx_color_get_rgb_i16(color, &r, &g, &b);
	rfb->color = PIXEL(0, r >> 8, g >> 8, b >> 8);
	return EOK;
}

/** Fill rectangle on RFB.
 *
 * @param arg RFB
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	rfb_gc_t *rfb = (rfb_gc_t *) arg;
	gfx_rect_t crect;
	gfx_coord_t x, y;

	gfx_rect_clip(rect, &rfb->clip_rect, &crect);

	for (y = crect.p0.y; y < crect.p1.y; y++) {
		for (x = crect.p0.x; x < crect.p1.x; x++) {
			pixelmap_put_pixel(&rfb->rfb.framebuffer, x, y,
			    rfb->color);
		}
	}

	rfb_gc_invalidate_rect(rfb, &crect);

	return EOK;
}

/** Create bitmap in RFB GC.
 *
 * @param arg RFB
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t rfb_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	rfb_gc_t *rfb = (rfb_gc_t *) arg;
	rfb_bitmap_t *rfbbm = NULL;
	gfx_coord2_t dim;
	errno_t rc;

	/* Check that we support all required flags */
	if ((params->flags & ~(bmpf_color_key | bmpf_colorize)) != 0)
		return ENOTSUP;

	rfbbm = calloc(1, sizeof(rfb_bitmap_t));
	if (rfbbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	rfbbm->rect = params->rect;
	rfbbm->flags = params->flags;
	rfbbm->key_color = params->key_color;

	if (alloc == NULL) {
		rfbbm->alloc.pitch = dim.x * sizeof(uint32_t);
		rfbbm->alloc.off0 = 0;
		rfbbm->alloc.pixels = malloc(rfbbm->alloc.pitch * dim.y);
		rfbbm->myalloc = true;

		if (rfbbm->alloc.pixels == NULL) {
			rc = ENOMEM;
			goto error;
		}
	} else {
		rfbbm->alloc = *alloc;
	}

	rfbbm->rfb = rfb;
	*rbm = (void *)rfbbm;
	return EOK;
error:
	if (rbm != NULL)
		free(rfbbm);
	return rc;
}

/** Destroy bitmap in RFB GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_bitmap_destroy(void *bm)
{
	rfb_bitmap_t *rfbbm = (rfb_bitmap_t *)bm;
	if (rfbbm->myalloc)
		free(rfbbm->alloc.pixels);
	free(rfbbm);
	return EOK;
}

/** Render bitmap in RFB GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	rfb_bitmap_t *rfbbm = (rfb_bitmap_t *)bm;
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_rect_t crect;
	gfx_coord2_t offs;
	gfx_coord2_t bmdim;
	gfx_coord2_t dim;
	gfx_coord_t x, y;
	pixelmap_t pbm;
	pixel_t color;

	if (srect0 != NULL)
		srect = *srect0;
	else
		srect = rfbbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);
	gfx_rect_clip(&drect, &rfbbm->rfb->clip_rect, &crect);
	gfx_coord2_subtract(&crect.p1, &crect.p0, &dim);
	gfx_coord2_subtract(&rfbbm->rect.p1, &rfbbm->rect.p0, &bmdim);

	pbm.width = bmdim.x;
	pbm.height = bmdim.y;
	pbm.data = rfbbm->alloc.pixels;

	if ((rfbbm->flags & bmpf_color_key) == 0) {
		/* Simple copy */
		for (y = srect.p0.y; y < srect.p1.y; y++) {
			for (x = srect.p0.x; x < srect.p1.x; x++) {
				color = pixelmap_get_pixel(&pbm, x, y);
				pixelmap_put_pixel(&rfbbm->rfb->rfb.framebuffer,
				    x + offs.x, y + offs.y, color);
			}
		}
	} else if ((rfbbm->flags & bmpf_colorize) == 0) {
		/* Color key */
		for (y = srect.p0.y; y < srect.p1.y; y++) {
			for (x = srect.p0.x; x < srect.p1.x; x++) {
				color = pixelmap_get_pixel(&pbm, x, y);
				if (color != rfbbm->key_color) {
					pixelmap_put_pixel(&rfbbm->rfb->rfb.framebuffer,
					    x + offs.x, y + offs.y, color);
				}
			}
		}
	} else {
		/* Color key & colorization */
		for (y = srect.p0.y; y < srect.p1.y; y++) {
			for (x = srect.p0.x; x < srect.p1.x; x++) {
				color = pixelmap_get_pixel(&pbm, x, y);
				if (color != rfbbm->key_color) {
					pixelmap_put_pixel(&rfbbm->rfb->rfb.framebuffer,
					    x + offs.x, y + offs.y,
					    rfbbm->rfb->color);
				}
			}
		}
	}

	rfb_gc_invalidate_rect(rfbbm->rfb, &crect);

	return EOK;
}

/** Get allocation info for bitmap in RFB GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t rfb_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	rfb_bitmap_t *rfbbm = (rfb_bitmap_t *)bm;
	*alloc = rfbbm->alloc;
	return EOK;
}

static void syntax_print(void)
{
	fprintf(stderr, "Usage: %s <name> <width> <height> [port]\n", NAME);
}

static void client_connection(ipc_call_t *icall, void *arg)
{
	rfb_t *rfb = (rfb_t *) arg;
	rfb_gc_t *rfbgc;
	ddev_srv_t srv;
	sysarg_t svc_id;
	gfx_context_t *gc;
	errno_t rc;

	svc_id = ipc_get_arg2(icall);

	if (svc_id != 0) {
		/* Set up protocol structure */
		ddev_srv_initialize(&srv);
		srv.ops = &rfb_ddev_ops;
		srv.arg = (void *) rfb;

		/* Handle connection */
		ddev_conn(icall, &srv);
	} else {
		rc = rgb_gc_create(&rfbgc);
		if (rc != EOK) {
			async_answer_0(icall, ENOMEM);
			return;
		}

		rfbgc->rect.p0.x = 0;
		rfbgc->rect.p0.y = 0;
		rfbgc->rect.p1.x = rfb->width;
		rfbgc->rect.p1.y = rfb->height;
		rfbgc->clip_rect = rfbgc->rect;

		rc = gfx_context_new(&rfb_gc_ops, (void *) rfbgc, &gc);
		if (rc != EOK) {
			rfb_gc_destroy(rfbgc);
			async_answer_0(icall, ENOMEM);
			return;
		}

		/* GC connection */
		gc_conn(icall, gc);
	}
}

int main(int argc, char **argv)
{
	rfb_t rfb;
	loc_srv_t *srv;

	log_init(NAME);

	if (argc <= 3) {
		syntax_print();
		return 1;
	}

	const char *rfb_name = argv[1];

	char *endptr;
	unsigned long width = strtoul(argv[2], &endptr, 0);
	if (*endptr != 0) {
		fprintf(stderr, "Invalid width\n");
		syntax_print();
		return 1;
	}

	unsigned long height = strtoul(argv[3], &endptr, 0);
	if (*endptr != 0) {
		fprintf(stderr, "Invalid height\n");
		syntax_print();
		return 1;
	}

	unsigned long port = 5900;
	if (argc > 4) {
		port = strtoul(argv[4], &endptr, 0);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid port number\n");
			syntax_print();
			return 1;
		}
	}

	rfb_init(&rfb, width, height, rfb_name);

	async_set_fallback_port_handler(client_connection, &rfb);

	errno_t rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		printf("%s: Unable to register server.\n", NAME);
		return rc;
	}

	char *service_name;
	rc = asprintf(&service_name, "rfb/%s", rfb_name);
	if (rc < 0) {
		printf(NAME ": Unable to create service name\n");
		return rc;
	}

	service_id_t service_id;

	rc = loc_service_register(srv, service_name, &service_id);
	if (rc != EOK) {
		printf(NAME ": Unable to register service %s.\n", service_name);
		return rc;
	}

	free(service_name);

	category_id_t ddev_cid;
	rc = loc_category_get_id("display-device", &ddev_cid, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to get display device category id.\n");
		return 1;
	}

	rc = loc_service_add_to_cat(srv, service_id, ddev_cid);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to add service to display device category.\n");
		return 1;
	}

	rc = rfb_listen(&rfb, port);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to listen at rfb port\n");
		return 2;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}
