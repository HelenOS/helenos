/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup libguigfx
 * @{
 */
/**
 * @file GFX canvas backend
 *
 * This implements a graphics context over a libgui canvas.
 * This is just for experimentation purposes and its kind of backwards.
 */

#include <draw/drawctx.h>
#include <draw/source.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <guigfx/canvas.h>
#include <io/pixel.h>
#include <stdlib.h>
#include <transform.h>
#include "../private/canvas.h"
//#include "../../private/color.h"

static void canvas_gc_update_cb(void *, gfx_rect_t *);

/** Create canvas GC.
 *
 * Create graphics context for rendering into a canvas.
 *
 * @param canvas Canvas object
 * @param fout File to which characters are written (canvas)
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t canvas_gc_create(canvas_t *canvas, surface_t *surface,
    canvas_gc_t **rgc)
{
	canvas_gc_t *cgc = NULL;
	surface_coord_t w, h;
	gfx_rect_t rect;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	surface_get_resolution(surface, &w, &h);

	cgc = calloc(1, sizeof(canvas_gc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = w;
	rect.p1.y = h;

	alloc.pitch = w * sizeof(uint32_t);
	alloc.off0 = 0;
	alloc.pixels = surface_direct_access(surface);

	rc = mem_gc_create(&rect, &alloc, canvas_gc_update_cb,
	    (void *)cgc, &cgc->mgc);
	if (rc != EOK)
		goto error;

	cgc->gc = mem_gc_get_ctx(cgc->mgc);

	cgc->canvas = canvas;
	cgc->surface = surface;
	*rgc = cgc;
	return EOK;
error:
	if (cgc != NULL)
		free(cgc);
	return rc;
}

/** Delete canvas GC.
 *
 * @param cgc Canvas GC
 */
errno_t canvas_gc_delete(canvas_gc_t *cgc)
{
	errno_t rc;

	mem_gc_delete(cgc->mgc);

	rc = gfx_context_delete(cgc->gc);
	if (rc != EOK)
		return rc;

	free(cgc);
	return EOK;
}

/** Get generic graphic context from canvas GC.
 *
 * @param cgc Canvas GC
 * @return Graphic context
 */
gfx_context_t *canvas_gc_get_ctx(canvas_gc_t *cgc)
{
	return cgc->gc;
}

/** Canvas GC update callback called by memory GC.
 *
 * @param arg Canvas GC
 * @param rect Rectangle to update
 */
static void canvas_gc_update_cb(void *arg, gfx_rect_t *rect)
{
	canvas_gc_t *cgc = (canvas_gc_t *)arg;

	update_canvas(cgc->canvas, cgc->surface);
}

/** @}
 */
