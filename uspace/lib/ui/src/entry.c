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
 * @file Text entry.
 *
 * Currentry text entry is always read-only. It differs from label mostly
 * by its looks.
 */

#include <errno.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/entry.h>
#include <ui/ui.h>
#include "../private/entry.h"
#include "../private/resource.h"

static void ui_entry_ctl_destroy(void *);
static errno_t ui_entry_ctl_paint(void *);
static ui_evclaim_t ui_entry_ctl_pos_event(void *, pos_event_t *);

enum {
	ui_entry_hpad = 4,
	ui_entry_vpad = 4,
	ui_entry_hpad_text = 1,
	ui_entry_vpad_text = 0
};

/** Text entry control ops */
ui_control_ops_t ui_entry_ops = {
	.destroy = ui_entry_ctl_destroy,
	.paint = ui_entry_ctl_paint,
	.pos_event = ui_entry_ctl_pos_event
};

/** Create new text entry.
 *
 * @param resource UI resource
 * @param text Text
 * @param rentry Place to store pointer to new text entry
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_entry_create(ui_resource_t *resource, const char *text,
    ui_entry_t **rentry)
{
	ui_entry_t *entry;
	errno_t rc;

	entry = calloc(1, sizeof(ui_entry_t));
	if (entry == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_entry_ops, (void *) entry, &entry->control);
	if (rc != EOK) {
		free(entry);
		return rc;
	}

	entry->text = str_dup(text);
	if (entry->text == NULL) {
		ui_control_delete(entry->control);
		free(entry);
		return ENOMEM;
	}

	entry->res = resource;
	entry->halign = gfx_halign_left;
	*rentry = entry;
	return EOK;
}

/** Destroy text entry.
 *
 * @param entry Text entry or @c NULL
 */
void ui_entry_destroy(ui_entry_t *entry)
{
	if (entry == NULL)
		return;

	ui_control_delete(entry->control);
	free(entry);
}

/** Get base control from text entry.
 *
 * @param entry Text entry
 * @return Control
 */
ui_control_t *ui_entry_ctl(ui_entry_t *entry)
{
	return entry->control;
}

/** Set text entry rectangle.
 *
 * @param entry Text entry
 * @param rect New text entry rectangle
 */
void ui_entry_set_rect(ui_entry_t *entry, gfx_rect_t *rect)
{
	entry->rect = *rect;
}

/** Set text entry horizontal text alignment.
 *
 * @param entry Text entry
 * @param halign Horizontal alignment
 */
void ui_entry_set_halign(ui_entry_t *entry, gfx_halign_t halign)
{
	entry->halign = halign;
}

/** Set entry text.
 *
 * @param entry Text entry
 * @param text New entry text
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_entry_set_text(ui_entry_t *entry, const char *text)
{
	char *tcopy;

	tcopy = str_dup(text);
	if (tcopy == NULL)
		return ENOMEM;

	free(entry->text);
	entry->text = tcopy;

	return EOK;
}

/** Paint text entry.
 *
 * @param entry Text entry
 * @return EOK on success or an error code
 */
errno_t ui_entry_paint(ui_entry_t *entry)
{
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_coord_t hpad;
	gfx_coord_t vpad;
	gfx_rect_t inside;
	errno_t rc;

	if (entry->res->textmode) {
		hpad = ui_entry_hpad_text;
		vpad = ui_entry_vpad_text;
	} else {
		hpad = ui_entry_hpad;
		vpad = ui_entry_vpad;
	}

	if (entry->res->textmode == false) {
		/* Paint inset frame */
		rc = ui_paint_inset_frame(entry->res, &entry->rect, &inside);
		if (rc != EOK)
			goto error;
	} else {
		inside = entry->rect;
	}

	/* Paint entry background */

	rc = gfx_set_color(entry->res->gc, entry->res->entry_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(entry->res->gc, &inside);
	if (rc != EOK)
		goto error;

	switch (entry->halign) {
	case gfx_halign_left:
	case gfx_halign_justify:
		pos.x = inside.p0.x + hpad;
		break;
	case gfx_halign_center:
		pos.x = (inside.p0.x + inside.p1.x) / 2;
		break;
	case gfx_halign_right:
		pos.x = inside.p1.x - hpad - 1;
		break;
	}

	pos.y = inside.p0.y + vpad;

	gfx_text_fmt_init(&fmt);
	fmt.color = entry->res->entry_fg_color;
	fmt.halign = entry->halign;
	fmt.valign = gfx_valign_top;

	rc = gfx_puttext(entry->res->font, &pos, &fmt, entry->text);
	if (rc != EOK)
		goto error;

	rc = gfx_update(entry->res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Destroy text entry control.
 *
 * @param arg Argument (ui_entry_t *)
 */
void ui_entry_ctl_destroy(void *arg)
{
	ui_entry_t *entry = (ui_entry_t *) arg;

	ui_entry_destroy(entry);
}

/** Paint text entry control.
 *
 * @param arg Argument (ui_entry_t *)
 * @return EOK on success or an error code
 */
errno_t ui_entry_ctl_paint(void *arg)
{
	ui_entry_t *entry = (ui_entry_t *) arg;

	return ui_entry_paint(entry);
}

/** Handle text entry control position event.
 *
 * @param arg Argument (ui_entry_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_entry_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_entry_t *entry = (ui_entry_t *) arg;

	(void) entry;
	return ui_unclaimed;
}

/** @}
 */
