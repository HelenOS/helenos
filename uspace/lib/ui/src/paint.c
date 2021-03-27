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
#include "../private/resource.h"

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

/** Paint inset frame.
 *
 * @param resource UI resource
 * @param rect Rectangle to paint onto
 * @param inside Place to store inside rectangle or @c NULL
 * @return EOK on success or an error code
 */
errno_t ui_paint_inset_frame(ui_resource_t *resource, gfx_rect_t *rect,
    gfx_rect_t *inside)
{
	gfx_rect_t frame;
	errno_t rc;

	rc = ui_paint_bevel(resource->gc, rect,
	    resource->wnd_shadow_color, resource->wnd_highlight_color,
	    1, &frame);
	if (rc != EOK)
		goto error;

	rc = ui_paint_bevel(resource->gc, &frame,
	    resource->wnd_frame_sh_color, resource->wnd_frame_hi_color,
	    1, inside);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint outset frame.
 *
 * @param resource UI resource
 * @param rect Rectangle to paint onto
 * @param inside Place to store inside rectangle or @c NULL
 * @return EOK on success or an error code
 */
errno_t ui_paint_outset_frame(ui_resource_t *resource, gfx_rect_t *rect,
    gfx_rect_t *inside)
{
	gfx_rect_t frame;
	errno_t rc;

	rc = ui_paint_bevel(resource->gc, rect,
	    resource->wnd_frame_hi_color, resource->wnd_frame_sh_color,
	    1, &frame);
	if (rc != EOK)
		goto error;

	rc = ui_paint_bevel(resource->gc, &frame,
	    resource->wnd_highlight_color, resource->wnd_shadow_color,
	    1, inside);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint filled circle vertical scanline.
 *
 * @param gc Graphic context
 * @param center Coordinates of the center of the circle
 * @param x X-coordinate of the scanline
 * @param y0 Lowest Y coordinate of the scanline (inclusive)
 * @param y1 Highest Y coordinate of the scanline (inclusive)
 * @param part Which part(s) of cicle to paint
 * @return EOK on success or an error code
 */
static errno_t ui_paint_fcircle_line(gfx_context_t *gc, gfx_coord2_t *center,
    gfx_coord_t x, gfx_coord_t y0, gfx_coord_t y1, ui_fcircle_part_t part)
{
	gfx_rect_t rect;
	gfx_rect_t trect;

	rect.p0.x = x;
	rect.p0.y = y0;
	rect.p1.x = x + 1;
	rect.p1.y = y1;

	/* Clip to upper-left/lower-right half of circle, if required */

	if ((part & ui_fcircle_upleft) == 0) {
		if (rect.p0.y < -rect.p1.x)
			rect.p0.y = -rect.p1.x;
	}

	if ((part & ui_fcircle_lowright) == 0) {
		if (rect.p1.y > -rect.p1.x)
			rect.p1.y = -rect.p1.x;
	}

	/* If coordinates are reversed, there is nothing to do */
	if (rect.p1.y <= rect.p0.y)
		return EOK;

	gfx_rect_translate(center, &rect, &trect);

	return gfx_fill_rect(gc, &trect);
}

/** Paint filled circle scanlines corresponding to octant point.
 *
 * Fills the four vertical scanlines lying between the eight reflections
 * of a circle point.
 *
 * @param gc Graphic context
 * @param center Coordinates of the center
 * @param r Radius in pixels
 * @param part Which part(s) of cicle to paint
 * @return EOK on success or an error code
 */
static int ui_paint_fcircle_point(gfx_context_t *gc, gfx_coord2_t *center,
    gfx_coord2_t *p, ui_fcircle_part_t part)
{
	errno_t rc;

	rc = ui_paint_fcircle_line(gc, center, +p->x, -p->y, p->y + 1, part);
	if (rc != EOK)
		return rc;
	rc = ui_paint_fcircle_line(gc, center, -p->x, -p->y, p->y + 1, part);
	if (rc != EOK)
		return rc;
	rc = ui_paint_fcircle_line(gc, center, +p->y, -p->x, p->x + 1, part);
	if (rc != EOK)
		return rc;
	rc = ui_paint_fcircle_line(gc, center, -p->y, -p->x, p->x + 1, part);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint a filled circle.
 *
 * @param gc Graphic context
 * @param center Coordinates of the center
 * @param r Radius in pixels
 * @param part Which part(s) of cicle to paint
 * @return EOK on success or an error code
 */
errno_t ui_paint_filled_circle(gfx_context_t *gc, gfx_coord2_t *center,
    gfx_coord_t r, ui_fcircle_part_t part)
{
	gfx_coord2_t p;
	gfx_coord_t f;
	errno_t rc;

	/* Run through one octant using circle midpoint algorithm */

	p.x = r;
	p.y = 0;
	f = 1 - r;

	rc = ui_paint_fcircle_point(gc, center, &p, part);
	if (rc != EOK)
		return rc;

	while (p.x > p.y) {
		if (f < 0) {
			f = f + 2 * p.y + 3;
		} else {
			f = f + 2 * (p.y - p.x) + 5;
			--p.x;
		}
		++p.y;

		rc = ui_paint_fcircle_point(gc, center, &p, part);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** @}
 */
