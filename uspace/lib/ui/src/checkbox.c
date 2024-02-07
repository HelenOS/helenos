/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * @file Check box
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
#include <ui/checkbox.h>
#include "../private/checkbox.h"
#include "../private/resource.h"

enum {
	checkbox_box_w = 16,
	checkbox_box_h = 16,
	checkbox_label_margin = 8,
	checkbox_cross_n = 5,
	checkbox_cross_w = 2,
	checkbox_cross_h = 2
};

static void ui_checkbox_ctl_destroy(void *);
static errno_t ui_checkbox_ctl_paint(void *);
static ui_evclaim_t ui_checkbox_ctl_pos_event(void *, pos_event_t *);

/** Check box control ops */
ui_control_ops_t ui_checkbox_ops = {
	.destroy = ui_checkbox_ctl_destroy,
	.paint = ui_checkbox_ctl_paint,
	.pos_event = ui_checkbox_ctl_pos_event
};

/** Create new check box.
 *
 * @param resource UI resource
 * @param caption Caption
 * @param rcheckbox Place to store pointer to new check box
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_checkbox_create(ui_resource_t *resource, const char *caption,
    ui_checkbox_t **rcheckbox)
{
	ui_checkbox_t *checkbox;
	errno_t rc;

	checkbox = calloc(1, sizeof(ui_checkbox_t));
	if (checkbox == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_checkbox_ops, (void *) checkbox,
	    &checkbox->control);
	if (rc != EOK) {
		free(checkbox);
		return rc;
	}

	checkbox->caption = str_dup(caption);
	if (checkbox->caption == NULL) {
		ui_control_delete(checkbox->control);
		free(checkbox);
		return ENOMEM;
	}

	checkbox->res = resource;
	*rcheckbox = checkbox;
	return EOK;
}

/** Destroy check box.
 *
 * @param checkbox Check box or @c NULL
 */
void ui_checkbox_destroy(ui_checkbox_t *checkbox)
{
	if (checkbox == NULL)
		return;

	ui_control_delete(checkbox->control);
	free(checkbox);
}

/** Get base control from check box.
 *
 * @param checkbox Check box
 * @return Control
 */
ui_control_t *ui_checkbox_ctl(ui_checkbox_t *checkbox)
{
	return checkbox->control;
}

/** Set check box callbacks.
 *
 * @param checkbox Check box
 * @param cb Check box callbacks
 * @param arg Callback argument
 */
void ui_checkbox_set_cb(ui_checkbox_t *checkbox, ui_checkbox_cb_t *cb, void *arg)
{
	checkbox->cb = cb;
	checkbox->arg = arg;
}

/** Set check box rectangle.
 *
 * @param checkbox Check box
 * @param rect New check box rectangle
 */
void ui_checkbox_set_rect(ui_checkbox_t *checkbox, gfx_rect_t *rect)
{
	checkbox->rect = *rect;
}

/** Return if check box is checked.
 *
 * @param checkbox Check box
 * @return @c true iff check box is checked
 */
bool ui_checkbox_get_checked(ui_checkbox_t *checkbox)
{
	return checkbox->checked;
}

/** Set check box checked state.
 *
 * @param checkbox Check box
 * @param checked @c true iff checkbox should be checked
 */
void ui_checkbox_set_checked(ui_checkbox_t *checkbox, bool checked)
{
	checkbox->checked = checked;
}

/** Paint check box in graphics mode.
 *
 * @param checkbox Check box
 * @return EOK on success or an error code
 */
errno_t ui_checkbox_paint_gfx(ui_checkbox_t *checkbox)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_rect_t box_rect;
	gfx_rect_t box_inside;
	gfx_coord2_t box_center;
	bool depressed;
	errno_t rc;

	box_rect.p0 = checkbox->rect.p0;
	box_rect.p1.x = box_rect.p0.x + checkbox_box_w;
	box_rect.p1.y = box_rect.p0.y + checkbox_box_h;

	/* Paint checkbox frame */

	rc = ui_paint_inset_frame(checkbox->res, &box_rect, &box_inside);
	if (rc != EOK)
		goto error;

	/* Paint checkbox interior */

	depressed = checkbox->held && checkbox->inside;

	rc = gfx_set_color(checkbox->res->gc, depressed ?
	    checkbox->res->entry_act_bg_color :
	    checkbox->res->entry_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(checkbox->res->gc, &box_inside);
	if (rc != EOK)
		goto error;

	/* Paint cross mark */

	if (checkbox->checked) {
		rc = gfx_set_color(checkbox->res->gc,
		    checkbox->res->entry_fg_color);
		if (rc != EOK)
			goto error;

		box_center.x = (box_inside.p0.x + box_inside.p1.x) / 2 - 1;
		box_center.y = (box_inside.p0.y + box_inside.p1.y) / 2 - 1;

		rc = ui_paint_cross(checkbox->res->gc, &box_center,
		    checkbox_cross_n, checkbox_cross_w, checkbox_cross_h);
		if (rc != EOK)
			goto error;
	}

	/* Paint checkbox label */

	pos.x = box_rect.p1.x + checkbox_label_margin;
	pos.y = (box_rect.p0.y + box_rect.p1.y) / 2;

	gfx_text_fmt_init(&fmt);
	fmt.font = checkbox->res->font;
	fmt.color = checkbox->res->wnd_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_center;

	rc = gfx_puttext(&pos, &fmt, checkbox->caption);
	if (rc != EOK)
		goto error;

	rc = gfx_update(checkbox->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint check box in text mode.
 *
 * @param checkbox Check box
 * @return EOK on success or an error code
 */
errno_t ui_checkbox_paint_text(ui_checkbox_t *checkbox)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	bool depressed;
	errno_t rc;

	/* Paint checkbox */

	depressed = checkbox->held && checkbox->inside;

	pos.x = checkbox->rect.p0.x;
	pos.y = checkbox->rect.p0.y;

	gfx_text_fmt_init(&fmt);
	fmt.font = checkbox->res->font;
	fmt.color = depressed ? checkbox->res->entry_act_bg_color :
	    checkbox->res->wnd_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	rc = gfx_puttext(&pos, &fmt, checkbox->checked ? "[X]" : "[ ]");
	if (rc != EOK)
		goto error;

	/* Paint checkbox label */

	pos.x += 4;
	fmt.color = checkbox->res->wnd_text_color;

	rc = gfx_puttext(&pos, &fmt, checkbox->caption);
	if (rc != EOK)
		goto error;

	rc = gfx_update(checkbox->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint check box.
 *
 * @param checkbox Check box
 * @return EOK on success or an error code
 */
errno_t ui_checkbox_paint(ui_checkbox_t *checkbox)
{
	if (checkbox->res->textmode)
		return ui_checkbox_paint_text(checkbox);
	else
		return ui_checkbox_paint_gfx(checkbox);
}

/** Press down button.
 *
 * @param checkbox Check box
 */
void ui_checkbox_press(ui_checkbox_t *checkbox)
{
	if (checkbox->held)
		return;

	checkbox->inside = true;
	checkbox->held = true;
	(void) ui_checkbox_paint(checkbox);
}

/** Release button.
 *
 * @param checkbox Check box
 */
void ui_checkbox_release(ui_checkbox_t *checkbox)
{
	if (!checkbox->held)
		return;

	checkbox->held = false;

	if (checkbox->inside) {
		/* Toggle check box state */
		checkbox->checked = !checkbox->checked;

		/* Repaint and notify */
		(void) ui_checkbox_paint(checkbox);
		ui_checkbox_switched(checkbox);
	}
}

/** Pointer entered button.
 *
 * @param checkbox Check box
 */
void ui_checkbox_enter(ui_checkbox_t *checkbox)
{
	if (checkbox->inside)
		return;

	checkbox->inside = true;
	if (checkbox->held)
		(void) ui_checkbox_paint(checkbox);
}

/** Pointer left button.
 *
 * @param checkbox Check box
 */
void ui_checkbox_leave(ui_checkbox_t *checkbox)
{
	if (!checkbox->inside)
		return;

	checkbox->inside = false;
	if (checkbox->held)
		(void) ui_checkbox_paint(checkbox);
}

/** Button was switched.
 *
 * @param checkbox Check box
 */
void ui_checkbox_switched(ui_checkbox_t *checkbox)
{
	if (checkbox->cb != NULL && checkbox->cb->switched != NULL) {
		checkbox->cb->switched(checkbox, checkbox->arg,
		    checkbox->checked);
	}
}

/** Handle check box position event.
 *
 * @param checkbox Check box
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_checkbox_pos_event(ui_checkbox_t *checkbox, pos_event_t *event)
{
	gfx_coord2_t pos;
	bool inside;

	pos.x = event->hpos;
	pos.y = event->vpos;

	inside = gfx_pix_inside_rect(&pos, &checkbox->rect);

	switch (event->type) {
	case POS_PRESS:
		if (inside) {
			ui_checkbox_press(checkbox);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (checkbox->held) {
			ui_checkbox_release(checkbox);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		if (inside && !checkbox->inside) {
			ui_checkbox_enter(checkbox);
			return ui_claimed;
		} else if (!inside && checkbox->inside) {
			ui_checkbox_leave(checkbox);
		}
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Destroy check box control.
 *
 * @param arg Argument (ui_checkbox_t *)
 */
void ui_checkbox_ctl_destroy(void *arg)
{
	ui_checkbox_t *checkbox = (ui_checkbox_t *) arg;

	ui_checkbox_destroy(checkbox);
}

/** Paint check box control.
 *
 * @param arg Argument (ui_checkbox_t *)
 * @return EOK on success or an error code
 */
errno_t ui_checkbox_ctl_paint(void *arg)
{
	ui_checkbox_t *checkbox = (ui_checkbox_t *) arg;

	return ui_checkbox_paint(checkbox);
}

/** Handle check box control position event.
 *
 * @param arg Argument (ui_checkbox_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_checkbox_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_checkbox_t *checkbox = (ui_checkbox_t *) arg;

	return ui_checkbox_pos_event(checkbox, event);
}

/** @}
 */
