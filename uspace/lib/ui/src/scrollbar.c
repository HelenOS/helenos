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
 * @file Scrollbar
 *
 * Anatomy of a horizontal scrollbar:
 *
 *       Up                Down
 *      through           through
 * +---+------+--------+---------+---+
 * | < |      |   |||  |         | > |
 * +---+------+--------+---------+---+
 *  Up           Thumb           Down
 * button                       button
 *
 *     +-------- Through --------+
 *
 * Scrollbar uses the same terminology whether it is running in horizontal
 * or vertical mode, in horizontal mode up means left, down means right
 * (i.e. lower and higher coordinates, respectively).
 *
 * The thumb can be dragged to a specific position, resulting in a move
 * event. The up/down buttons generate up/down events. Pressing a mouse
 * button on the up/down through generates page up / page down events.
 *
 * TODO: Up/down buttons/throughs should be equipped with an autorepeat
 * mechanism: after an initial delay, start repeating at a preset rate.
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
#include <ui/pbutton.h>
#include <ui/scrollbar.h>
#include "../private/resource.h"
#include "../private/scrollbar.h"

enum {
	/** Scrollbar button width */
	ui_scrollbar_btn_len = 20,
	/** Scrollbar button width in text mode */
	ui_scrollbar_btn_len_text = 1,
	/** Scrollbar thumb frame thickness */
	ui_scrollbar_thumb_frame_thickness = 1,
	/** Scrollbar thumb bevel width */
	ui_scrollbar_thumb_bevel_width = 2,
	/** Scrollbar default thumb length */
	ui_scrollbar_def_thumb_len = 20,
	/** Scrollbar default thumb length in text mode */
	ui_scrollbar_def_thumb_len_text = 1,
	/** Scrollbar minimum thumb length */
	ui_scrollbar_min_thumb_len = 10,
	/** Scrollbar minimum thumb length in text mode */
	ui_scrollbar_min_thumb_len_text = 1,
};

static void ui_scrollbar_btn_up_clicked(ui_pbutton_t *, void *);
static void ui_scrollbar_btn_down_clicked(ui_pbutton_t *, void *);
static void ui_scrollbar_ctl_destroy(void *);
static errno_t ui_scrollbar_ctl_paint(void *);
static ui_evclaim_t ui_scrollbar_ctl_pos_event(void *, pos_event_t *);

ui_pbutton_cb_t ui_scrollbar_btn_up_cb = {
	.clicked = ui_scrollbar_btn_up_clicked
};

ui_pbutton_cb_t ui_scrollbar_btn_down_cb = {
	.clicked = ui_scrollbar_btn_down_clicked
};

/** Scrollbar control ops */
ui_control_ops_t ui_scrollbar_ops = {
	.destroy = ui_scrollbar_ctl_destroy,
	.paint = ui_scrollbar_ctl_paint,
	.pos_event = ui_scrollbar_ctl_pos_event
};

/** Create new scrollbar.
 *
 * @param resource UI resource
 * @param rscrollbar Place to store pointer to new scrollbar
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_scrollbar_create(ui_resource_t *resource,
    ui_scrollbar_t **rscrollbar)
{
	ui_scrollbar_t *scrollbar;
	errno_t rc;

	scrollbar = calloc(1, sizeof(ui_scrollbar_t));
	if (scrollbar == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_scrollbar_ops, (void *) scrollbar,
	    &scrollbar->control);
	if (rc != EOK) {
		free(scrollbar);
		return rc;
	}

	rc = ui_pbutton_create(resource, resource->textmode ? "\u25c4" : "<",
	    &scrollbar->btn_up);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(scrollbar->btn_up, &ui_scrollbar_btn_up_cb,
	    (void *) scrollbar);

	rc = ui_pbutton_create(resource, resource->textmode ? "\u25ba" : ">",
	    &scrollbar->btn_down);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(scrollbar->btn_down, &ui_scrollbar_btn_down_cb,
	    (void *) scrollbar);

	scrollbar->thumb_len = resource->textmode ?
	    ui_scrollbar_def_thumb_len_text :
	    ui_scrollbar_def_thumb_len;

	scrollbar->res = resource;
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

	ui_pbutton_destroy(scrollbar->btn_up);
	ui_pbutton_destroy(scrollbar->btn_down);
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
	ui_pbutton_set_rect(scrollbar->btn_up, &geom.up_btn_rect);
	ui_pbutton_set_rect(scrollbar->btn_down, &geom.down_btn_rect);
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
	return ui_paint_bevel(scrollbar->res->gc, rect,
	    scrollbar->res->btn_highlight_color,
	    scrollbar->res->btn_shadow_color, ui_scrollbar_thumb_bevel_width, inside);
}

/** Determine scrollbar thumb rectagle.
 *
 * @param scrollbar Scrollbar
 */
static void ui_scrollbar_thumb_rect(ui_scrollbar_t *scrollbar, gfx_rect_t *rect)
{
	ui_scrollbar_geom_t geom;

	ui_scrollbar_get_geom(scrollbar, &geom);
	*rect = geom.thumb_rect;
}

/** Determine scrollbar through length.
 *
 * Return the size of the space within which the thumb can move
 * (without subtracting the length of the thumb).
 *
 * @param scrollbar Scrollbar
 * @return Scrollbar through length in pixels
 */
gfx_coord_t ui_scrollbar_through_length(ui_scrollbar_t *scrollbar)
{
	gfx_coord2_t dims;
	gfx_coord_t w;

	gfx_rect_dims(&scrollbar->rect, &dims);
	w = scrollbar->res->textmode ? ui_scrollbar_btn_len_text :
	    ui_scrollbar_btn_len;
	return dims.x - 2 * w;
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
	return ui_scrollbar_through_length(scrollbar) -
	    scrollbar->thumb_len;
}

/** Set scrollbar thumb length.
 *
 * @param scrollbar Scrollbar
 * @param len Thumb length in pixels
 */
void ui_scrollbar_set_thumb_length(ui_scrollbar_t *scrollbar, gfx_coord_t len)
{
	gfx_coord_t min_len;
	gfx_coord_t max_len;

	min_len = scrollbar->res->textmode ?
	    ui_scrollbar_min_thumb_len_text :
	    ui_scrollbar_min_thumb_len;

	max_len = ui_scrollbar_through_length(scrollbar);
	if (len < min_len)
		len = min_len;
	if (len > max_len)
		len = max_len;

	if (len != scrollbar->thumb_len) {
		(void) ui_scrollbar_thumb_clear(scrollbar);
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
		(void) ui_scrollbar_thumb_clear(scrollbar);
		scrollbar->pos = pos;
		(void) ui_scrollbar_paint(scrollbar);
		ui_scrollbar_moved(scrollbar, pos);
	}
}

/** Paint scrollbar in graphics mode.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_paint_gfx(ui_scrollbar_t *scrollbar)
{
	ui_scrollbar_geom_t geom;
	gfx_rect_t brect;
	gfx_rect_t irect;
	errno_t rc;

	ui_scrollbar_get_geom(scrollbar, &geom);

	/* Paint scrollbar frame */

	rc = ui_paint_inset_frame(scrollbar->res, &scrollbar->rect, NULL);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar up through */
	rc = gfx_set_color(scrollbar->res->gc,
	    scrollbar->up_through_held ?
	    scrollbar->res->sbar_act_through_color :
	    scrollbar->res->sbar_through_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(scrollbar->res->gc, &geom.up_through_rect);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar down through */

	rc = gfx_set_color(scrollbar->res->gc,
	    scrollbar->down_through_held ?
	    scrollbar->res->sbar_act_through_color :
	    scrollbar->res->sbar_through_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(scrollbar->res->gc, &geom.down_through_rect);
	if (rc != EOK)
		goto error;

	/* Paint scrollbar thumb */

	rc = ui_scrollbar_paint_thumb_frame(scrollbar->res, &geom.thumb_rect,
	    ui_scrollbar_thumb_frame_thickness, &brect);
	if (rc != EOK)
		goto error;

	rc = ui_scrollbar_paint_outset(scrollbar, &brect, &irect);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(scrollbar->res->gc, scrollbar->res->btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(scrollbar->res->gc, &irect);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->btn_up);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->btn_down);
	if (rc != EOK)
		goto error;

	rc = gfx_update(scrollbar->res->gc);
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
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_coord_t w, i;
	char *buf;
	const char *gchar;
	size_t gcharsz;
	errno_t rc;

	/* Paint scrollbar through */

	pos = scrollbar->rect.p0;
	pos.x += ui_scrollbar_btn_len_text;

	gfx_text_fmt_init(&fmt);
	fmt.font = scrollbar->res->font;
	fmt.color = scrollbar->res->sbar_through_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	w = scrollbar->rect.p1.x - scrollbar->rect.p0.x -
	    2 * ui_scrollbar_btn_len_text;
	assert(w >= 0);
	if (w < 0)
		return EINVAL;

	gchar = "\u2592";
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

	/* Paint scrollbar thumb */

	pos.x += scrollbar->pos;

	gchar = "\u25a0";
	gcharsz = str_size(gchar);
	w = scrollbar->thumb_len;

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

	rc = ui_pbutton_paint(scrollbar->btn_up);
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_paint(scrollbar->btn_down);
	if (rc != EOK)
		goto error;

	rc = gfx_update(scrollbar->res->gc);
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
	if (scrollbar->res->textmode)
		return ui_scrollbar_paint_text(scrollbar);
	else
		return ui_scrollbar_paint_gfx(scrollbar);
}

/** Get scrollbar geometry.
 *
 * @param scrollbar Scrollbar
 * @param geom Structure to fill in with computed geometry
 */
void ui_scrollbar_get_geom(ui_scrollbar_t *scrollbar, ui_scrollbar_geom_t *geom)
{
	gfx_coord_t btn_len;
	gfx_rect_t orect;
	gfx_rect_t irect;

	if (scrollbar->res->textmode) {
		btn_len = ui_scrollbar_btn_len_text;
	} else {
		btn_len = ui_scrollbar_btn_len;
	}

	if (scrollbar->res->textmode) {
		irect = scrollbar->rect;
		orect = scrollbar->rect;
	} else {
		ui_paint_get_inset_frame_inside(scrollbar->res,
		    &scrollbar->rect, &irect);
		ui_paint_get_bevel_inside(scrollbar->res->gc,
		    &scrollbar->rect, 1, &orect);
	}

	/* Up button */
	geom->up_btn_rect.p0.x = orect.p0.x;
	geom->up_btn_rect.p0.y = orect.p0.y;
	geom->up_btn_rect.p1.x = orect.p0.x + btn_len;
	geom->up_btn_rect.p1.y = orect.p1.y;

	/* Through */
	geom->through_rect.p0.x = geom->up_btn_rect.p1.x;
	geom->through_rect.p0.y = irect.p0.y;
	geom->through_rect.p1.x = orect.p1.x - btn_len;
	geom->through_rect.p1.y = irect.p1.y;

	/* Thumb */
	geom->thumb_rect.p0.x = geom->up_btn_rect.p1.x + scrollbar->pos;
	geom->thumb_rect.p0.y = orect.p0.y;
	geom->thumb_rect.p1.x = geom->thumb_rect.p0.x + scrollbar->thumb_len;
	geom->thumb_rect.p1.y = orect.p1.y;

	/* Up through */
	geom->up_through_rect.p0 = geom->through_rect.p0;
	geom->up_through_rect.p1.x = geom->thumb_rect.p0.x;
	geom->up_through_rect.p1.y = geom->through_rect.p1.y;

	/* Down through */
	geom->down_through_rect.p0.x = geom->thumb_rect.p1.x;
	geom->down_through_rect.p0.y = geom->through_rect.p0.y;
	geom->down_through_rect.p1 = geom->through_rect.p1;

	/* Down button */
	geom->down_btn_rect.p0.x = geom->through_rect.p1.x;
	geom->down_btn_rect.p0.y = orect.p0.y;
	geom->down_btn_rect.p1.x = orect.p1.x;
	geom->down_btn_rect.p1.y = orect.p1.y;
}

/** Clear scrollbar thumb.
 *
 * @param scrollbar Scrollbar
 * @return EOK on success or an error code
 */
errno_t ui_scrollbar_thumb_clear(ui_scrollbar_t *scrollbar)
{
	gfx_rect_t rect;
	errno_t rc;

	/* No need to clear thumb in text mode */
	if (scrollbar->res->textmode)
		return EOK;

	ui_scrollbar_thumb_rect(scrollbar, &rect);

	rc = gfx_set_color(scrollbar->res->gc, scrollbar->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(scrollbar->res->gc, &rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
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

/** Press down scrollbar up through.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_up_through_press(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->up_through_held)
		return;

	scrollbar->up_through_held = true;
	(void) ui_scrollbar_paint(scrollbar);

	ui_scrollbar_page_up(scrollbar);
}

/** Press down scrollbar down through.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_down_through_press(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->down_through_held)
		return;

	scrollbar->down_through_held = true;
	(void) ui_scrollbar_paint(scrollbar);

	ui_scrollbar_page_down(scrollbar);
}

/** Release scrollbar.
 *
 * @param scrollbar Scrollbar
 * @param pos Pointer position
 */
void ui_scrollbar_release(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	if (scrollbar->thumb_held) {
		ui_scrollbar_update(scrollbar, pos);
		scrollbar->thumb_held = false;
	}

	if (scrollbar->up_through_held || scrollbar->down_through_held) {
		scrollbar->up_through_held = false;
		scrollbar->down_through_held = false;
		(void) ui_scrollbar_paint(scrollbar);
	}
}

/** Pointer moved.
 *
 * @param scrollbar Scrollbar
 * @param pos New pointer position
 */
void ui_scrollbar_update(ui_scrollbar_t *scrollbar, gfx_coord2_t *pos)
{
	gfx_coord_t spos;

	if (scrollbar->thumb_held) {
		spos = scrollbar->last_pos + pos->x - scrollbar->press_pos.x;
		ui_scrollbar_set_pos(scrollbar, spos);
	}
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

/** Scrollbar up through was pressed.
 *
 * @param scrollbar Scrollbar
 */
void ui_scrollbar_page_up(ui_scrollbar_t *scrollbar)
{
	if (scrollbar->cb != NULL && scrollbar->cb->page_up != NULL)
		scrollbar->cb->page_up(scrollbar, scrollbar->arg);
}

/** Scrollbar down through was pressed.
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

	claimed = ui_pbutton_pos_event(scrollbar->btn_up, event);
	if (claimed == ui_claimed)
		return ui_claimed;

	claimed = ui_pbutton_pos_event(scrollbar->btn_down, event);
	if (claimed == ui_claimed)
		return ui_claimed;

	switch (event->type) {
	case POS_PRESS:
		if (gfx_pix_inside_rect(&pos, &geom.thumb_rect)) {
			ui_scrollbar_thumb_press(scrollbar, &pos);
			return ui_claimed;
		}
		if (gfx_pix_inside_rect(&pos, &geom.up_through_rect)) {
			ui_scrollbar_up_through_press(scrollbar);
			return ui_claimed;
		}
		if (gfx_pix_inside_rect(&pos, &geom.down_through_rect)) {
			ui_scrollbar_down_through_press(scrollbar);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (scrollbar->thumb_held || scrollbar->up_through_held ||
		    scrollbar->down_through_held) {
			ui_scrollbar_release(scrollbar, &pos);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		ui_scrollbar_update(scrollbar, &pos);
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Scrollbar up button clicked.
 *
 * @param pbutton Up button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_btn_up_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	ui_scrollbar_up(scrollbar);
}

/** Scrollbar down button clicked.
 *
 * @param pbutton Down button
 * @param arg Argument (ui_scrollbar_t *)
 */
static void ui_scrollbar_btn_down_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *)arg;

	ui_scrollbar_down(scrollbar);
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
	ui_scrollbar_t *scrollbar = (ui_scrollbar_t *) arg;

	return ui_scrollbar_pos_event(scrollbar, event);
}

/** @}
 */
