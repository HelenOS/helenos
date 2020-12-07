/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Painting routines
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <ui/paint.h>

/** Paint bevel.
 *
 * @param gc Graphic context
 * @param rect Rectangle to paint into
 * @param tlcolor Top-left color
 * @param brcolor Bottom-right color
 * @param thickness Bevel thickness in pixels
 * @param inside Place to store rectangle of the interior or @c NULL
 * @reutrn EOK on success or an error code
 */
errno_t ui_paint_bevel(gfx_context_t *gc, gfx_rect_t *rect,
    gfx_color_t *tlcolor, gfx_color_t *brcolor, gfx_coord_t thickness,
    gfx_rect_t *inside)
{
	gfx_rect_t frect;
	gfx_coord_t i;
	errno_t rc;

	/* Top left */

	rc = gfx_set_color(gc, tlcolor);
	if (rc != EOK)
		goto error;

	for (i = 0; i < thickness; i++) {
		frect.p0.x = rect->p0.x + i;
		frect.p0.y = rect->p0.y + i;
		frect.p1.x = rect->p1.x - i - 1;
		frect.p1.y = rect->p0.y + i + 1;
		rc = gfx_fill_rect(gc, &frect);
		if (rc != EOK)
			goto error;

		frect.p0.x = rect->p0.x + i;
		frect.p0.y = rect->p0.y + i + 1;
		frect.p1.x = rect->p0.x + i + 1;
		frect.p1.y = rect->p1.y - i - 1;
		rc = gfx_fill_rect(gc, &frect);
		if (rc != EOK)
			goto error;
	}

	/* Bottom right */

	rc = gfx_set_color(gc, brcolor);
	if (rc != EOK)
		goto error;

	for (i = 0; i < thickness; i++) {
		frect.p0.x = rect->p0.x + i;
		frect.p0.y = rect->p1.y - i - 1;
		frect.p1.x = rect->p1.x - i - 1;
		frect.p1.y = rect->p1.y - i;
		rc = gfx_fill_rect(gc, &frect);
		if (rc != EOK)
			goto error;

		frect.p0.x = rect->p1.x - i - 1;
		frect.p0.y = rect->p0.y + i;
		frect.p1.x = rect->p1.x - i;
		frect.p1.y = rect->p1.y - i;
		rc = gfx_fill_rect(gc, &frect);
		if (rc != EOK)
			goto error;
	}

	if (inside != NULL) {
		inside->p0.x = rect->p0.x + thickness;
		inside->p0.y = rect->p0.y + thickness;
		inside->p1.x = rect->p1.x - thickness;
		inside->p1.y = rect->p1.y - thickness;
	}

	return EOK;
error:
	return rc;
}

/** @}
 */
