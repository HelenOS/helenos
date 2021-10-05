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

/** @addtogroup nav
 * @{
 */
/** @file Navigator panel.
 *
 * Displays a file listing.
 */

#include <errno.h>
#include <gfx/render.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include "panel.h"
#include "nav.h"

static void panel_ctl_destroy(void *);
static errno_t panel_ctl_paint(void *);
static ui_evclaim_t panel_ctl_pos_event(void *, pos_event_t *);

/** Panel control ops */
ui_control_ops_t panel_ctl_ops = {
	.destroy = panel_ctl_destroy,
	.paint = panel_ctl_paint,
	.pos_event = panel_ctl_pos_event
};

/** Create panel.
 *
 * @param window Containing window
 * @param rpanel Place to store pointer to new panel
 * @return EOK on success or an error code
 */
errno_t panel_create(ui_window_t *window, panel_t **rpanel)
{
	panel_t *panel;
	errno_t rc;

	panel = calloc(1, sizeof(panel_t));
	if (panel == NULL)
		return ENOMEM;

	rc = ui_control_new(&panel_ctl_ops, (void *)panel,
	    &panel->control);
	if (rc != EOK) {
		free(panel);
		return rc;
	}

	rc = gfx_color_new_ega(0x07, &panel->color);
	if (rc != EOK)
		goto error;

	panel->window = window;
	list_initialize(&panel->entries);
	*rpanel = panel;
	return EOK;
error:
	ui_control_delete(panel->control);
	free(panel);
	return rc;
}

/** Destroy panel.
 *
 * @param panel Panel
 */
void panel_destroy(panel_t *panel)
{
	panel_entry_t *entry;

	entry = panel_first(panel);
	while (entry != NULL) {
		panel_entry_delete(entry);
		entry = panel_first(panel);
	}

	ui_control_delete(panel->control);
	free(panel);
}

/** Paint panel.
 *
 * @param panel Panel
 */
errno_t panel_paint(panel_t *panel)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	ui_resource_t *res = ui_window_get_res(panel->window);
	errno_t rc;

	rc = gfx_set_color(gc, panel->color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &panel->rect);
	if (rc != EOK)
		return rc;

	rc = ui_paint_text_box(res, &panel->rect, ui_box_single,
	    panel->color);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle panel position event.
 *
 * @param panel Panel
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t panel_pos_event(panel_t *panel, pos_event_t *event)
{
	return ui_unclaimed;
}

/** Get base control for panel.
 *
 * @param panel Panel
 * @return Base UI control
 */
ui_control_t *panel_ctl(panel_t *panel)
{
	return panel->control;
}

/** Set panel rectangle.
 *
 * @param panel Panel
 * @param rect Rectangle
 */
void panel_set_rect(panel_t *panel, gfx_rect_t *rect)
{
	panel->rect = *rect;
}

/** Destroy panel control.
 *
 * @param arg Argument (panel_t *)
 */
void panel_ctl_destroy(void *arg)
{
	panel_t *panel = (panel_t *) arg;

	panel_destroy(panel);
}

/** Paint panel control.
 *
 * @param arg Argument (panel_t *)
 * @return EOK on success or an error code
 */
errno_t panel_ctl_paint(void *arg)
{
	panel_t *panel = (panel_t *) arg;

	return panel_paint(panel);
}

/** Handle panel control position event.
 *
 * @param arg Argument (panel_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t panel_ctl_pos_event(void *arg, pos_event_t *event)
{
	panel_t *panel = (panel_t *) arg;

	return panel_pos_event(panel, event);
}

/** Append new panel entry.
 *
 * @param panel Panel
 * @param name File name
 * @param size File size;
 * @return EOK on success or an error code
 */
errno_t panel_entry_append(panel_t *panel, const char *name, uint64_t size)
{
	panel_entry_t *entry;

	entry = calloc(1, sizeof(panel_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->panel = panel;
	entry->name = str_dup(name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->size = size;
	link_initialize(&entry->lentries);
	list_append(&entry->lentries, &panel->entries);
	return EOK;
}

/** Delete panel entry.
 *
 * @param entry Panel entry
 */
void panel_entry_delete(panel_entry_t *entry)
{
	list_remove(&entry->lentries);
	free(entry->name);
	free(entry);
}

/** Return first panel entry.
 *
 * @panel Panel
 * @return First panel entry or @c NULL if there are no entries
 */
panel_entry_t *panel_first(panel_t *panel)
{
	link_t *link;

	link = list_first(&panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** Return next panel entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
panel_entry_t *panel_next(panel_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** @}
 */
