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
 * @file Scrollbar
 *
 * Anatomy of a horizontal scrollbar:
 *
 *        Upper             Lower
 *       trough            trough
 *  +---+------+--------+---------+---+
 *  | < |      |   |||  |         | > |
 *  +---+------+--------+---------+---+
 *   Up           Thumb           Down
 * button                        button
 *
 *     +-------- Trough --------+
 *
 * Scrollbar uses the same terminology whether it is running in horizontal
 * or vertical mode, in horizontal mode up means left, down means right
 * (i.e. lower and higher coordinates, respectively).
 *
 * The thumb can be dragged to a specific position, resulting in a move
 * event. The up/down buttons generate up/down events. Pressing a mouse
 * button on the upper/lower trough generates page up / page down events.
 *
 * Pressing and holding down mouse button on up / down button or upper /
 * lower trough will auto-scroll (using clickmatic).
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/clickmatic.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/pbutton.h>
#include <ui/scrollbar.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/pbutton.h"
#include "../private/resource.h"
#include "../private/scrollbar.h"

enum {
	/** Scrollbar button width */
	ui_scrollbar_btn_len = 21,
	/** Scrollbar button width in text mode */
	ui_scrollbar_btn_len_text = 1,
	/** Scrollbar thumb frame thickness */
	ui_scrollbar_thumb_frame_thickness = 1,
	/** Scrollbar thumb bevel width */
	ui_scrollbar_thumb_bevel_width = 2,
	/** Scrollbar default thumb length */
	ui_scrollbar_def_thumb_len = 21,
	/** Scrollbar default thumb length in text mode */
	ui_scrollbar_def_thumb_len_text = 1,
	/** Scrollbar minimum thumb length */
	ui_scrollbar_min_thumb_len = 10,
	/** Scrollbar minimum thumb length in text mode */
	ui_scrollbar_min_thumb_len_text = 1,
};

static void ui_scrollbar_up_btn_down(ui_pbutton_t *, void *);
static void ui_scrollbar_up_btn_up(ui_pbutton_t *, void *);
static errno_t ui_scrollbar_up_btn_decor_paint(ui_pbutton_t *, void *,
    gfx_coord2_t *);
static errno_t ui_scrollbar_down_btn_decor_paint(ui_pbutton_t *, void *,
    gfx_coord2_t *);
static void ui_scrollbar_down_btn_down(ui_pbutton_t *, void *);
static void ui_scrollbar_down_btn_up(ui_pbutton_t *, void *);
static void ui_scrollbar_ctl_destroy(void *);
static errno_t ui_scrollbar_ctl_paint(void *);
static ui_evclaim_t ui_scrollbar_ctl_pos_event(void *, pos_event_t *);

static ui_pbutton_cb_t ui_scrollbar_up_btn_cb = {
	.down = ui_scrollbar_up_btn_down,
	.up = ui_scrollbar_up_btn_up
};

static ui_pbutton_decor_ops_t ui_scrollbar_up_btn_decor_ops = {
	.paint = ui_scrollbar_up_btn_decor_paint
};

static ui_pbutton_cb_t ui_scrollbar_down_btn_cb = {
	.down = ui_scrollbar_down_btn_down,
	.up = ui_scrollbar_down_btn_up
};

static ui_pbutton_decor_ops_t ui_scrollbar_down_btn_decor_ops = {
	.paint = ui_scrollbar_down_btn_decor_paint
};

/** Scrollbar control ops */
static ui_control_ops_t ui_scrollbar_ops = {
	.destroy = ui_scrollbar_ctl_destroy,
	.paint = ui_scrollbar_ctl_paint,
	.pos_event = ui_scrollbar_ctl_pos_event
};

static void ui_scrollbar_cm_up(ui_clickmatic_t *, void *);
static void ui_scrollbar_cm_down(ui_clickmatic_t *, void *);
static void ui_scrollbar_cm_page_up(ui_clickmatic_t *, void *);
static void ui_scrollbar_cm_page_down(ui_clickmatic_t *, void *);

/** Scrollbar clickmatic up callbacks */
ui_clickmatic_cb_t ui_scrollbar_clickmatic_up_cb = {
	.clicked = ui_scrollbar_cm_up
};

/** Scrollbar clickmatic down callbacks */
ui_clickmatic_cb_t ui_scrollbar_clickmatic_down_cb = {
	.clicked = ui_scrollbar_cm_down
};

/** Scrollbar clickmatic page up callbacks */
ui_clickmatic_cb_t ui_scrollbar_clickmatic_page_up_cb = {
	.clicked = ui_scrollbar_cm_page_up
};

/** Scrollbar clickmatic page down callbacks */
ui_clickmatic_cb_t ui_scrollbar_clickmatic_page_down_cb = {
	.clicked = ui_scrollbar_cm_page_down
};

/** Create new scrollbar.
 *
 * @param ui UI
 * @param window Window containing scrollbar
 * @param dir Scrollbar direction
 * @param rscrollbar Place to store pointer to new scrollbar
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_scrollbar_create(ui_t *ui, ui_window_t *window,
    ui_scrollbar_dir_t dir, ui_scrollbar_t **rscrollbar)
{
	ui_scrollbar_t *scrollbar;
	ui_resource_t *resource;
	const char *up_text;
	const char *down_text;
	errno_t rc;

	resource = ui_window_get_res(window);

	scrollbar = calloc(1, sizeof(ui_scrollbar_t));
	if (scrollbar == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_scrollbar_ops, (void *) scrollbar,
	    &scrollbar->control);
	if (rc != EOK) {
		free(scrollbar);
		return rc;
	}

	if (dir == ui_sbd_horiz) {
		up_text = resource->textmode ? "\u25c4" : "<";
		down_text = resource->textmode ? "\u25ba" : ">";
	} else {
		up_text = resource->textmode ? "\u25b2" : "^";
		down_text = resource->textmode ? "\u25bc" : "v";
	}

	rc = ui_pbutton_create(resource, up_text, &scrollbar->up_btn);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(scrollbar->up_btn, &ui_scrollbar_up_btn_cb,
	    scrollbar);

	ui_pbutton_set_decor_ops(scrollbar->up_btn,
	    &ui_scrollbar_up_btn_decor_ops, (void *) scrollbar);

	ui_pbutton_set_flags(scrollbar->up_btn, ui_pbf_no_text_depress);

	rc = ui_pbutton_create(resource, down_text, &scrollbar->down_btn);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(scrollbar->down_btn, &ui_scrollbar_down_btn_cb,
	    (void *) scrollbar);

	ui_pbutton_set_decor_ops(scrollbar->down_btn,
	    &ui_scrollbar_down_btn_decor_ops, (void *) scrollbar);

	ui_pbutton_set_flags(scrollbar->down_btn, ui_pbf_no_text_depress);

	scrollbar->thumb_len = resource->textmode ?
	    ui_scrollbar_def_thumb_len_text :
	    ui_scrollbar_def_thumb_len;

	scrollbar->ui = ui;
	scrollbar->window = window;
	scrollbar->dir = dir;
	*rscrollbar = scrollbar;
	return EOK;
error:
	ui_scrollbar_destroy(scrollbar);
	return rc;
}

/** Destroy scrollbar.
 *
 * @param scrollbar Scrollbar or @c NULL
 */
void ui_scrollbar_destroy(ui_scrollbar_t *scrollbar)
{
	if (scrollbar == NULL)
		return;

	ui_pbutton_destroy(scrollbar->up_btn);
	ui_pbutton_destroy(scrollbar->down_btn);
	ui_control_delete(scrollbar->control);
	free(scrollbar);
}

/** Get base control from scrollbar.
 *
 * @param scrollbar Scrollbar
 * @return Control
 */
ui_control_t *ui_scrollbar_ctl(ui_scrollbar_t *scrollbar)
{
	return scrollbar->control;
}

/** Set scrollbar callbacks.
 *
 * @param scrollbar Scrollbar
 * @param cb Scrollbar callbacks
 * @param arg Callback argument
 */
void ui_scrollbar_set_cb(ui_scrollbar_t *scrollbar, ui_scrollbar_cb_t *cb, void *arg)
{
	scrollbar->cb = cb;
	scrollbar->arg = arg;
}

/** Set scrollbar rectangle.
 *
 * @param scrollbar Scrollbar
 * @param rect New scrollbar rectangle
 */
void ui_scrollbar_set_rect(ui_scrollbar_t *scrollbar, gfx_rect_t *rect)
{
	ui_scrollbar_geom_t geom;

	scrollbar->rect = *rect;

	ui_scrollbar_get_geom(scrollbar, &geom);
	ui_pbutton_set_rect(scrollbar->up_btn, &geom.up_btn_rect);
	ui_pbutton_set_rect(scrollbar->down_btn, &geom.down_btn_rect);
}

/** Paint outer thumb frame.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
static errno_t ui_scrollbar_paint_thumb_frame(ui_resource_t *res, gfx_rect_t *rect,
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

/** Paint outset scrollbar bevel.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
static errno_t ui_scrollbar_paint_outset(ui_scrollbar_t *scrollbar,
    gfx_rect_t *rect, gfx_rect_t *inside)
{
	ui_resource_t *resource = ui_window_get_res(scrollbar->window);

	return ui_paint_bevel(resource->gc, rect,
	    resource->btn_highlight_color,
	    resource->btn_shadow_color, ui_scrollbar_thumb_bevel_width, inside);
}

/** Determine scrollbar trough length.
 *
 * Return the size of the space within which the thumb can move
 * (without subtracting the length of the thumb).
 *
 * @param scrollbar Scrollbar
 * @return Scrollbar trough length in pixels
 */
gfx_coord_t ui_scrollbar_trough_length(ui_scrollbar_t *scrollbar)
{
	ui_resource_t *resource;
	gfx_coord2_t dims;
	gfx_coord_t len;
	gfx_coord_t sblen;

	resource = ui_window_get_res(scrollbar->window);

	gfx_rect_dims(&scrollbar->rect, &dims);
	sblen = scrollbar->dir == ui_sbd_horiz ?
	    dims.x : dims.y;

	len = resource->textmode ? ui_scrollbar_btn_len_text :
	    ui_scrollbar_btn_len;

	return sblen - 2 * len;
}

/** Determine scrollbar move length.
 *
 * Return the maximum distance the thumb can move.
 *
 * @param scrollbar Scrollbar
 * @return Scrollbar move length in pixels
 */
gfx_coord_t ui_scrollbar_move_length(ui_scrollbar_t *scrollbar)
{
	return ui_scrollbar_trough_length(scrollbar) -
	    scrollbar->thumb_len;
}

/** Set scrollbar thumb length.
 *
 * @param scrollbar Scrollbar
 * @param len Thumb length in pixels
 */
void ui_scrollbar_set_thumb_length(ui_scrollbar_t *scrollbar, gfx_coord_t len)
{
	ui_resource_t *resource;
	gfx_coord_t min_len;
	gfx_coord_t max_len;

	resource = ui_window_get_res(scrollbar->window);

	min_len = resource->textmode ?
	    ui_scrollbar_min_thumb_len_text :
	    ui_scrollbar_min_thumb_len;

	max_len = ui_scrollbar_trough_length(scrollbar);
	if (len < min_len)
		len = min_len;
	if (len > max_len)
		len = max_len;

	if (len != scrollbar->thumb_len) {
		scrollbar->thumb_len = len;
		(void) ui_scrollbar_paint(scrollbar);
	}
}

/** Get scrollbar thumb position.
 *
 * @param scrollbar Scrollbar
 * @return Scrollbar thumb position in pixels
 */
gfx_coord_t ui_scrollbar_get_pos(ui_scrollbar_t *scrollbar)
{
	return scrollbar->pos;
}

/** Set scrollbar thumb position.
 *
 * The position is clipped to the allowed range.
 *
 * @param scrollbar Scrollbar
 * @parap pos       Scrollbar thumb position in pixels
 */
void ui_scrollbar_set_pos(ui_scrollbar_t *scrollbar, gfx_coord_t pos)
{
	gfx_coord_t length;

	length = ui_scrollbar_move_length(scrollbar);
	if (pos < 0)
		pos = 0;
	if (pos > length)
		pos = length;

	if (pos != scrollbar->pos) {
		scrollbar->pos = pos;
		(void) ui_scrollbar_paint(scrollbar);
		ui_scrollbar_troughs_update(scrollbar,
		    &scrollbar->last_curs_pos);
	}
}

/** Paint scrollbar in graphics mode.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_paint_gfx(ui_scrollbar_t *scrollbar)
{
	ui_resource_t *resource;
	ui_scrollbar_geom_t geom;
	gfx_rect_t brect;
	gfx_rect_t irect;
	errno_t rc;

	resource = ui_window_get_res(scrollbar->window);

	ui_scrollbar_get_geom(scrollbar, &geom);

	/* Paint scrollbar frame */

	rc = ui_paint_inset_frame(resource, &scrollbar->rect, NULL);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar upper trough */
	rc = gfx_set_color(resource->gc,
	    scrollbar->upper_trough_held && scrollbar->upper_trough_inside ?
	    resource->sbar_act_trough_color :
	    resource->sbar_trough_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(resource->gc, &geom.upper_trough_rect);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar lower trough */

	rc = gfx_set_color(resource->gc,
	    scrollbar->lower_trough_held && scrollbar->lower_trough_inside ?
	    resource->sbar_act_trough_color :
	    resource->sbar_trough_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(resource->gc, &geom.lower_trough_rect);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar thumb */
	rc = ui_scrollbar_paint_thumb_frame(resource, &geom.thumb_rect,
	    ui_scrollbar_thumb_frame_thickness, &brect);
	if (rc != EOK)
		goto error;

	rc = ui_scrollbar_paint_outset(scrollbar, &brect, &irect);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(resource->gc, resource->btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(resource->gc, &irect);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->up_btn);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->down_btn);
	if (rc != EOK)
		goto error;

	rc = gfx_update(resource->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint scrollbar in text mode.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_paint_text(ui_scrollbar_t *scrollbar)
{
	ui_resource_t *resource;
	ui_scrollbar_geom_t geom;
	errno_t rc;

	resource = ui_window_get_res(scrollbar->window);
	ui_scrollbar_get_geom(scrollbar, &geom);

	/* Paint scrollbar trough */

	rc = ui_paint_text_rect(resource, &geom.trough_rect,
	    resource->sbar_trough_color, "\u2592");
	if (rc != EOK)
		goto error;

	/* Paint scrollbar thumb */

	rc = ui_paint_text_rect(resource, &geom.thumb_rect,
	    resource->sbar_trough_color, "\u25a0");
	if (rc != EOK)
		goto error;

	/* Paint buttons */

	rc = ui_pbutton_paint(scrollbar->up_btn);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->down_btn);
	if (rc != EOK)
		goto error;

	rc = gfx_update(resource->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint scrollbar.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_paint(ui_scrollbar_t *scrollbar)
{
	ui_resource_t *resource = ui_window_get_res(scrollbar->window);

	if (resource->textmode) {
		return ui_scrollbar_paint_text(scrollbar);
	} else {
		return ui_scrollbar_paint_gfx(scrollbar);
	}
}

/** Get scrollbar geometry.
 *
 * @param scrollbar Scrollbar
 * @param geom Structure to fill in with computed geometry
 */
void ui_scrollbar_get_geom(ui_scrollbar_t *scrollbar, ui_scrollbar_geom_t *geom)
{
	ui_resource_t *resource;
	gfx_coord_t btn_len;
	gfx_rect_t orect;
	gfx_rect_t irect;

	resource = ui_window_get_res(scrollbar->window);

	if (resource->textmode) {
		btn_len = ui_scrollbar_btn_len_text;
	} else {
		btn_len = ui_scrollbar_btn_len;
	}

	if (resource->textmode) {
		irect = scrollbar->rect;
		orect = scrollbar->rect;
	} else {
		ui_paint_get_inset_frame_inside(resource,
		    &scrollbar->rect, &irect);
		ui_paint_get_bevel_inside(resource->gc,
		    &scrollbar->rect, 1, &orect);
	}

	if (scrollbar->dir == ui_sbd_horiz) {
		/* Up button */
		geom->up_btn_rect.p0.x = orect.p0.x;
		geom->up_btn_rect.p0.y = orect.p0.y;
		geom->up_btn_rect.p1.x = orect.p0.x + btn_len;
		geom->up_btn_rect.p1.y = orect.p1.y;

		/* Trough */
		geom->trough_rect.p0.x = geom->up_btn_rect.p1.x;
		geom->trough_rect.p0.y = irect.p0.y;
		geom->trough_rect.p1.x = orect.p1.x - btn_len;
		geom->trough_rect.p1.y = irect.p1.y;

		/* Thumb */
		geom->thumb_rect.p0.x = geom->up_btn_rect.p1.x +
		    scrollbar->pos;
		geom->thumb_rect.p0.y = orect.p0.y;
		geom->thumb_rect.p1.x = geom->thumb_rect.p0.x +
		    scrollbar->thumb_len;
		geom->thumb_rect.p1.y = orect.p1.y;

		/* Upper trough */
		geom->upper_trough_rect.p0 = geom->trough_rect.p0;
		geom->upper_trough_rect.p1.x = geom->thumb_rect.p0.x;
		geom->upper_trough_rect.p1.y = geom->trough_rect.p1.y;

		/* Lower trough */
		geom->lower_trough_rect.p0.x = geom->thumb_rect.p1.x;
		geom->lower_trough_rect.p0.y = geom->trough_rect.p0.y;
		geom->lower_trough_rect.p1 = geom->trough_rect.p1;

		/* Down button */
		geom->down_btn_rect.p0.x = geom->trough_rect.p1.x;
		geom->down_btn_rect.p0.y = orect.p0.y;
		geom->down_btn_rect.p1.x = orect.p1.x;
		geom->down_btn_rect.p1.y = orect.p1.y;
	} else {
		/* Up button */
		geom->up_btn_rect.p0.x = orect.p0.x;
		geom->up_btn_rect.p0.y = orect.p0.y;
		geom->up_btn_rect.p1.x = orect.p1.x;
		geom->up_btn_rect.p1.y = orect.p0.y + btn_len;

		/* Trough */
		geom->trough_rect.p0.x = irect.p0.x;
		geom->trough_rect.p0.y = geom->up_btn_rect.p1.y;
		geom->trough_rect.p1.x = irect.p1.x;
		geom->trough_rect.p1.y = orect.p1.y - btn_len;

		/* Thumb */
		geom->thumb_rect.p0.x = orect.p0.x;
		geom->thumb_rect.p0.y = geom->up_btn_rect.p1.y +
		    scrollbar->pos;
		geom->thumb_rect.p1.x = orect.p1.x;
		geom->thumb_rect.p1.y = geom->thumb_rect.p0.y +
		    scrollbar->thumb_len;

		/* Upper trough */
		geom->upper_trough_rect.p0 = geom->trough_rect.p0;
		geom->upper_trough_rect.p1.x = geom->trough_rect.p1.x;
		geom->upper_trough_rect.p1.y = geom->thumb_rect.p0.y;

		/* Lower trough */
		geom->lower_trough_rect.p0.x = geom->trough_rect.p0.x;
		geom->lower_trough_rect.p0.y = geom->thumb_rect.p1.y;
		geom->lower_trough_rect.p1 = geom->trough_rect.p1;

		/* Down button */
		geom->down_btn_rect.p0.x = orect.p0.x;
		geom->down_btn_rect.p0.y = geom->trough_rect.p1.y;
		geom->down_btn_rect.p1.x = orect.p1.x;
		geom->down_btn_rect.p1.y = orect.p1.y;
	}
}

/** Press down scrollbar thumb.
 *
 * @param scrollbar Scrollbar
 * @param pos Pointer position
 */
void ui_scrollbar_thumb_press(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	if (scrollbar->thumb_held)
		return;

	scrollbar->thumb_held = true;
	scrollbar->press_pos = *pos;
	scrollbar->last_pos = scrollbar->pos;

	(void) ui_scrollbar_paint(scrollbar);
}

/** Press down scrollbar upper trough.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_upper_trough_press(ui_scrollbar_t *scrollbar)
{
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	scrollbar->upper_trough_held = true;
	scrollbar->upper_trough_inside = true;

	ui_clickmatic_set_cb(clickmatic, &ui_scrollbar_clickmatic_page_up_cb,
	    (void *) scrollbar);
	ui_clickmatic_press(clickmatic);
}

/** Press down scrollbar lower trough.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_lower_trough_press(ui_scrollbar_t *scrollbar)
{
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	scrollbar->lower_trough_held = true;
	scrollbar->lower_trough_inside = true;

	ui_clickmatic_set_cb(clickmatic, &ui_scrollbar_clickmatic_page_down_cb,
	    (void *) scrollbar);
	ui_clickmatic_press(clickmatic);
}

/** Release scrollbar.
 *
 * @param scrollbar Scrollbar
 * @param pos Pointer position
 */
void ui_scrollbar_release(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	ui_clickmatic_t *clickmatic;

	if (scrollbar->thumb_held) {
		ui_scrollbar_update(scrollbar, pos);
		scrollbar->thumb_held = false;
	}

	if (scrollbar->upper_trough_held || scrollbar->lower_trough_held) {
		clickmatic = ui_get_clickmatic(scrollbar->ui);
		ui_clickmatic_release(clickmatic);
		ui_clickmatic_set_cb(clickmatic, NULL, NULL);

		scrollbar->upper_trough_held = false;
		scrollbar->lower_trough_held = false;
		(void) ui_scrollbar_paint(scrollbar);
	}
}

/** Update state of scrollbar throuhgs.
 *
 * Update state of scrollbar troughs after mouse cursor or thumb has moved.
 *
 * @param scrollbar Scrollbar
 * @param pos Mouse cursor position
 */
void ui_scrollbar_troughs_update(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	ui_scrollbar_geom_t geom;
	bool inside_up;
	bool inside_down;

	ui_scrollbar_get_geom(scrollbar, &geom);

	inside_up = gfx_pix_inside_rect(pos, &geom.upper_trough_rect);
	inside_down = gfx_pix_inside_rect(pos, &geom.lower_trough_rect);

	if (inside_up && !scrollbar->upper_trough_inside) {
		scrollbar->upper_trough_inside = true;
		(void) ui_scrollbar_paint(scrollbar);
	} else if (!inside_up && scrollbar->upper_trough_inside) {
		scrollbar->upper_trough_inside = false;
		(void) ui_scrollbar_paint(scrollbar);
	}

	if (inside_down && !scrollbar->lower_trough_inside) {
		scrollbar->lower_trough_inside = true;
		(void) ui_scrollbar_paint(scrollbar);
	} else if (!inside_down && scrollbar->lower_trough_inside) {
		scrollbar->lower_trough_inside = false;
		(void) ui_scrollbar_paint(scrollbar);
	}
}

/** Scrollbar handler for pointer movement.
 *
 * @param scrollbar Scrollbar
 * @param pos New pointer position
 */
void ui_scrollbar_update(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	gfx_coord_t spos;

	if (scrollbar->thumb_held) {
		if (scrollbar->dir == ui_sbd_horiz)
			spos = scrollbar->last_pos + pos->x - scrollbar->press_pos.x;
		else
			spos = scrollbar->last_pos + pos->y - scrollbar->press_pos.y;

		ui_scrollbar_set_pos(scrollbar, spos);
		ui_scrollbar_moved(scrollbar, scrollbar->pos);
	}

	ui_scrollbar_troughs_update(scrollbar, pos);
}

/** Scrollbar up button was pressed.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_up(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->cb != NULL && scrollbar->cb->up != NULL)
		scrollbar->cb->up(scrollbar, scrollbar->arg);
}

/** Scrollbar down button was pressed.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_down(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->cb != NULL && scrollbar->cb->down != NULL)
		scrollbar->cb->down(scrollbar, scrollbar->arg);
}

/** Scrollbar upper trough was pressed.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_page_up(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->cb != NULL && scrollbar->cb->page_up != NULL)
		scrollbar->cb->page_up(scrollbar, scrollbar->arg);
}

/** Scrollbar lower trough was pressed.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_page_down(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->cb != NULL && scrollbar->cb->page_down != NULL)
		scrollbar->cb->page_down(scrollbar, scrollbar->arg);
}

/** Scrollbar was moved.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_moved(ui_scrollbar_t *scrollbar, gfx_coord_t pos)
{
	if (scrollbar->cb != NULL && scrollbar->cb->moved != NULL)
		scrollbar->cb->moved(scrollbar, scrollbar->arg, pos);
}

/** Handle scrollbar position event.
 *
 * @param scrollbar Scrollbar
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_scrollbar_pos_event(ui_scrollbar_t *scrollbar, pos_event_t *event)
{
	gfx_coord2_t pos;
	ui_evclaim_t claimed;
	ui_scrollbar_geom_t geom;

	ui_scrollbar_get_geom(scrollbar, &geom);

	pos.x = event->hpos;
	pos.y = event->vpos;

	claimed = ui_pbutton_pos_event(scrollbar->up_btn, event);
	if (claimed == ui_claimed)
		return ui_claimed;

	claimed = ui_pbutton_pos_event(scrollbar->down_btn, event);
	if (claimed == ui_claimed)
		return ui_claimed;

	switch (event->type) {
	case POS_PRESS:
		if (gfx_pix_inside_rect(&pos, &geom.thumb_rect)) {
			ui_scrollbar_thumb_press(scrollbar, &pos);
			return ui_claimed;
		}
		if (gfx_pix_inside_rect(&pos, &geom.upper_trough_rect)) {
			ui_scrollbar_upper_trough_press(scrollbar);
			return ui_claimed;
		}
		if (gfx_pix_inside_rect(&pos, &geom.lower_trough_rect)) {
			ui_scrollbar_lower_trough_press(scrollbar);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (scrollbar->thumb_held || scrollbar->upper_trough_held ||
		    scrollbar->lower_trough_held) {
			ui_scrollbar_release(scrollbar, &pos);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		ui_scrollbar_update(scrollbar, &pos);
		scrollbar->last_curs_pos = pos;
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Scrollbar up button pressed.
 *
 * @param pbutton Up button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_up_btn_down(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	ui_clickmatic_set_cb(clickmatic, &ui_scrollbar_clickmatic_up_cb,
	    (void *) scrollbar);
	ui_clickmatic_press(clickmatic);
}

/** Scrollbar up button released.
 *
 * @param pbutton Up button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_up_btn_up(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	ui_clickmatic_release(clickmatic);
	ui_clickmatic_set_cb(clickmatic, NULL, NULL);
}

/** Paint up button decoration.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_scrollbar_t *)
 * @param pos Center position
 */
static errno_t ui_scrollbar_up_btn_decor_paint(ui_pbutton_t *pbutton,
    void *arg, gfx_coord2_t *pos)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	errno_t rc;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->btn_text_color);
	if (rc != EOK)
		return rc;

	if (scrollbar->dir == ui_sbd_horiz)
		return ui_paint_left_triangle(pbutton->res->gc, pos, 5);
	else
		return ui_paint_up_triangle(pbutton->res->gc, pos, 5);
}

/** Paint down button decoration.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_scrollbar_t *)
 * @param pos Center position
 */
static errno_t ui_scrollbar_down_btn_decor_paint(ui_pbutton_t *pbutton,
    void *arg, gfx_coord2_t *pos)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	errno_t rc;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->btn_text_color);
	if (rc != EOK)
		return rc;

	if (scrollbar->dir == ui_sbd_horiz)
		return ui_paint_right_triangle(pbutton->res->gc, pos, 5);
	else
		return ui_paint_down_triangle(pbutton->res->gc, pos, 5);
}

/** Scrollbar down button pressed.
 *
 * @param pbutton Down button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_down_btn_down(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	ui_clickmatic_set_cb(clickmatic, &ui_scrollbar_clickmatic_down_cb,
	    (void *) scrollbar);
	ui_clickmatic_press(clickmatic);
}

/** Scrollbar down button released.
 *
 * @param pbutton Down button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_down_btn_up(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;
	ui_clickmatic_t *clickmatic = ui_get_clickmatic(scrollbar->ui);

	ui_clickmatic_release(clickmatic);
	ui_clickmatic_set_cb(clickmatic, NULL, NULL);
}

/** Destroy scrollbar control.
 *
 * @param arg Argument (ui_scrollbar_t *)
 */
void ui_scrollbar_ctl_destroy(void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *) arg;

	ui_scrollbar_destroy(scrollbar);
}

/** Paint scrollbar control.
 *
 * @param arg Argument (ui_scrollbar_t *)
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_ctl_paint(void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *) arg;

	return ui_scrollbar_paint(scrollbar);
}

/** Handle scrollbar control position event.
 *
 * @param arg Argument (ui_scrollbar_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_scrollbar_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	return ui_scrollbar_pos_event(scrollbar, event);
}

/** Scrollbar clickmatic up button click event.
 *
 * @param clickmatic Clickmatic
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_cm_up(ui_clickmatic_t *clickmatic, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	if (scrollbar->up_btn->inside)
		ui_scrollbar_up(scrollbar);
}

/** Scrollbar clickmatic down button click event.
 *
 * @param clickmatic Clickmatic
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_cm_down(ui_clickmatic_t *clickmatic, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	if (scrollbar->down_btn->inside)
		ui_scrollbar_down(scrollbar);
}

/** Scrollbar clickmatic upper trough click event.
 *
 * @param clickmatic Clickmatic
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_cm_page_up(ui_clickmatic_t *clickmatic, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	if (scrollbar->upper_trough_inside)
		ui_scrollbar_page_up(scrollbar);
}

/** Scrollbar clickmatic lower trough click event.
 *
 * @param clickmatic Clickmatic
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_cm_page_down(ui_clickmatic_t *clickmatic, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	if (scrollbar->lower_trough_inside)
		ui_scrollbar_page_down(scrollbar);
}

/** @}
 */
