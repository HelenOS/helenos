/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file GFX IPC server
 *
 * Serve a graphics context via HelenOS IPC.
 */

#include <as.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <ipcgfx/ipc/gc.h>
#include <ipcgfx/server.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "../private/server.h"

static ipc_gc_srv_bitmap_t *gc_bitmap_lookup(ipc_gc_srv_t *, sysarg_t);

static void gc_set_clip_rect_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	gfx_rect_t rect;
	errno_t rc;

	rect.p0.x = ipc_get_arg1(call);
	rect.p0.y = ipc_get_arg2(call);
	rect.p1.x = ipc_get_arg3(call);
	rect.p1.y = ipc_get_arg4(call);

	rc = gfx_set_clip_rect(srvgc->gc, &rect);
	async_answer_0(call, rc);
}

static void gc_set_clip_rect_null_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	errno_t rc;

	rc = gfx_set_clip_rect(srvgc->gc, NULL);
	async_answer_0(call, rc);
}

static void gc_set_rgb_color_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	uint16_t r, g, b;
	gfx_color_t *color;
	errno_t rc;

	r = ipc_get_arg1(call);
	g = ipc_get_arg2(call);
	b = ipc_get_arg3(call);

	rc = gfx_color_new_rgb_i16(r, g, b, &color);
	if (rc != EOK) {
		async_answer_0(call, ENOMEM);
		return;
	}

	rc = gfx_set_color(srvgc->gc, color);
	async_answer_0(call, rc);
}

static void gc_fill_rect_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	gfx_rect_t rect;
	errno_t rc;

	rect.p0.x = ipc_get_arg1(call);
	rect.p0.y = ipc_get_arg2(call);
	rect.p1.x = ipc_get_arg3(call);
	rect.p1.y = ipc_get_arg4(call);

	rc = gfx_fill_rect(srvgc->gc, &rect);
	async_answer_0(call, rc);
}

static void gc_update_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	errno_t rc;

	rc = gfx_update(srvgc->gc);
	async_answer_0(call, rc);
}

static void gc_bitmap_create_srv(ipc_gc_srv_t *srvgc, ipc_call_t *icall)
{
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *bitmap;
	gfx_coord2_t dim;
	ipc_gc_srv_bitmap_t *srvbmp = NULL;
	ipc_call_t call;
	size_t size;
	unsigned int flags;
	void *pixels;
	errno_t rc;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_bitmap_params_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &params, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Bitmap dimensions */
	gfx_coord2_subtract(&params.rect.p1, &params.rect.p0, &dim);

	if (!async_share_out_receive(&call, &size, &flags)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	/* Check size */
	if (size != PAGES2SIZE(SIZE2PAGES(dim.x * dim.y * sizeof(uint32_t)))) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_share_out_finalize(&call, &pixels);
	if (rc != EOK || pixels == AS_MAP_FAILED) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	alloc.pitch = dim.x * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = pixels;

	srvbmp = calloc(1, sizeof(ipc_gc_srv_bitmap_t));
	if (srvbmp == NULL) {
		as_area_destroy(pixels);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = gfx_bitmap_create(srvgc->gc, &params, &alloc, &bitmap);
	if (rc != EOK) {
		free(srvbmp);
		as_area_destroy(pixels);
		async_answer_0(icall, rc);
		return;
	}

	srvbmp->srvgc = srvgc;
	list_append(&srvbmp->lbitmaps, &srvgc->bitmaps);
	srvbmp->bmp = bitmap;
	srvbmp->bmp_id = srvgc->next_bmp_id++;

	/* We created the memory area by sharing it in */
	srvbmp->myalloc = true;
	srvbmp->pixels = pixels;

	async_answer_1(icall, EOK, srvbmp->bmp_id);
}

static void gc_bitmap_create_doutput_srv(ipc_gc_srv_t *srvgc, ipc_call_t *icall)
{
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *bitmap;
	gfx_coord2_t dim;
	ipc_gc_srv_bitmap_t *srvbmp = NULL;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_bitmap_params_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &params, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/* Bitmap dimensions */
	gfx_coord2_subtract(&params.rect.p1, &params.rect.p0, &dim);

	if (!async_share_in_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		return;
	}

	/* Check size */
	if (size != PAGES2SIZE(SIZE2PAGES(dim.x * dim.y * sizeof(uint32_t)))) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = gfx_bitmap_create(srvgc->gc, &params, NULL, &bitmap);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK) {
		gfx_bitmap_destroy(bitmap);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	rc = async_share_in_finalize(&call, alloc.pixels, AS_AREA_READ |
	    AS_AREA_WRITE | AS_AREA_CACHEABLE);
	if (rc != EOK) {
		gfx_bitmap_destroy(bitmap);
		async_answer_0(icall, EIO);
		return;
	}

	srvbmp = calloc(1, sizeof(ipc_gc_srv_bitmap_t));
	if (srvbmp == NULL) {
		gfx_bitmap_destroy(bitmap);
		async_answer_0(icall, ENOMEM);
		return;
	}

	srvbmp->srvgc = srvgc;
	list_append(&srvbmp->lbitmaps, &srvgc->bitmaps);
	srvbmp->bmp = bitmap;
	srvbmp->bmp_id = srvgc->next_bmp_id++;

	/* Area allocated by backing GC, we just shared it out */
	srvbmp->myalloc = false;
	srvbmp->pixels = alloc.pixels; // Not really needed

	async_answer_1(icall, EOK, srvbmp->bmp_id);
}

static void gc_bitmap_destroy_srv(ipc_gc_srv_t *srvgc, ipc_call_t *call)
{
	sysarg_t bmp_id;
	ipc_gc_srv_bitmap_t *bitmap;
	errno_t rc;

	bmp_id = ipc_get_arg1(call);

	bitmap = gc_bitmap_lookup(srvgc, bmp_id);
	if (bitmap == NULL) {
		async_answer_0(call, ENOENT);
		return;
	}

	rc = gfx_bitmap_destroy(bitmap->bmp);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return;
	}

	if (bitmap->myalloc)
		as_area_destroy(bitmap->pixels);

	list_remove(&bitmap->lbitmaps);
	free(bitmap);

	async_answer_0(call, rc);
}

static void gc_bitmap_render_srv(ipc_gc_srv_t *srvgc, ipc_call_t *icall)
{
	ipc_gc_srv_bitmap_t *bitmap;
	sysarg_t bmp_id;
	gfx_rect_t srect;
	gfx_coord2_t offs;
	ipc_call_t call;
	size_t size;
	errno_t rc;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(gfx_rect_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&call, &srect, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	bmp_id = ipc_get_arg1(icall);
	offs.x = ipc_get_arg2(icall);
	offs.y = ipc_get_arg3(icall);

	bitmap = gc_bitmap_lookup(srvgc, bmp_id);
	if (bitmap == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	rc = gfx_bitmap_render(bitmap->bmp, &srect, &offs);
	async_answer_0(icall, rc);
}

errno_t gc_conn(ipc_call_t *icall, gfx_context_t *gc)
{
	ipc_gc_srv_t srvgc;
	ipc_gc_srv_bitmap_t *bitmap;
	link_t *link;

	/* Accept the connection */
	async_accept_0(icall);

	srvgc.gc = gc;
	list_initialize(&srvgc.bitmaps);
	srvgc.next_bmp_id = 1;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}

		switch (method) {
		case GC_SET_CLIP_RECT:
			gc_set_clip_rect_srv(&srvgc, &call);
			break;
		case GC_SET_CLIP_RECT_NULL:
			gc_set_clip_rect_null_srv(&srvgc, &call);
			break;
		case GC_SET_RGB_COLOR:
			gc_set_rgb_color_srv(&srvgc, &call);
			break;
		case GC_FILL_RECT:
			gc_fill_rect_srv(&srvgc, &call);
			break;
		case GC_UPDATE:
			gc_update_srv(&srvgc, &call);
			break;
		case GC_BITMAP_CREATE:
			gc_bitmap_create_srv(&srvgc, &call);
			break;
		case GC_BITMAP_CREATE_DOUTPUT:
			gc_bitmap_create_doutput_srv(&srvgc, &call);
			break;
		case GC_BITMAP_DESTROY:
			gc_bitmap_destroy_srv(&srvgc, &call);
			break;
		case GC_BITMAP_RENDER:
			gc_bitmap_render_srv(&srvgc, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}

	/*
	 * Destroy all remaining bitmaps. A client should destroy all
	 * the bitmaps before closing connection. But it could happen
	 * that the client is misbehaving or was abruptly disconnected
	 * (e.g. crashed).
	 */
	link = list_first(&srvgc.bitmaps);
	while (link != NULL) {
		bitmap = list_get_instance(link, ipc_gc_srv_bitmap_t,
		    lbitmaps);

		(void) gfx_bitmap_destroy(bitmap->bmp);
		if (bitmap->myalloc)
			as_area_destroy(bitmap->pixels);
		list_remove(&bitmap->lbitmaps);
		free(bitmap);

		link = list_first(&srvgc.bitmaps);
	}

	return EOK;
}

static ipc_gc_srv_bitmap_t *gc_bitmap_lookup(ipc_gc_srv_t *srvgc,
    sysarg_t bmp_id)
{
	link_t *link;
	ipc_gc_srv_bitmap_t *bmp;

	link = list_first(&srvgc->bitmaps);
	while (link != NULL) {
		bmp = list_get_instance(link, ipc_gc_srv_bitmap_t, lbitmaps);
		if (bmp->bmp_id == bmp_id)
			return bmp;
		link = list_next(link, &srvgc->bitmaps);
	}

	return NULL;
}

/** @}
 */
