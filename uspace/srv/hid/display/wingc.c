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

/** @addtogroup display
 * @{
 */
/**
 * @file GFX window backend
 *
 * This implements a graphics context over display server window.
 */

#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/log.h>
#include <stdlib.h>
#include "wingc.h"

static errno_t win_gc_set_color(void *, gfx_color_t *);
static errno_t win_gc_fill_rect(void *, gfx_rect_t *);

gfx_context_ops_t win_gc_ops = {
	.set_color = win_gc_set_color,
	.fill_rect = win_gc_fill_rect
};

/** Set color on window GC.
 *
 * Set drawing color on window GC.
 *
 * @param arg Console GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t win_gc_set_color(void *arg, gfx_color_t *color)
{
	win_gc_t *wgc = (win_gc_t *) arg;

	(void) wgc;
	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_set_color");
	return EOK;
}

/** Fill rectangle on window GC.
 *
 * @param arg Console GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t win_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	win_gc_t *wgc = (win_gc_t *) arg;

	(void) wgc;
	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_fill_rect");
	return EOK;
}

/** Create window GC.
 *
 * Create graphics context for rendering into a window.
 *
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t win_gc_create(win_gc_t **rgc)
{
	win_gc_t *wgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	wgc = calloc(1, sizeof(win_gc_t));
	if (wgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&win_gc_ops, wgc, &gc);
	if (rc != EOK)
		goto error;

	wgc->gc = gc;
	*rgc = wgc;
	return EOK;
error:
	if (wgc != NULL)
		free(wgc);
	gfx_context_delete(gc);
	return rc;
}

/** Delete window GC.
 *
 * @param wgc Console GC
 */
errno_t win_gc_delete(win_gc_t *wgc)
{
	errno_t rc;

	rc = gfx_context_delete(wgc->gc);
	if (rc != EOK)
		return rc;

	free(wgc);
	return EOK;
}

/** Get generic graphic context from window GC.
 *
 * @param wgc Console GC
 * @return Graphic context
 */
gfx_context_t *win_gc_get_ctx(win_gc_t *wgc)
{
	return wgc->gc;
}

/** @}
 */
