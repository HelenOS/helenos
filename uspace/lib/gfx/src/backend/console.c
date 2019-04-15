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

/** @addtogroup libgfx
 * @{
 */
/**
 * @file GFX console backend
 *
 * This implements a graphics context over a classic console interface.
 * This is just for experimentation purposes. In the end we want the
 * console to actually directly suport GFX interface.
 */

#include <gfx/backend/console.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/pixel.h>
#include <stdlib.h>
#include "private/backend/console.h"
#include "private/color.h"

static errno_t console_gc_set_color(void *, gfx_color_t *);
static errno_t console_gc_fill_rect(void *, gfx_rect_t *);

gfx_context_ops_t console_gc_ops = {
	.set_color = console_gc_set_color,
	.fill_rect = console_gc_fill_rect
};

/** Set color on console GC.
 *
 * Set drawing color on console GC.
 *
 * @param arg Console GC
 * @param color Color
 *
 * @return EOK on success or an error code
 */
static errno_t console_gc_set_color(void *arg, gfx_color_t *color)
{
	console_gc_t *cgc = (console_gc_t *) arg;
	pixel_t clr;

	clr = PIXEL(0, color->r >> 8, color->g >> 8, color->b >> 8);

	console_set_rgb_color(cgc->con, clr, clr);
	return EOK;
}

/** Fill rectangle on console GC.
 *
 * @param arg Console GC
 * @param rect Rectangle
 *
 * @return EOK on success or an error code
 */
static errno_t console_gc_fill_rect(void *arg, gfx_rect_t *rect)
{
	console_gc_t *cgc = (console_gc_t *) arg;
	int rv;
	int x, y;

	// XXX We should handle p0.x > p1.x and p0.y > p1.y

	for (y = rect->p0.y; y < rect->p1.y; y++) {
		console_set_pos(cgc->con, rect->p0.x, y);

		for (x = rect->p0.x; x < rect->p1.x; x++) {
			rv = fputc('X', cgc->fout);
			if (rv < 0)
				return EIO;
		}

		console_flush(cgc->con);
	}

	return EOK;
}

/** Create console GC.
 *
 * Create graphics context for rendering into a console.
 *
 * @param con Console object
 * @param fout File to which characters are written (console)
 * @param rgc Place to store pointer to new GC.
 *
 * @return EOK on success or an error code
 */
errno_t console_gc_create(console_ctrl_t *con, FILE *fout,
    gfx_context_t **rgc)
{
	console_gc_t *cgc = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	cgc = calloc(1, sizeof(console_gc_t));
	if (cgc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = gfx_context_new(&console_gc_ops, cgc, &gc);
	if (rc != EOK)
		goto error;

	cgc->con = con;
	cgc->fout = fout;
	*rgc = gc;
	return EOK;
error:
	if (cgc != NULL)
		free(cgc);
	gfx_context_delete(gc);
	return rc;
}

/** @}
 */
