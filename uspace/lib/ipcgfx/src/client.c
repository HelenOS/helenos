/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file GFX IPC backend
 *
 * This implements a graphics context via HelenOS IPC.
 */

#include <as.h>
#include <ipcgfx/client.h>
#include <ipcgfx/ipc/gc.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <stdlib.h>
#include "../private/client.h"

static errno_t ipc_gc_set_clip_rect(void *, gfx_rect_t *);
static errno_t ipc_gc_set_color(void *, gfx_color_t *);
static errno_t ipc_gc_fill_rect(void *, gfx_rect_t *);
static errno_t ipc_gc_update(void *);
static errno_t ipc_gc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t ipc_gc_bitmap_destroy(void *);
static errno_t ipc_gc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t ipc_gc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

gfx_context_ops_t ipc_gc_ops = {
	.set_clip_rect = ipc_gc_set_clip_rect,
	.set_color = ipc_gc_set_color,
	.fill_rect = ipc_gc_fill_rect,
	.update = ipc_gc_update,
	.bitmap_create = ipc_gc_bitmap_create,
	.bitmap_destroy = ipc_gc_bitmap_destroy,
	.bitmap_render = ipc_gc_bitmap_render,
	.bitmap_get_alloc = ipc_gc_bitmap_get_alloc
};

/** Set clipping rectangle on IPC GC.
 *
 * @param arg IPC GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(ipcgc->sess);
	if (rect != NULL) {
		rc = async_req_4_0(exch, GC_SET_CLIP_RECT, rect->p0.x, rect->p0.y,
		    rect->p1.x, rect->p1.y);
	} else {
		rc = async_req_0_0(exch, GC_SET_CLIP_RECT_NULL);
	}

	async_exchange_end(exch);

	return rc;
}

/** Set color on IPC GC.
 *
 * Set drawing color on IPC GC.
 *
 * @param arg IPC GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_set_color(void *arg, gfx_color_t *color)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	uint16_t r, g, b;
	errno_t rc;

	gfx_color_get_rgb_i16(color, &r, &g, &b);

	exch = async_exchange_begin(ipcgc->sess);
	rc = async_req_3_0(exch, GC_SET_RGB_COLOR, r, g, b);
	async_exchange_end(exch);

	return rc;
}

/** Fill rectangle on IPC GC.
 *
 * @param arg IPC GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(ipcgc->sess);
	rc = async_req_4_0(exch, GC_FILL_RECT, rect->p0.x, rect->p0.y,
	    rect->p1.x, rect->p1.y);
	async_exchange_end(exch);

	return rc;
}

/** Update display on IPC GC.
 *
 * @param arg IPC GC
 *
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_update(void *arg)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(ipcgc->sess);
	rc = async_req_0_0(exch, GC_UPDATE);
	async_exchange_end(exch);

	return rc;
}

/** Create normal bitmap in IPC GC.
 *
 * @param arg IPC GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_bitmap_create_normal(void *arg,
    gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	ipc_gc_bitmap_t *ipcbm = NULL;
	gfx_coord2_t dim;
	async_exch_t *exch = NULL;
	as_area_info_t info;
	ipc_call_t answer;
	size_t asize;
	aid_t req;
	errno_t rc;

	ipcbm = calloc(1, sizeof(ipc_gc_bitmap_t));
	if (ipcbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	ipcbm->rect = params->rect;

	if (alloc == NULL) {
		ipcbm->alloc.pitch = dim.x * sizeof(uint32_t);
		ipcbm->alloc.off0 = 0;
		ipcbm->alloc.pixels = as_area_create(AS_AREA_ANY,
		    dim.x * dim.y * sizeof(uint32_t), AS_AREA_READ |
		    AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
		if (ipcbm->alloc.pixels == AS_MAP_FAILED) {
			rc = ENOMEM;
			goto error;
		}

		ipcbm->myalloc = true;
	} else {
		/*
		 * Accept user allocation if it points to a an acceptable
		 * memory area.
		 */
		rc = as_area_get_info(alloc->pixels, &info);
		if (rc != EOK)
			goto error;

		/* Pixels should start at the beginning of the area */
		if (info.start_addr != (uintptr_t) alloc->pixels) {
			rc = EINVAL;
			goto error;
		}

		/* Size of area should be size of bitmap rounded up to page size */
		asize = PAGES2SIZE(SIZE2PAGES(alloc->pitch * dim.y));
		if (info.size != asize) {
			rc = EINVAL;
			goto error;
		}

		ipcbm->alloc = *alloc;
	}

	exch = async_exchange_begin(ipcgc->sess);
	req = async_send_0(exch, GC_BITMAP_CREATE, &answer);
	rc = async_data_write_start(exch, params, sizeof (gfx_bitmap_params_t));
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}

	rc = async_share_out_start(exch, ipcbm->alloc.pixels,
	    AS_AREA_READ | AS_AREA_CACHEABLE);
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}
	async_exchange_end(exch);
	exch = NULL;

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	ipcbm->ipcgc = ipcgc;
	ipcbm->bmp_id = ipc_get_arg1(&answer);
	*rbm = (void *)ipcbm;
	return EOK;
error:
	if (exch != NULL)
		async_exchange_end(exch);
	if (ipcbm != NULL) {
		if (ipcbm->alloc.pixels != NULL)
			as_area_destroy(ipcbm->alloc.pixels);
		free(ipcbm);
	}
	return rc;
}

/** Create direct output bitmap in IPC GC.
 *
 * @param arg IPC GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_bitmap_create_direct_output(void *arg,
    gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	ipc_gc_t *ipcgc = (ipc_gc_t *) arg;
	ipc_gc_bitmap_t *ipcbm = NULL;
	gfx_coord2_t dim;
	async_exch_t *exch = NULL;
	void *pixels;
	ipc_call_t answer;
	size_t asize;
	aid_t req;
	errno_t rc;

	/* Cannot specify allocation for direct output bitmap */
	if (alloc != NULL)
		return EINVAL;

	ipcbm = calloc(1, sizeof(ipc_gc_bitmap_t));
	if (ipcbm == NULL)
		return ENOMEM;

	gfx_coord2_subtract(&params->rect.p1, &params->rect.p0, &dim);
	ipcbm->rect = params->rect;

	ipcbm->alloc.pitch = dim.x * sizeof(uint32_t);
	ipcbm->alloc.off0 = 0;
	ipcbm->myalloc = true;

	asize = PAGES2SIZE(SIZE2PAGES(ipcbm->alloc.pitch * dim.y));

	exch = async_exchange_begin(ipcgc->sess);
	req = async_send_0(exch, GC_BITMAP_CREATE_DOUTPUT, &answer);
	rc = async_data_write_start(exch, params, sizeof (gfx_bitmap_params_t));
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}

	rc = async_share_in_start_0_0(exch, asize, &pixels);
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}
	async_exchange_end(exch);
	exch = NULL;

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	ipcbm->ipcgc = ipcgc;
	ipcbm->bmp_id = ipc_get_arg1(&answer);
	ipcbm->alloc.pixels = pixels;
	*rbm = (void *)ipcbm;
	return EOK;
error:
	if (exch != NULL)
		async_exchange_end(exch);
	if (ipcbm != NULL) {
		if (ipcbm->alloc.pixels != NULL)
			as_area_destroy(ipcbm->alloc.pixels);
		free(ipcbm);
	}
	return rc;
}

/** Create bitmap in IPC GC.
 *
 * @param arg IPC GC
 * @param params Bitmap params
 * @param alloc Bitmap allocation info or @c NULL
 * @param rbm Place to store pointer to new bitmap
 * @return EOK on success or an error code
 */
errno_t ipc_gc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	if ((params->flags & bmpf_direct_output) != 0) {
		return ipc_gc_bitmap_create_direct_output(arg, params, alloc,
		    rbm);
	} else {
		return ipc_gc_bitmap_create_normal(arg, params, alloc, rbm);
	}
}

/** Destroy bitmap in IPC GC.
 *
 * @param bm Bitmap
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_bitmap_destroy(void *bm)
{
	ipc_gc_bitmap_t *ipcbm = (ipc_gc_bitmap_t *)bm;
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(ipcbm->ipcgc->sess);
	rc = async_req_1_0(exch, GC_BITMAP_DESTROY, ipcbm->bmp_id);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	if (ipcbm->myalloc)
		as_area_destroy(ipcbm->alloc.pixels);
	free(ipcbm);
	return EOK;
}

/** Render bitmap in IPC GC.
 *
 * @param bm Bitmap
 * @param srect0 Source rectangle or @c NULL
 * @param offs0 Offset or @c NULL
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_bitmap_render(void *bm, gfx_rect_t *srect0,
    gfx_coord2_t *offs0)
{
	ipc_gc_bitmap_t *ipcbm = (ipc_gc_bitmap_t *)bm;
	gfx_rect_t srect;
	gfx_rect_t drect;
	gfx_coord2_t offs;
	async_exch_t *exch = NULL;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	if (srect0 != NULL)
		srect = *srect0;
	else
		srect = ipcbm->rect;

	if (offs0 != NULL) {
		offs = *offs0;
	} else {
		offs.x = 0;
		offs.y = 0;
	}

	/* Destination rectangle */
	gfx_rect_translate(&offs, &srect, &drect);

	exch = async_exchange_begin(ipcbm->ipcgc->sess);
	req = async_send_3(exch, GC_BITMAP_RENDER, ipcbm->bmp_id, offs.x,
	    offs.y, &answer);

	rc = async_data_write_start(exch, &srect, sizeof (gfx_rect_t));
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}

	async_exchange_end(exch);
	exch = NULL;

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (exch != NULL)
		async_exchange_end(exch);
	return rc;
}

/** Get allocation info for bitmap in IPC GC.
 *
 * @param bm Bitmap
 * @param alloc Place to store allocation info
 * @return EOK on success or an error code
 */
static errno_t ipc_gc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	ipc_gc_bitmap_t *ipcbm = (ipc_gc_bitmap_t *)bm;
	*alloc = ipcbm->alloc;
	return EOK;
}

/** Create IPC GC.
 *
 * Create graphics context for rendering via IPC.
 *
 * @param sess Async session
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t ipc_gc_create(async_sess_t *sess, ipc_gc_t **rgc)
{
	ipc_gc_t *ipcgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	ipcgc = calloc(1, sizeof(ipc_gc_t));
	if (ipcgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&ipc_gc_ops, ipcgc, &gc);
	if (rc != EOK)
		goto error;

	ipcgc->gc = gc;
	ipcgc->sess = sess;
	*rgc = ipcgc;
	return EOK;
error:
	if (ipcgc != NULL)
		free(ipcgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete IPC GC.
 *
 * @param ipcgc IPC GC
 */
errno_t ipc_gc_delete(ipc_gc_t *ipcgc)
{
	errno_t rc;

	rc = gfx_context_delete(ipcgc->gc);
	if (rc != EOK)
		return rc;

	free(ipcgc);
	return EOK;
}

/** Get generic graphic context from IPC GC.
 *
 * @param ipcgc IPC GC
 * @return Graphic context
 */
gfx_context_t *ipc_gc_get_ctx(ipc_gc_t *ipcgc)
{
	return ipcgc->gc;
}

/** @}
 */
