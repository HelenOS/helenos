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
 * @file Push button
 *
 * Push button either uses text as decoration, or it can use a caller-provided
 * function to paint the decoration.
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
#include "../private/pbutton.h"
#include "../private/resource.h"

/** Caption movement when button is pressed down */
enum {
	ui_pb_press_dx = 1,
	ui_pb_press_dy = 1,
	ui_pb_pad_x = 2,
	ui_pb_pad_x_text = 1
};

static void ui_pbutton_ctl_destroy(void *);
static errno_t ui_pbutton_ctl_paint(void *);
static ui_evclaim_t ui_pbutton_ctl_pos_event(void *, pos_event_t *);

/** Push button control ops */
ui_control_ops_t ui_pbutton_ops = {
	.destroy = ui_pbutton_ctl_destroy,
	.paint = ui_pbutton_ctl_paint,
	.pos_event = ui_pbutton_ctl_pos_event
};

/** Create new push button.
 *
 * @param resource UI resource
 * @param caption Caption
 * @param rpbutton Place to store pointer to new push button
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_pbutton_create(ui_resource_t *resource, const char *caption,
    ui_pbutton_t **rpbutton)
{
	ui_pbutton_t *pbutton;
	errno_t rc;

	pbutton = calloc(1, sizeof(ui_pbutton_t));
	if (pbutton == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_pbutton_ops, (void *) pbutton,
	    &pbutton->control);
	if (rc != EOK) {
		free(pbutton);
		return rc;
	}

	pbutton->caption = str_dup(caption);
	if (pbutton->caption == NULL) {
		ui_control_delete(pbutton->control);
		free(pbutton);
		return ENOMEM;
	}

	pbutton->res = resource;
	*rpbutton = pbutton;
	return EOK;
}

/** Destroy push button.
 *
 * @param pbutton Push button or @c NULL
 */
void ui_pbutton_destroy(ui_pbutton_t *pbutton)
{
	if (pbutton == NULL)
		return;

	ui_control_delete(pbutton->control);
	free(pbutton->caption);
	free(pbutton);
}

/** Get base control from push button.
 *
 * @param pbutton Push button
 * @return Control
 */
ui_control_t *ui_pbutton_ctl(ui_pbutton_t *pbutton)
{
	return pbutton->control;
}

/** Set push button callbacks.
 *
 * @param pbutton Push button
 * @param cb Push button callbacks
 * @param arg Callback argument
 */
void ui_pbutton_set_cb(ui_pbutton_t *pbutton, ui_pbutton_cb_t *cb, void *arg)
{
	pbutton->cb = cb;
	pbutton->arg = arg;
}

/** Set push button decoration ops.
 *
 * @param pbutton Push button
 * @param ops Push button decoration callbacks
 * @param arg Decoration ops argument
 */
void ui_pbutton_set_decor_ops(ui_pbutton_t *pbutton,
    ui_pbutton_decor_ops_t *ops, void *arg)
{
	pbutton->decor_ops = ops;
	pbutton->decor_arg = arg;
}

/** Set push button flag.s
 *
 * @param pbutton Push button
 * @param flags Flags
 */
void ui_pbutton_set_flags(ui_pbutton_t *pbutton, ui_pbutton_flags_t flags)
{
	pbutton->flags = flags;
}

/** Set button rectangle.
 *
 * @param pbutton Button
 * @param rect New button rectangle
 */
void ui_pbutton_set_rect(ui_pbutton_t *pbutton, gfx_rect_t *rect)
{
	pbutton->rect = *rect;
}

/** Set default flag.
 *
 * Default button is the one activated by Enter key, it is marked
 * by thicker frame.
 *
 * @param pbutton Button
 * @param isdefault @c true iff button is default
 */
void ui_pbutton_set_default(ui_pbutton_t *pbutton, bool isdefault)
{
	pbutton->isdefault = isdefault;
}

/** Get button light status.
 *
 * @param pbutton Button
 * @return @c true iff light is on
 */
bool ui_pbutton_get_light(ui_pbutton_t *pbutton)
{
	return pbutton->light;
}

/** Turn button light on or off.
 *
 * @param pbutton Button
 * @param light @c true iff button should be lit
 */
void ui_pbutton_set_light(ui_pbutton_t *pbutton, bool light)
{
	pbutton->light = light;
}

/** Set push button caption.
 *
 * @param pbutton Button
 * @param caption New caption
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_pbutton_set_caption(ui_pbutton_t *pbutton, const char *caption)
{
	char *dcaption;

	dcaption = str_dup(caption);
	if (dcaption == NULL)
		return ENOMEM;

	free(pbutton->caption);
	pbutton->caption = dcaption;
	return EOK;
}

/** Paint outer button frame.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_frame(ui_pbutton_t *pbutton)
{
	gfx_rect_t rect;
	gfx_coord_t thickness;
	errno_t rc;

	thickness = pbutton->isdefault ? 2 : 1;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->btn_frame_color);
	if (rc != EOK)
		goto error;

	rect.p0.x = pbutton->rect.p0.x + 1;
	rect.p0.y = pbutton->rect.p0.y;
	rect.p1.x = pbutton->rect.p1.x - 1;
	rect.p1.y = pbutton->rect.p0.y + thickness;
	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	rect.p0.x = pbutton->rect.p0.x + 1;
	rect.p0.y = pbutton->rect.p1.y - thickness;
	rect.p1.x = pbutton->rect.p1.x - 1;
	rect.p1.y = pbutton->rect.p1.y;
	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	rect.p0.x = pbutton->rect.p0.x;
	rect.p0.y = pbutton->rect.p0.y + 1;
	rect.p1.x = pbutton->rect.p0.x + thickness;
	rect.p1.y = pbutton->rect.p1.y - 1;
	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	rect.p0.x = pbutton->rect.p1.x - thickness;
	rect.p0.y = pbutton->rect.p0.y + 1;
	rect.p1.x = pbutton->rect.p1.x;
	rect.p1.y = pbutton->rect.p1.y - 1;
	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint outset button bevel.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_outset(ui_pbutton_t *pbutton,
    gfx_rect_t *rect)
{
	return ui_paint_bevel(pbutton->res->gc, rect,
	    pbutton->res->btn_highlight_color,
	    pbutton->res->btn_shadow_color, 2, NULL);
}

/** Paint inset button bevel.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_inset(ui_pbutton_t *pbutton,
    gfx_rect_t *rect)
{
	return ui_paint_bevel(pbutton->res->gc, rect,
	    pbutton->res->btn_shadow_color,
	    pbutton->res->btn_face_color, 2, NULL);
}

/** Paint button text shadow.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_text_shadow(ui_pbutton_t *pbutton)
{
	gfx_rect_t rect;
	errno_t rc;

	rect.p0.x = pbutton->rect.p0.x + 1;
	rect.p0.y = pbutton->rect.p0.y + 1;
	rect.p1.x = pbutton->rect.p1.x;
	rect.p1.y = pbutton->rect.p1.y;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->btn_shadow_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint push button in graphic mode.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_gfx(ui_pbutton_t *pbutton)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_rect_t rect;
	gfx_rect_t irect;
	gfx_coord_t thickness;
	gfx_color_t *color;
	bool depressed;
	errno_t rc;

	thickness = pbutton->isdefault ? 2 : 1;
	depressed = pbutton->held && pbutton->inside;

	rect.p0.x = pbutton->rect.p0.x + thickness;
	rect.p0.y = pbutton->rect.p0.y + thickness;
	rect.p1.x = pbutton->rect.p1.x - thickness;
	rect.p1.y = pbutton->rect.p1.y - thickness;

	color = pbutton->light ? pbutton->res->btn_face_lit_color :
	    pbutton->res->btn_face_color;

	rc = gfx_set_color(pbutton->res->gc, color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	/* Center of button rectangle */
	pos.x = (rect.p0.x + rect.p1.x) / 2;
	pos.y = (rect.p0.y + rect.p1.y) / 2;

	if (depressed) {
		pos.x += ui_pb_press_dx;
		pos.y += ui_pb_press_dy;
	}

	if (pbutton->decor_ops != NULL && pbutton->decor_ops->paint != NULL) {
		/* Custom decoration */
		rc = pbutton->decor_ops->paint(pbutton, pbutton->decor_arg,
		    &pos);
		if (rc != EOK)
			goto error;
	} else {
		/* Text decoration */
		ui_paint_get_inset_frame_inside(pbutton->res, &rect, &irect);
		gfx_text_fmt_init(&fmt);
		fmt.font = pbutton->res->font;
		fmt.color = pbutton->res->btn_text_color;
		fmt.halign = gfx_halign_center;
		fmt.valign = gfx_valign_center;
		fmt.abbreviate = true;
		fmt.width = irect.p1.x - irect.p0.x - 2 * ui_pb_pad_x;

		rc = gfx_puttext(&pos, &fmt, pbutton->caption);
		if (rc != EOK)
			goto error;
	}

	rc = ui_pbutton_paint_frame(pbutton);
	if (rc != EOK)
		goto error;

	if (depressed) {
		rc = ui_pbutton_paint_inset(pbutton, &rect);
		if (rc != EOK)
			goto error;
	} else {
		rc = ui_pbutton_paint_outset(pbutton, &rect);
		if (rc != EOK)
			goto error;
	}

	rc = gfx_update(pbutton->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint push button in text mode.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
static errno_t ui_pbutton_paint_text(ui_pbutton_t *pbutton)
{
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	gfx_rect_t rect;
	bool depressed;
	errno_t rc;

	if ((pbutton->flags & ui_pbf_no_text_depress) == 0)
		depressed = pbutton->held && pbutton->inside;
	else
		depressed = false;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(pbutton->res->gc, &pbutton->rect);
	if (rc != EOK)
		goto error;

	rect.p0.x = pbutton->rect.p0.x + (depressed ? 1 : 0);
	rect.p0.y = pbutton->rect.p0.y;
	rect.p1.x = pbutton->rect.p1.x - 1 + (depressed ? 1 : 0);
	rect.p1.y = pbutton->rect.p0.y + 1;

	rc = gfx_set_color(pbutton->res->gc, pbutton->res->btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(pbutton->res->gc, &rect);
	if (rc != EOK)
		goto error;

	/* Center of button rectangle */
	pos.x = (rect.p0.x + rect.p1.x) / 2;
	pos.y = (rect.p0.y + rect.p1.y) / 2;

	gfx_text_fmt_init(&fmt);
	fmt.font = pbutton->res->font;
	fmt.color = pbutton->res->btn_text_color;
	fmt.halign = gfx_halign_center;
	fmt.valign = gfx_valign_center;
	fmt.abbreviate = true;
	fmt.width = rect.p1.x - rect.p0.x - 2 * ui_pb_pad_x_text;
	if (fmt.width < 1)
		fmt.width = 1;

	rc = gfx_puttext(&pos, &fmt, pbutton->caption);
	if (rc != EOK)
		goto error;

	if (!depressed) {
		rc = ui_pbutton_paint_text_shadow(pbutton);
		if (rc != EOK)
			goto error;
	}

	rc = gfx_update(pbutton->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint push button.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
errno_t ui_pbutton_paint(ui_pbutton_t *pbutton)
{
	if (pbutton->res->textmode)
		return ui_pbutton_paint_text(pbutton);
	else
		return ui_pbutton_paint_gfx(pbutton);
}

/** Press down button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_press(ui_pbutton_t *pbutton)
{
	if (pbutton->held)
		return;

	pbutton->inside = true;
	pbutton->held = true;
	(void) ui_pbutton_paint(pbutton);
	ui_pbutton_down(pbutton);
}

/** Release button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_release(ui_pbutton_t *pbutton)
{
	if (!pbutton->held)
		return;

	pbutton->held = false;
	ui_pbutton_up(pbutton);

	if (pbutton->inside) {
		(void) ui_pbutton_paint(pbutton);
		ui_pbutton_clicked(pbutton);
	}
}

/** Pointer entered button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_enter(ui_pbutton_t *pbutton)
{
	if (pbutton->inside)
		return;

	pbutton->inside = true;
	if (pbutton->held)
		(void) ui_pbutton_paint(pbutton);
}

/** Pointer left button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_leave(ui_pbutton_t *pbutton)
{
	if (!pbutton->inside)
		return;

	pbutton->inside = false;
	if (pbutton->held)
		(void) ui_pbutton_paint(pbutton);
}

/** Send button clicked event.
 *
 * @param pbutton Push button
 */
void ui_pbutton_clicked(ui_pbutton_t *pbutton)
{
	if (pbutton->cb != NULL && pbutton->cb->clicked != NULL)
		pbutton->cb->clicked(pbutton, pbutton->arg);
}

/** Send button down event.
 *
 * @param pbutton Push button
 */
void ui_pbutton_down(ui_pbutton_t *pbutton)
{
	if (pbutton->cb != NULL && pbutton->cb->down != NULL)
		pbutton->cb->down(pbutton, pbutton->arg);
}

/** Send button up event.
 *
 * @param pbutton Push button
 */
void ui_pbutton_up(ui_pbutton_t *pbutton)
{
	if (pbutton->cb != NULL && pbutton->cb->up != NULL)
		pbutton->cb->up(pbutton, pbutton->arg);
}

/** Handle push button position event.
 *
 * @param pbutton Push button
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_pbutton_pos_event(ui_pbutton_t *pbutton, pos_event_t *event)
{
	gfx_coord2_t pos;
	bool inside;

	pos.x = event->hpos;
	pos.y = event->vpos;

	inside = gfx_pix_inside_rect(&pos, &pbutton->rect);

	switch (event->type) {
	case POS_PRESS:
		if (inside) {
			ui_pbutton_press(pbutton);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (pbutton->held) {
			ui_pbutton_release(pbutton);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		if (inside && !pbutton->inside) {
			ui_pbutton_enter(pbutton);
			return ui_claimed;
		} else if (!inside && pbutton->inside) {
			ui_pbutton_leave(pbutton);
		}
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Destroy push button control.
 *
 * @param arg Argument (ui_pbutton_t *)
 */
void ui_pbutton_ctl_destroy(void *arg)
{
	ui_pbutton_t *pbutton = (ui_pbutton_t *) arg;

	ui_pbutton_destroy(pbutton);
}

/** Paint push button control.
 *
 * @param arg Argument (ui_pbutton_t *)
 * @return EOK on success or an error code
 */
errno_t ui_pbutton_ctl_paint(void *arg)
{
	ui_pbutton_t *pbutton = (ui_pbutton_t *) arg;

	return ui_pbutton_paint(pbutton);
}

/** Handle push button control position event.
 *
 * @param arg Argument (ui_pbutton_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_pbutton_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_pbutton_t *pbutton = (ui_pbutton_t *) arg;

	return ui_pbutton_pos_event(pbutton, event);
}

/** @}
 */
