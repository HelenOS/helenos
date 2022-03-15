/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file Slider
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/slider.h>
#include "../private/resource.h"
#include "../private/slider.h"

/*
 * The kind reader will appreciate that slider button dimensions 23:15
 * are chosen such that, after subtracting the frame width (2 times 1),
 * we get 21:13, which is a good approximation of the golden ratio.
 */
enum {
	/** Slider button width */
	ui_slider_btn_w = 15,
	/** Slider button height */
	ui_slider_btn_h = 23,
	/** Slider button width in text mode */
	ui_slider_btn_w_text = 3,
	/** Slider button height in text mode */
	ui_slider_btn_h_text = 1,
	/** Slider button frame thickness */
	ui_slider_btn_frame_thickness = 1,
	/** Slider button bevel width */
	ui_slider_btn_bevel_width = 2,
	/** Slider groove width (total) */
	ui_slider_groove_width = 4,
	/** Amount by which slider groove bevel extendeds on each side
	 * beyond nominal groove length.
	 */
	ui_slider_groove_margin = 2
};

static void ui_slider_ctl_destroy(void *);
static errno_t ui_slider_ctl_paint(void *);
static ui_evclaim_t ui_slider_ctl_pos_event(void *, pos_event_t *);

/** Slider control ops */
ui_control_ops_t ui_slider_ops = {
	.destroy = ui_slider_ctl_destroy,
	.paint = ui_slider_ctl_paint,
	.pos_event = ui_slider_ctl_pos_event
};

/** Create new slider.
 *
 * @param resource UI resource
 * @param rslider Place to store pointer to new slider
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_slider_create(ui_resource_t *resource, ui_slider_t **rslider)
{
	ui_slider_t *slider;
	errno_t rc;

	slider = calloc(1, sizeof(ui_slider_t));
	if (slider == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_slider_ops, (void *) slider,
	    &slider->control);
	if (rc != EOK) {
		free(slider);
		return rc;
	}

	slider->res = resource;
	*rslider = slider;
	return EOK;
}

/** Destroy slider.
 *
 * @param slider Slider or @c NULL
 */
void ui_slider_destroy(ui_slider_t *slider)
{
	if (slider == NULL)
		return;

	ui_control_delete(slider->control);
	free(slider);
}

/** Get base control from slider.
 *
 * @param slider Slider
 * @return Control
 */
ui_control_t *ui_slider_ctl(ui_slider_t *slider)
{
	return slider->control;
}

/** Set slider callbacks.
 *
 * @param slider Slider
 * @param cb Slider callbacks
 * @param arg Callback argument
 */
void ui_slider_set_cb(ui_slider_t *slider, ui_slider_cb_t *cb, void *arg)
{
	slider->cb = cb;
	slider->arg = arg;
}

/** Set slider rectangle.
 *
 * @param slider Slider
 * @param rect New slider rectangle
 */
void ui_slider_set_rect(ui_slider_t *slider, gfx_rect_t *rect)
{
	slider->rect = *rect;
}

/** Paint outer slider frame.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
static errno_t ui_slider_paint_frame(ui_resource_t *res, gfx_rect_t *rect,
    gfx_coord_t thickness, gfx_rect_t *inside)
{
	gfx_rect_t drect;
	errno_t rc;

	rc = gfx_set_color(res->gc, res->btn_frame_color);
	if (rc != EOK)
		goto error;

	drect.p0.x = rect->p0.x + 1;
	drect.p0.y = rect->p0.y;
	drect.p1.x = rect->p1.x - 1;
	drect.p1.y = rect->p0.y + thickness;
	rc = gfx_fill_rect(res->gc, &drect);
	if (rc != EOK)
		goto error;

	drect.p0.x = rect->p0.x + 1;
	drect.p0.y = rect->p1.y - thickness;
	drect.p1.x = rect->p1.x - 1;
	drect.p1.y = rect->p1.y;
	rc = gfx_fill_rect(res->gc, &drect);
	if (rc != EOK)
		goto error;

	drect.p0.x = rect->p0.x;
	drect.p0.y = rect->p0.y + 1;
	drect.p1.x = rect->p0.x + thickness;
	drect.p1.y = rect->p1.y - 1;
	rc = gfx_fill_rect(res->gc, &drect);
	if (rc != EOK)
		goto error;

	drect.p0.x = rect->p1.x - thickness;
	drect.p0.y = rect->p0.y + 1;
	drect.p1.x = rect->p1.x;
	drect.p1.y = rect->p1.y - 1;
	rc = gfx_fill_rect(res->gc, &drect);
	if (rc != EOK)
		goto error;

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

/** Paint outset slider bevel.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
static errno_t ui_slider_paint_outset(ui_slider_t *slider,
    gfx_rect_t *rect, gfx_rect_t *inside)
{
	return ui_paint_bevel(slider->res->gc, rect,
	    slider->res->btn_highlight_color,
	    slider->res->btn_shadow_color, ui_slider_btn_bevel_width, inside);
}

/** Determine slider button rectagle.
 *
 * @param slider Slider
 */
static void ui_slider_btn_rect(ui_slider_t *slider, gfx_rect_t *rect)
{
	gfx_coord2_t pos;

	pos.x = slider->rect.p0.x + slider->pos;
	pos.y = slider->rect.p0.y;

	rect->p0.x = pos.x;
	rect->p0.y = pos.y;

	if (slider->res->textmode) {
		rect->p1.x = pos.x + ui_slider_btn_w_text;
		rect->p1.y = pos.y + ui_slider_btn_h_text;
	} else {
		rect->p1.x = pos.x + ui_slider_btn_w;
		rect->p1.y = pos.y + ui_slider_btn_h;
	}
}

/** Determine slider length.
 *
 * @param slider Slider
 * @return Slider length in pixels
 */
gfx_coord_t ui_slider_length(ui_slider_t *slider)
{
	gfx_coord2_t dims;
	gfx_coord_t w;

	gfx_rect_dims(&slider->rect, &dims);
	w = slider->res->textmode ? ui_slider_btn_w_text :
	    ui_slider_btn_w;
	return dims.x - w;
}

/** Paint slider in graphics mode.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
errno_t ui_slider_paint_gfx(ui_slider_t *slider)
{
	gfx_coord2_t pos;
	gfx_coord_t length;
	gfx_rect_t rect;
	gfx_rect_t brect;
	gfx_rect_t irect;
	errno_t rc;

	/* Paint slider groove */

	pos = slider->rect.p0;
	length = ui_slider_length(slider);

	rect.p0.x = pos.x + ui_slider_btn_w / 2 - ui_slider_groove_margin;
	rect.p0.y = pos.y + ui_slider_btn_h / 2 - ui_slider_groove_width / 2;
	rect.p1.x = pos.x + ui_slider_btn_w / 2 + length +
	    ui_slider_groove_margin;
	rect.p1.y = pos.y + ui_slider_btn_h / 2 + ui_slider_groove_width / 2;

	rc = ui_paint_inset_frame(slider->res, &rect, NULL);
	if (rc != EOK)
		goto error;

	/* Paint slider button */

	ui_slider_btn_rect(slider, &rect);

	rc = ui_slider_paint_frame(slider->res, &rect,
	    ui_slider_btn_frame_thickness, &brect);
	if (rc != EOK)
		goto error;

	rc = ui_slider_paint_outset(slider, &brect, &irect);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(slider->res->gc, slider->res->btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(slider->res->gc, &irect);
	if (rc != EOK)
		goto error;

	rc = gfx_update(slider->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint slider in text mode.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
errno_t ui_slider_paint_text(ui_slider_t *slider)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_coord_t w, i;
	char *buf;
	const char *gchar;
	size_t gcharsz;
	errno_t rc;

	/* Paint slider groove */

	pos = slider->rect.p0;

	gfx_text_fmt_init(&fmt);
	fmt.font = slider->res->font;
	fmt.color = slider->res->wnd_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	w = slider->rect.p1.x - slider->rect.p0.x;
	gchar = "\u2550";
	gcharsz = str_size(gchar);

	buf = malloc(w * gcharsz + 1);
	if (buf == NULL)
		return ENOMEM;

	for (i = 0; i < w; i++)
		str_cpy(buf + i * gcharsz, (w - i) * gcharsz + 1, gchar);
	buf[w * gcharsz] = '\0';

	rc = gfx_puttext(&pos, &fmt, buf);
	free(buf);
	if (rc != EOK)
		goto error;

	/* Paint slider button */

	pos.x += slider->pos;

	rc = gfx_puttext(&pos, &fmt, "[O]");
	if (rc != EOK)
		goto error;

	rc = gfx_update(slider->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint slider.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
errno_t ui_slider_paint(ui_slider_t *slider)
{
	if (slider->res->textmode)
		return ui_slider_paint_text(slider);
	else
		return ui_slider_paint_gfx(slider);
}

/** Clear slider button.
 *
 * @param slider Slider
 * @return EOK on success or an error code
 */
errno_t ui_slider_btn_clear(ui_slider_t *slider)
{
	gfx_rect_t rect;
	errno_t rc;

	/* No need to clear button in text mode */
	if (slider->res->textmode)
		return EOK;

	ui_slider_btn_rect(slider, &rect);

	rc = gfx_set_color(slider->res->gc, slider->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(slider->res->gc, &rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Press down slider.
 *
 * @param slider Slider
 * @param pos Pointer position
 */
void ui_slider_press(ui_slider_t *slider, gfx_coord2_t *pos)
{
	if (slider->held)
		return;

	slider->held = true;
	slider->press_pos = *pos;
	slider->last_pos = slider->pos;

	(void) ui_slider_paint(slider);
}

/** Release slider.
 *
 * @param slider Slider
 * @param pos Pointer position
 */
void ui_slider_release(ui_slider_t *slider, gfx_coord2_t *pos)
{
	if (!slider->held)
		return;

	ui_slider_update(slider, pos);
	slider->held = false;
}

/** Pointer moved.
 *
 * @param slider Slider
 * @param pos New pointer position
 */
void ui_slider_update(ui_slider_t *slider, gfx_coord2_t *pos)
{
	gfx_coord_t spos;
	gfx_coord_t length;

	if (slider->held) {
		spos = slider->last_pos + pos->x - slider->press_pos.x;
		length = ui_slider_length(slider);
		if (spos < 0)
			spos = 0;
		if (spos > length)
			spos = length;

		if (spos != slider->pos) {
			(void) ui_slider_btn_clear(slider);
			slider->pos = spos;
			(void) ui_slider_paint(slider);
			ui_slider_moved(slider, spos);
		}
	}
}

/** Slider was moved.
 *
 * @param slider Slider
 */
void ui_slider_moved(ui_slider_t *slider, gfx_coord_t pos)
{
	if (slider->cb != NULL && slider->cb->moved != NULL)
		slider->cb->moved(slider, slider->arg, pos);
}

/** Handle slider position event.
 *
 * @param slider Slider
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_slider_pos_event(ui_slider_t *slider, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_rect_t rect;

	pos.x = event->hpos;
	pos.y = event->vpos;

	switch (event->type) {
	case POS_PRESS:
		ui_slider_btn_rect(slider, &rect);
		if (gfx_pix_inside_rect(&pos, &rect)) {
			ui_slider_press(slider, &pos);
			slider->press_pos = pos;
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (slider->held) {
			ui_slider_release(slider, &pos);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		ui_slider_update(slider, &pos);
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Destroy slider control.
 *
 * @param arg Argument (ui_slider_t *)
 */
void ui_slider_ctl_destroy(void *arg)
{
	ui_slider_t *slider = (ui_slider_t *) arg;

	ui_slider_destroy(slider);
}

/** Paint slider control.
 *
 * @param arg Argument (ui_slider_t *)
 * @return EOK on success or an error code
 */
errno_t ui_slider_ctl_paint(void *arg)
{
	ui_slider_t *slider = (ui_slider_t *) arg;

	return ui_slider_paint(slider);
}

/** Handle slider control position event.
 *
 * @param arg Argument (ui_slider_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_slider_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_slider_t *slider = (ui_slider_t *) arg;

	return ui_slider_pos_event(slider, event);
}

/** @}
 */
