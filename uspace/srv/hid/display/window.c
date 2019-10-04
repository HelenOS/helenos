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
#include "display.h"
#include "window.h"

static errno_t ds_window_set_color(void *, gfx_color_t *);
static errno_t ds_window_fill_rect(void *, gfx_rect_t *);

gfx_context_ops_t ds_window_ops = {
	.set_color = ds_window_set_color,
	.fill_rect = ds_window_fill_rect
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
static errno_t ds_window_set_color(void *arg, gfx_color_t *color)
{
	ds_window_t *wnd = (ds_window_t *) arg;

	(void) wnd;
	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_set_color");
	return EOK;
}

/** Fill rectangle on window GC.
 *
 * @param arg Window GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t ds_window_fill_rect(void *arg, gfx_rect_t *rect)
{
	ds_window_t *wnd = (ds_window_t *) arg;

	(void) wnd;
	log_msg(LOG_DEFAULT, LVL_NOTE, "gc_fill_rect");
	return EOK;
}

/** Create window.
 *
 * Create graphics context for rendering into a window.
 *
 * @param disp Display to create window on
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t ds_window_create(ds_display_t *disp, ds_window_t **rgc)
{
	ds_window_t *wnd = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	wnd = calloc(1, sizeof(ds_window_t));
	if (wnd == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&ds_window_ops, wnd, &gc);
	if (rc != EOK)
		goto error;

	ds_display_add_window(disp, wnd);

	wnd->gc = gc;
	*rgc = wnd;
	return EOK;
error:
	if (wnd != NULL)
		free(wnd);
	gfx_context_delete(gc);
	return rc;
}

/** Delete window GC.
 *
 * @param wnd Window GC
 */
errno_t ds_window_delete(ds_window_t *wnd)
{
	errno_t rc;

	rc = gfx_context_delete(wnd->gc);
	if (rc != EOK)
		return rc;

	free(wnd);
	return EOK;
}

/** Get generic graphic context from window.
 *
 * @param wnd Window
 * @return Graphic context
 */
gfx_context_t *ds_window_get_ctx(ds_window_t *wnd)
{
	return wnd->gc;
}

/** @}
 */
