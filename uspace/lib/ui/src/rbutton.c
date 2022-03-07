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
 * @file Radio button
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
#include <ui/rbutton.h>
#include "../private/rbutton.h"
#include "../private/resource.h"

enum {
	rbutton_oframe_r = 7,
	rbutton_iframe_r = 6,
	rbutton_interior_r = 5,
	rbutton_indicator_r = 3,
	rbutton_label_margin = 8
};

static void ui_rbutton_ctl_destroy(void *);
static errno_t ui_rbutton_ctl_paint(void *);
static ui_evclaim_t ui_rbutton_ctl_pos_event(void *, pos_event_t *);

/** Radio button control ops */
ui_control_ops_t ui_rbutton_ops = {
	.destroy = ui_rbutton_ctl_destroy,
	.paint = ui_rbutton_ctl_paint,
	.pos_event = ui_rbutton_ctl_pos_event
};

/** Create new radio button group.
 *
 * @param res UI resource
 * @param rgroup Place to store pointer to new radio button group
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_rbutton_group_create(ui_resource_t *res, ui_rbutton_group_t **rgroup)
{
	ui_rbutton_group_t *group;

	group = calloc(1, sizeof(ui_rbutton_group_t));
	if (group == NULL)
		return ENOMEM;

	group->res = res;
	*rgroup = group;
	return EOK;
}

/** Destroy radio button group.
 *
 * @param group Radio button group or @c NULL
 */
void ui_rbutton_group_destroy(ui_rbutton_group_t *group)
{
	if (group == NULL)
		return;

	free(group);
}

/** Create new radio button.
 *
 * @param group Radio button group
 * @param caption Caption
 * @param arg Callback argument
 * @param rrbutton Place to store pointer to new radio button
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_rbutton_create(ui_rbutton_group_t *group, const char *caption,
    void *arg, ui_rbutton_t **rrbutton)
{
	ui_rbutton_t *rbutton;
	errno_t rc;

	rbutton = calloc(1, sizeof(ui_rbutton_t));
	if (rbutton == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_rbutton_ops, (void *) rbutton,
	    &rbutton->control);
	if (rc != EOK) {
		free(rbutton);
		return rc;
	}

	rbutton->caption = str_dup(caption);
	if (rbutton->caption == NULL) {
		ui_control_delete(rbutton->control);
		free(rbutton);
		return ENOMEM;
	}

	rbutton->group = group;
	rbutton->arg = arg;

	if (group->selected == NULL)
		group->selected = rbutton;

	*rrbutton = rbutton;
	return EOK;
}

/** Destroy radio button.
 *
 * @param rbutton Radio button or @c NULL
 */
void ui_rbutton_destroy(ui_rbutton_t *rbutton)
{
	if (rbutton == NULL)
		return;

	ui_control_delete(rbutton->control);
	free(rbutton);
}

/** Get base control from radio button.
 *
 * @param rbutton Radio button
 * @return Control
 */
ui_control_t *ui_rbutton_ctl(ui_rbutton_t *rbutton)
{
	return rbutton->control;
}

/** Set radio button group callbacks.
 *
 * @param group Radio button group
 * @param cb Radio button group callbacks
 * @param arg Callback argument
 */
void ui_rbutton_group_set_cb(ui_rbutton_group_t *group,
    ui_rbutton_group_cb_t *cb, void *arg)
{
	group->cb = cb;
	group->arg = arg;
}

/** Set button rectangle.
 *
 * @param rbutton Button
 * @param rect New button rectangle
 */
void ui_rbutton_set_rect(ui_rbutton_t *rbutton, gfx_rect_t *rect)
{
	rbutton->rect = *rect;
}

/** Paint radio button in graphics mode.
 *
 * @param rbutton Radio button
 * @return EOK on success or an error code
 */
errno_t ui_rbutton_paint_gfx(ui_rbutton_t *rbutton)
{
	gfx_coord2_t pos;
	gfx_coord2_t center;
	gfx_text_fmt_t fmt;
	bool depressed;
	errno_t rc;

	center.x = rbutton->rect.p0.x + rbutton_oframe_r;
	center.y = rbutton->rect.p0.y + rbutton_oframe_r;

	/* Paint radio button frame */

	rc = gfx_set_color(rbutton->group->res->gc,
	    rbutton->group->res->wnd_shadow_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_filled_circle(rbutton->group->res->gc,
	    &center, rbutton_oframe_r, ui_fcircle_upleft);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(rbutton->group->res->gc,
	    rbutton->group->res->wnd_highlight_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_filled_circle(rbutton->group->res->gc,
	    &center, rbutton_oframe_r, ui_fcircle_lowright);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(rbutton->group->res->gc,
	    rbutton->group->res->wnd_frame_sh_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_filled_circle(rbutton->group->res->gc,
	    &center, rbutton_iframe_r, ui_fcircle_upleft);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(rbutton->group->res->gc,
	    rbutton->group->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_filled_circle(rbutton->group->res->gc,
	    &center, rbutton_iframe_r, ui_fcircle_lowright);
	if (rc != EOK)
		goto error;

	/* Paint radio button interior */
	depressed = rbutton->held && rbutton->inside;

	rc = gfx_set_color(rbutton->group->res->gc, depressed ?
	    rbutton->group->res->entry_act_bg_color :
	    rbutton->group->res->entry_bg_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_filled_circle(rbutton->group->res->gc,
	    &center, rbutton_interior_r, ui_fcircle_entire);
	if (rc != EOK)
		goto error;

	/* Paint active mark */

	if (rbutton->group->selected == rbutton) {
		rc = gfx_set_color(rbutton->group->res->gc,
		    rbutton->group->res->entry_fg_color);
		if (rc != EOK)
			goto error;

		rc = ui_paint_filled_circle(rbutton->group->res->gc,
		    &center, rbutton_indicator_r, ui_fcircle_entire);
		if (rc != EOK)
			goto error;
	}

	/* Paint rbutton label */

	pos.x = center.x + rbutton_oframe_r + rbutton_label_margin;
	pos.y = center.y;

	gfx_text_fmt_init(&fmt);
	fmt.font = rbutton->group->res->font;
	fmt.color = rbutton->group->res->wnd_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_center;

	rc = gfx_puttext(&pos, &fmt, rbutton->caption);
	if (rc != EOK)
		goto error;

	rc = gfx_update(rbutton->group->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint radio button in text mode.
 *
 * @param rbutton Radio button
 * @return EOK on success or an error code
 */
errno_t ui_rbutton_paint_text(ui_rbutton_t *rbutton)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	bool depressed;
	errno_t rc;

	/* Paint radio button */

	depressed = rbutton->held && rbutton->inside;

	pos.x = rbutton->rect.p0.x;
	pos.y = rbutton->rect.p0.y;

	gfx_text_fmt_init(&fmt);
	fmt.font = rbutton->group->res->font;
	fmt.color = depressed ? rbutton->group->res->entry_act_bg_color :
	    rbutton->group->res->wnd_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	rc = gfx_puttext(&pos, &fmt, rbutton->group->selected == rbutton ?
	    "(\u2022)" : "( )");
	if (rc != EOK)
		goto error;

	/* Paint radio button label */

	pos.x += 4;

	fmt.color = rbutton->group->res->wnd_text_color;

	rc = gfx_puttext(&pos, &fmt, rbutton->caption);
	if (rc != EOK)
		goto error;

	rc = gfx_update(rbutton->group->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint radio button.
 *
 * @param rbutton Radio button
 * @return EOK on success or an error code
 */
errno_t ui_rbutton_paint(ui_rbutton_t *rbutton)
{
	if (rbutton->group->res->textmode)
		return ui_rbutton_paint_text(rbutton);
	else
		return ui_rbutton_paint_gfx(rbutton);
}

/** Press down button.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_press(ui_rbutton_t *rbutton)
{
	if (rbutton->held)
		return;

	rbutton->inside = true;
	rbutton->held = true;
	(void) ui_rbutton_paint(rbutton);
}

/** Release button.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_release(ui_rbutton_t *rbutton)
{
	if (!rbutton->held)
		return;

	rbutton->held = false;

	if (rbutton->inside) {
		/* Activate radio button */
		ui_rbutton_select(rbutton);
	}
}

/** Pointer entered button.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_enter(ui_rbutton_t *rbutton)
{
	if (rbutton->inside)
		return;

	rbutton->inside = true;
	if (rbutton->held)
		(void) ui_rbutton_paint(rbutton);
}

/** Pointer left button.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_leave(ui_rbutton_t *rbutton)
{
	if (!rbutton->inside)
		return;

	rbutton->inside = false;
	if (rbutton->held)
		(void) ui_rbutton_paint(rbutton);
}

/** Select radio button.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_select(ui_rbutton_t *rbutton)
{
	ui_rbutton_t *old_selected;

	old_selected = rbutton->group->selected;

	if (old_selected != rbutton) {
		rbutton->group->selected = rbutton;
		ui_rbutton_paint(old_selected);
	}

	/* Repaint and notify */
	(void) ui_rbutton_paint(rbutton);

	if (old_selected != rbutton)
		ui_rbutton_selected(rbutton);
}

/** Notify that button was selected.
 *
 * @param rbutton Radio button
 */
void ui_rbutton_selected(ui_rbutton_t *rbutton)
{
	ui_rbutton_group_t *group = rbutton->group;

	if (group->cb != NULL && group->cb->selected != NULL) {
		group->cb->selected(group, group->arg, rbutton->arg);
	}
}

/** Handle radio button position event.
 *
 * @param rbutton Radio button
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_rbutton_pos_event(ui_rbutton_t *rbutton, pos_event_t *event)
{
	gfx_coord2_t pos;
	bool inside;

	pos.x = event->hpos;
	pos.y = event->vpos;

	inside = gfx_pix_inside_rect(&pos, &rbutton->rect);

	switch (event->type) {
	case POS_PRESS:
		if (inside) {
			ui_rbutton_press(rbutton);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (rbutton->held) {
			ui_rbutton_release(rbutton);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		if (inside && !rbutton->inside) {
			ui_rbutton_enter(rbutton);
			return ui_claimed;
		} else if (!inside && rbutton->inside) {
			ui_rbutton_leave(rbutton);
		}
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Destroy radio button control.
 *
 * @param arg Argument (ui_rbutton_t *)
 */
void ui_rbutton_ctl_destroy(void *arg)
{
	ui_rbutton_t *rbutton = (ui_rbutton_t *) arg;

	ui_rbutton_destroy(rbutton);
}

/** Paint radio button control.
 *
 * @param arg Argument (ui_rbutton_t *)
 * @return EOK on success or an error code
 */
errno_t ui_rbutton_ctl_paint(void *arg)
{
	ui_rbutton_t *rbutton = (ui_rbutton_t *) arg;

	return ui_rbutton_paint(rbutton);
}

/** Handle radio button control position event.
 *
 * @param arg Argument (ui_rbutton_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_rbutton_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_rbutton_t *rbutton = (ui_rbutton_t *) arg;

	return ui_rbutton_pos_event(rbutton, event);
}

/** @}
 */
