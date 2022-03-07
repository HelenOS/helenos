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
 * @file Label
 */

#include <errno.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/label.h>
#include "../private/label.h"
#include "../private/resource.h"

static void ui_label_ctl_destroy(void *);
static errno_t ui_label_ctl_paint(void *);
static ui_evclaim_t ui_label_ctl_pos_event(void *, pos_event_t *);

/** Label control ops */
ui_control_ops_t ui_label_ops = {
	.destroy = ui_label_ctl_destroy,
	.paint = ui_label_ctl_paint,
	.pos_event = ui_label_ctl_pos_event
};

/** Create new label.
 *
 * @param resource UI resource
 * @param text Text
 * @param rlabel Place to store pointer to new label
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_label_create(ui_resource_t *resource, const char *text,
    ui_label_t **rlabel)
{
	ui_label_t *label;
	errno_t rc;

	label = calloc(1, sizeof(ui_label_t));
	if (label == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_label_ops, (void *) label, &label->control);
	if (rc != EOK) {
		free(label);
		return rc;
	}

	label->text = str_dup(text);
	if (label->text == NULL) {
		ui_control_delete(label->control);
		free(label);
		return ENOMEM;
	}

	label->res = resource;
	label->halign = gfx_halign_left;
	*rlabel = label;
	return EOK;
}

/** Destroy label.
 *
 * @param label Label or @c NULL
 */
void ui_label_destroy(ui_label_t *label)
{
	if (label == NULL)
		return;

	ui_control_delete(label->control);
	free(label);
}

/** Get base control from label.
 *
 * @param label Label
 * @return Control
 */
ui_control_t *ui_label_ctl(ui_label_t *label)
{
	return label->control;
}

/** Set label rectangle.
 *
 * @param label Label
 * @param rect New label rectangle
 */
void ui_label_set_rect(ui_label_t *label, gfx_rect_t *rect)
{
	label->rect = *rect;
}

/** Set label horizontal text alignment.
 *
 * @param label Label
 * @param halign Horizontal alignment
 */
void ui_label_set_halign(ui_label_t *label, gfx_halign_t halign)
{
	label->halign = halign;
}

/** Set label vertical text alignment.
 *
 * @param label Label
 * @param valign Vertical alignment
 */
void ui_label_set_valign(ui_label_t *label, gfx_valign_t valign)
{
	label->valign = valign;
}

/** Set label text.
 *
 * @param label Label
 * @param text New label text
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_label_set_text(ui_label_t *label, const char *text)
{
	char *tcopy;

	tcopy = str_dup(text);
	if (tcopy == NULL)
		return ENOMEM;

	free(label->text);
	label->text = tcopy;

	return EOK;
}

/** Paint label.
 *
 * @param label Label
 * @return EOK on success or an error code
 */
errno_t ui_label_paint(ui_label_t *label)
{
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	errno_t rc;

	/* Paint label background */

	rc = gfx_set_color(label->res->gc, label->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(label->res->gc, &label->rect);
	if (rc != EOK)
		goto error;

	switch (label->halign) {
	case gfx_halign_left:
	case gfx_halign_justify:
		pos.x = label->rect.p0.x;
		break;
	case gfx_halign_center:
		pos.x = (label->rect.p0.x + label->rect.p1.x) / 2;
		break;
	case gfx_halign_right:
		pos.x = label->rect.p1.x;
		break;
	}

	switch (label->valign) {
	case gfx_valign_top:
		pos.y = label->rect.p0.y;
		break;
	case gfx_valign_center:
		pos.y = (label->rect.p0.y + label->rect.p1.y) / 2;
		break;
	case gfx_valign_bottom:
		pos.y = label->rect.p1.y;
		break;
	case gfx_valign_baseline:
		return EINVAL;
	}

	gfx_text_fmt_init(&fmt);
	fmt.font = label->res->font;
	fmt.color = label->res->wnd_text_color;
	fmt.halign = label->halign;
	fmt.valign = label->valign;

	rc = gfx_puttext(&pos, &fmt, label->text);
	if (rc != EOK)
		goto error;

	rc = gfx_update(label->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Destroy label control.
 *
 * @param arg Argument (ui_label_t *)
 */
void ui_label_ctl_destroy(void *arg)
{
	ui_label_t *label = (ui_label_t *) arg;

	ui_label_destroy(label);
}

/** Paint label control.
 *
 * @param arg Argument (ui_label_t *)
 * @return EOK on success or an error code
 */
errno_t ui_label_ctl_paint(void *arg)
{
	ui_label_t *label = (ui_label_t *) arg;

	return ui_label_paint(label);
}

/** Handle label control position event.
 *
 * @param arg Argument (ui_label_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_label_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_label_t *label = (ui_label_t *) arg;

	(void) label;
	return ui_unclaimed;
}

/** @}
 */
