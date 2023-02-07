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
#include <gfx/text.h>
#include <ui/accel.h>
#include <ui/paint.h>
#include <stdlib.h>
#include <str.h>
#include "../private/resource.h"

/** Single box characters */
static ui_box_chars_t single_box_chars = {
	{
		{ "\u250c", "\u2500", "\u2510" },
		{ "\u2502", " ",      "\u2502" },
		{ "\u2514", "\u2500", "\u2518" }
	}
};

/** Double box characters */
static ui_box_chars_t double_box_chars = {
	{
		{ "\u2554", "\u2550", "\u2557" },
		{ "\u2551", " ",      "\u2551" },
		{ "\u255a", "\u2550", "\u255d" }
	}
};

/** Single horizontal brace characters */
static ui_brace_chars_t single_hbrace_chars = {
	.start = "\u251c",
	.middle = "\u2500",
	.end = "\u2524"
};

/** Double horizontal brace characters */
static ui_brace_chars_t double_hbrace_chars = {
	.start = "\u2560",
	.middle = "\u2550",
	.end = "\u2563"
};

/** Paint bevel.
 *
 * @param gc Graphic context
 * @param rect Rectangle to paint into
 * @param tlcolor Top-left color
 * @param brcolor Bottom-right color
 * @param thickness Bevel thickness in pixels
 * @param inside Place to store rectangle of the interior or @c NULL
 * @return EOK on success or an error code
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

	if (inside != NULL)
		ui_paint_get_bevel_inside(gc, rect, thickness, inside);

	return EOK;
error:
	return rc;
}

/** Get bevel interior rectangle.
 *
 * Get the bevel interior rectangle without painting it.
 *
 * @param gc Graphic context
 * @param rect Rectangle to paint into
 * @param thickness Bevel thickness in pixels
 * @param inside Place to store rectangle of the interior
 */
void ui_paint_get_bevel_inside(gfx_context_t *gc, gfx_rect_t *rect,
    gfx_coord_t thickness, gfx_rect_t *inside)
{
	inside->p0.x = rect->p0.x + thickness;
	inside->p0.y = rect->p0.y + thickness;
	inside->p1.x = rect->p1.x - thickness;
	inside->p1.y = rect->p1.y - thickness;
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

/** Get inset frame interior rectangle.
 *
 * This allows one to get the interior rectangle without actually painting
 * the inset frame.
 *
 * @param resource UI resource
 * @param rect Rectangle to paint onto
 * @param inside Place to store inside rectangle or @c NULL
 */
void ui_paint_get_inset_frame_inside(ui_resource_t *resource, gfx_rect_t *rect,
    gfx_rect_t *inside)
{
	ui_paint_get_bevel_inside(resource->gc, rect, 2, inside);
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

/** Paint upward pointing triangle.
 *
 * @param gc Graphic context
 * @param pos Center position
 * @param n Length of triangle side
 * @return EOK on success or an error code
 */
errno_t ui_paint_up_triangle(gfx_context_t *gc, gfx_coord2_t *pos,
    gfx_coord_t n)
{
	gfx_coord_t i;
	errno_t rc;
	gfx_rect_t rect;

	for (i = 0; i < n; i++) {
		rect.p0.x = pos->x - i;
		rect.p0.y = pos->y - n / 2 + i;
		rect.p1.x = pos->x + i + 1;
		rect.p1.y = pos->y - n / 2 + i + 1;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Paint downward pointing triangle.
 *
 * @param gc Graphic context
 * @param pos Center position
 * @param n Length of triangle side
 * @return EOK on success or an error code
 */
errno_t ui_paint_down_triangle(gfx_context_t *gc, gfx_coord2_t *pos,
    gfx_coord_t n)
{
	gfx_coord_t i;
	errno_t rc;
	gfx_rect_t rect;

	for (i = 0; i < n; i++) {
		rect.p0.x = pos->x - i;
		rect.p0.y = pos->y + n / 2 - i;
		rect.p1.x = pos->x + i + 1;
		rect.p1.y = pos->y + n / 2 - i + 1;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Paint left pointing triangle.
 *
 * @param gc Graphic context
 * @param pos Center position
 * @param n Length of triangle side
 * @return EOK on success or an error code
 */
errno_t ui_paint_left_triangle(gfx_context_t *gc, gfx_coord2_t *pos,
    gfx_coord_t n)
{
	gfx_coord_t i;
	errno_t rc;
	gfx_rect_t rect;

	for (i = 0; i < n; i++) {
		rect.p0.x = pos->x - n / 2 + i;
		rect.p0.y = pos->y - i;
		rect.p1.x = pos->x - n / 2 + i + 1;
		rect.p1.y = pos->y + i + 1;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Paint right pointing triangle.
 *
 * @param gc Graphic context
 * @param pos Center position
 * @param n Length of triangle side
 * @return EOK on success or an error code
 */
errno_t ui_paint_right_triangle(gfx_context_t *gc, gfx_coord2_t *pos,
    gfx_coord_t n)
{
	gfx_coord_t i;
	errno_t rc;
	gfx_rect_t rect;

	for (i = 0; i < n; i++) {
		rect.p0.x = pos->x + n / 2 - i;
		rect.p0.y = pos->y - i;
		rect.p1.x = pos->x + n / 2 - i + 1;
		rect.p1.y = pos->y + i + 1;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Paint diagonal cross (X).
 *
 * @param gc Graphic context
 * @param pos Center position
 * @param n Length of each leg
 * @param w Pen width
 * @param h Pen height
 * @return EOK on success or an error code
 */
errno_t ui_paint_cross(gfx_context_t *gc, gfx_coord2_t *pos,
    gfx_coord_t n, gfx_coord_t w, gfx_coord_t h)
{
	gfx_coord_t i;
	gfx_rect_t rect;
	errno_t rc;

	rect.p0.x = pos->x;
	rect.p0.y = pos->y;
	rect.p1.x = pos->x + w;
	rect.p1.y = pos->y + h;
	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		return rc;

	for (i = 1; i < n; i++) {
		rect.p0.x = pos->x - i;
		rect.p0.y = pos->y - i;
		rect.p1.x = pos->x - i + w;
		rect.p1.y = pos->y - i + h;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;

		rect.p0.x = pos->x - i;
		rect.p0.y = pos->y + i;
		rect.p1.x = pos->x - i + w;
		rect.p1.y = pos->y + i + h;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;

		rect.p0.x = pos->x + i;
		rect.p0.y = pos->y - i;
		rect.p1.x = pos->x + i + w;
		rect.p1.y = pos->y - i + h;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;

		rect.p0.x = pos->x + i;
		rect.p0.y = pos->y + i;
		rect.p1.x = pos->x + i + w;
		rect.p1.y = pos->y + i + h;
		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Paint minimize icon.
 *
 * @param resource UI resource
 * @param pos Center position
 * @param w Icon width
 * @param h Icon height
 * @return EOK on success or an error code
 */
errno_t ui_paint_minicon(ui_resource_t *resource, gfx_coord2_t *pos,
    gfx_coord_t w, gfx_coord_t h)
{
	gfx_rect_t rect;
	errno_t rc;

	rc = gfx_set_color(resource->gc, resource->btn_text_color);
	if (rc != EOK)
		return rc;

	rect.p0.x = pos->x - w / 2;
	rect.p0.y = pos->y + h / 2 - 2;
	rect.p1.x = rect.p0.x + w;
	rect.p1.y = rect.p0.y + 2;
	rc = gfx_fill_rect(resource->gc, &rect);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint maximize icon.
 *
 * @param resource UI resource
 * @param pos Center position
 * @param w Icon width
 * @param h Icon height
 * @return EOK on success or an error code
 */
errno_t ui_paint_maxicon(ui_resource_t *resource, gfx_coord2_t *pos,
    gfx_coord_t w, gfx_coord_t h)
{
	gfx_rect_t rect;
	errno_t rc;

	rc = gfx_set_color(resource->gc, resource->btn_text_color);
	if (rc != EOK)
		return rc;

	rect.p0.x = pos->x - w / 2;
	rect.p0.y = pos->y - h / 2;
	rect.p1.x = rect.p0.x + w;
	rect.p1.y = rect.p0.y + h;
	rc = gfx_fill_rect(resource->gc, &rect);
	if (rc != EOK)
		return rc;

	rc = gfx_set_color(resource->gc, resource->btn_face_color);
	if (rc != EOK)
		return rc;

	rect.p0.x += 1;
	rect.p0.y += 2;
	rect.p1.x -= 1;
	rect.p1.y -= 1;
	rc = gfx_fill_rect(resource->gc, &rect);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint unmaximize icon.
 *
 * Unmaximize icon consists of two overlapping window icons.
 *
 * @param resource UI resource
 * @param pos Center position
 * @param w Window icon width
 * @param h Window icon height
 * @param dw Horizontal distance between window icon centers
 * @param dh Vertical distance between window icon centers
 * @return EOK on success or an error code
 */
errno_t ui_paint_unmaxicon(ui_resource_t *resource, gfx_coord2_t *pos,
    gfx_coord_t w, gfx_coord_t h, gfx_coord_t dw, gfx_coord_t dh)
{
	gfx_coord2_t p;
	errno_t rc;

	p.x = pos->x + dw / 2;
	p.y = pos->y - dh / 2;
	rc = ui_paint_maxicon(resource, &p, w, h);
	if (rc != EOK)
		return rc;

	p.x = pos->x - dw / 2;
	p.y = pos->y + dh / 2;
	rc = ui_paint_maxicon(resource, &p, w, h);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint a custom text box.
 *
 * Paint a text box using user-provided box chars.
 *
 * @param resource UI resource
 * @param rect Rectangle inside which to paint the box
 * @param style Box style
 * @param boxc Box characters
 * @param color Color
 * @return EOK on success or an error code
 */
errno_t ui_paint_text_box_custom(ui_resource_t *resource, gfx_rect_t *rect,
    ui_box_chars_t *boxc, gfx_color_t *color)
{
	errno_t rc;
	gfx_text_fmt_t fmt;
	gfx_rect_t srect;
	gfx_coord2_t pos;
	gfx_coord2_t dim;
	size_t bufsz;
	size_t off;
	int i;
	gfx_coord_t y;
	char *str = NULL;

	gfx_rect_points_sort(rect, &srect);
	gfx_rect_dims(&srect, &dim);

	/* Is rectangle large enough to hold box? */
	if (dim.x < 2 || dim.y < 2)
		return EOK;

	gfx_text_fmt_init(&fmt);
	fmt.font = resource->font;
	fmt.color = color;

	bufsz = str_size(boxc->c[0][0]) +
	    str_size(boxc->c[0][1]) * (dim.x - 2) +
	    str_size(boxc->c[0][2]) + 1;

	str = malloc(bufsz);
	if (str == NULL)
		return ENOMEM;

	/* Top edge and corners */

	str_cpy(str, bufsz, boxc->c[0][0]);
	off = str_size(boxc->c[0][0]);

	for (i = 1; i < dim.x - 1; i++) {
		str_cpy(str + off, bufsz - off, boxc->c[0][1]);
		off += str_size(boxc->c[0][1]);
	}

	str_cpy(str + off, bufsz - off, boxc->c[0][2]);
	off += str_size(boxc->c[0][2]);
	str[off] = '\0';

	pos = rect->p0;
	rc = gfx_puttext(&pos, &fmt, str);
	if (rc != EOK)
		goto error;

	/* Vertical edges */
	for (y = rect->p0.y + 1; y < rect->p1.y - 1; y++) {
		pos.y = y;

		pos.x = rect->p0.x;
		rc = gfx_puttext(&pos, &fmt, boxc->c[1][0]);
		if (rc != EOK)
			goto error;

		pos.x = rect->p1.x - 1;
		rc = gfx_puttext(&pos, &fmt, boxc->c[1][2]);
		if (rc != EOK)
			goto error;
	}

	/* Bottom edge and corners */

	str_cpy(str, bufsz, boxc->c[2][0]);
	off = str_size(boxc->c[2][0]);

	for (i = 1; i < dim.x - 1; i++) {
		str_cpy(str + off, bufsz - off, boxc->c[2][1]);
		off += str_size(boxc->c[2][1]);
	}

	str_cpy(str + off, bufsz - off, boxc->c[2][2]);
	off += str_size(boxc->c[2][2]);
	str[off] = '\0';

	pos.x = rect->p0.x;
	pos.y = rect->p1.y - 1;
	rc = gfx_puttext(&pos, &fmt, str);
	if (rc != EOK)
		goto error;

	free(str);
	return EOK;
error:
	if (str != NULL)
		free(str);
	return rc;
}

/** Paint a text box.
 *
 * Paint a text box with the specified style.
 *
 * @param resource UI resource
 * @param rect Rectangle inside which to paint the box
 * @param style Box style
 * @param color Color
 * @return EOK on success or an error code
 */
errno_t ui_paint_text_box(ui_resource_t *resource, gfx_rect_t *rect,
    ui_box_style_t style, gfx_color_t *color)
{
	ui_box_chars_t *boxc = NULL;

	switch (style) {
	case ui_box_single:
		boxc = &single_box_chars;
		break;
	case ui_box_double:
		boxc = &double_box_chars;
		break;
	}

	if (boxc == NULL)
		return EINVAL;

	return ui_paint_text_box_custom(resource, rect, boxc, color);
}

/** Paint a text horizontal brace.
 *
 * @param resource UI resource
 * @param rect Rectangle inside which to paint the brace (height should
 *             be 1).
 * @param style Box style
 * @param color Color
 * @return EOK on success or an error code
 */
errno_t ui_paint_text_hbrace(ui_resource_t *resource, gfx_rect_t *rect,
    ui_box_style_t style, gfx_color_t *color)
{
	errno_t rc;
	gfx_text_fmt_t fmt;
	gfx_rect_t srect;
	gfx_coord2_t pos;
	gfx_coord2_t dim;
	size_t bufsz;
	size_t off;
	int i;
	char *str = NULL;
	ui_brace_chars_t *hbc = NULL;

	gfx_rect_points_sort(rect, &srect);
	gfx_rect_dims(&srect, &dim);

	/* Is rectangle large enough to hold brace? */
	if (dim.x < 2 || dim.y < 1)
		return EOK;

	switch (style) {
	case ui_box_single:
		hbc = &single_hbrace_chars;
		break;
	case ui_box_double:
		hbc = &double_hbrace_chars;
		break;
	}

	if (hbc == NULL)
		return EINVAL;

	gfx_text_fmt_init(&fmt);
	fmt.font = resource->font;
	fmt.color = color;

	bufsz = str_size(hbc->start) +
	    str_size(hbc->middle) * (dim.x - 2) +
	    str_size(hbc->end) + 1;

	str = malloc(bufsz);
	if (str == NULL)
		return ENOMEM;

	str_cpy(str, bufsz, hbc->start);
	off = str_size(hbc->start);

	for (i = 1; i < dim.x - 1; i++) {
		str_cpy(str + off, bufsz - off, hbc->middle);
		off += str_size(hbc->middle);
	}

	str_cpy(str + off, bufsz - off, hbc->end);
	off += str_size(hbc->end);
	str[off] = '\0';

	pos = rect->p0;
	rc = gfx_puttext(&pos, &fmt, str);
	if (rc != EOK)
		goto error;

	free(str);
	return EOK;
error:
	if (str != NULL)
		free(str);
	return rc;
}

/** Fill rectangle with text character.
 *
 * @param resource UI resource
 * @param rect Rectangle
 * @param color Color
 * @param gchar Character to fill with
 * @return EOK on success or an error code
 */
errno_t ui_paint_text_rect(ui_resource_t *resource, gfx_rect_t *rect,
    gfx_color_t *color, const char *gchar)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_rect_t srect;
	gfx_coord_t w, i;
	char *buf;
	size_t gcharsz;
	errno_t rc;

	gfx_rect_points_sort(rect, &srect);

	gfx_text_fmt_init(&fmt);
	fmt.font = resource->font;
	fmt.color = color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	w = srect.p1.x - srect.p0.x;
	if (w == 0)
		return EOK;

	gcharsz = str_size(gchar);

	buf = malloc(w * gcharsz + 1);
	if (buf == NULL)
		return ENOMEM;

	for (i = 0; i < w; i++)
		str_cpy(buf + i * gcharsz, (w - i) * gcharsz + 1, gchar);
	buf[w * gcharsz] = '\0';

	pos.x = srect.p0.x;
	for (pos.y = srect.p0.y; pos.y < srect.p1.y; pos.y++) {
		rc = gfx_puttext(&pos, &fmt, buf);
		if (rc != EOK)
			goto error;
	}

	free(buf);
	return EOK;
error:
	free(buf);
	return rc;
}

/** Initialize UI text formatting structure.
 *
 * UI text formatting structure must always be initialized using this function
 * first.
 *
 * @param fmt UI text formatting structure
 */
void ui_text_fmt_init(ui_text_fmt_t *fmt)
{
	memset(fmt, 0, sizeof(ui_text_fmt_t));
}

/** Compute UI text width.
 *
 * @param font Font
 * @param str String
 * @return Text width
 */
gfx_coord_t ui_text_width(gfx_font_t *font, const char *str)
{
	gfx_coord_t w;
	gfx_coord_t tw;
	const char *sp;

	/* Text including tildes */
	w = gfx_text_width(font, str);
	tw = gfx_text_width(font, "~");

	/* Subtract width of singular tildes */
	sp = str;
	while (*sp != '\0') {
		if (*sp == '~' && sp[1] != '~')
			w -= tw;
		++sp;
	}

	return w;
}

/** Paint text (with highlighting markup).
 *
 * Paint some text with highlighted sections enclosed with tilde characters
 * ('~'). A literal tilde can be printed with "~~". Text can be highlighted
 * with an underline or different color, based on display capabilities.
 *
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String containing '~' markup
 *
 * @return EOK on success or an error code
 */
errno_t ui_paint_text(gfx_coord2_t *pos, ui_text_fmt_t *fmt, const char *str)
{
	gfx_coord2_t tpos;
	gfx_text_fmt_t tfmt;
	char *buf;
	char *endptr;
	const char *sp;
	errno_t rc;

	/* Break down string into list of (non)highlighted parts */
	rc = ui_accel_process(str, &buf, &endptr);
	if (rc != EOK)
		return rc;

	gfx_text_fmt_init(&tfmt);
	tfmt.font = fmt->font;
	tfmt.color = fmt->color;
	tfmt.halign = fmt->halign;
	tfmt.width = fmt->width;
	tfmt.valign = fmt->valign;
	tfmt.underline = false;

	tpos = *pos;
	sp = buf;
	while (sp != endptr) {
		/* Print the current string */
		rc = gfx_puttext(&tpos, &tfmt, sp);
		if (rc != EOK)
			goto error;

		gfx_text_cont(&tpos, &tfmt, sp, &tpos, &tfmt);
		tfmt.underline = !tfmt.underline;
		tfmt.color = tfmt.underline ? fmt->hgl_color : fmt->color;

		/* Skip to the next string */
		while (*sp != '\0')
			++sp;
		++sp;
	}

	free(buf);
	return EOK;
error:
	free(buf);
	return rc;
}

/** @}
 */
