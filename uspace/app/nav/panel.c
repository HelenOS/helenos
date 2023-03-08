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

/** @addtogroup nav
 * @{
 */
/** @file Navigator panel.
 *
 * Displays a file listing.
 */

#include <errno.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <task.h>
#include <ui/control.h>
#include <ui/filelist.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include "panel.h"
#include "nav.h"

static void panel_ctl_destroy(void *);
static errno_t panel_ctl_paint(void *);
static ui_evclaim_t panel_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t panel_ctl_pos_event(void *, pos_event_t *);

/** Panel control ops */
static ui_control_ops_t panel_ctl_ops = {
	.destroy = panel_ctl_destroy,
	.paint = panel_ctl_paint,
	.kbd_event = panel_ctl_kbd_event,
	.pos_event = panel_ctl_pos_event
};

static void panel_flist_activate_req(ui_file_list_t *, void *);
static void panel_flist_selected(ui_file_list_t *, void *, const char *);

/** Panel file list callbacks */
static ui_file_list_cb_t panel_flist_cb = {
	.activate_req = panel_flist_activate_req,
	.selected = panel_flist_selected,
};

/** Create panel.
 *
 * @param window Containing window
 * @param active @c true iff panel should be active
 * @param rpanel Place to store pointer to new panel
 * @return EOK on success or an error code
 */
errno_t panel_create(ui_window_t *window, bool active, panel_t **rpanel)
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

	rc = gfx_color_new_ega(0x0f, &panel->act_border_color);
	if (rc != EOK)
		goto error;

	rc = ui_file_list_create(window, active, &panel->flist);
	if (rc != EOK)
		goto error;

	ui_file_list_set_cb(panel->flist, &panel_flist_cb, (void *)panel);

	panel->window = window;
	panel->active = active;
	*rpanel = panel;
	return EOK;
error:
	if (panel->color != NULL)
		gfx_color_delete(panel->color);
	if (panel->act_border_color != NULL)
		gfx_color_delete(panel->act_border_color);
	if (panel->flist != NULL)
		ui_file_list_destroy(panel->flist);
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
	gfx_color_delete(panel->color);
	gfx_color_delete(panel->act_border_color);
	ui_control_delete(panel->control);
	free(panel);
}

/** Set panel callbacks.
 *
 * @param panel Panel
 * @param cb Callbacks
 * @param arg Argument to callback functions
 */
void panel_set_cb(panel_t *panel, panel_cb_t *cb, void *arg)
{
	panel->cb = cb;
	panel->cb_arg = arg;
}

/** Paint panel.
 *
 * @param panel Panel
 */
errno_t panel_paint(panel_t *panel)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	ui_resource_t *res = ui_window_get_res(panel->window);
	ui_box_style_t bstyle;
	gfx_color_t *bcolor;
	ui_control_t *ctl;
	errno_t rc;

	rc = gfx_set_color(gc, panel->color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &panel->rect);
	if (rc != EOK)
		return rc;

	if (panel->active) {
		bstyle = ui_box_double;
		bcolor = panel->act_border_color;
	} else {
		bstyle = ui_box_single;
		bcolor = panel->color;
	}

	rc = ui_paint_text_box(res, &panel->rect, bstyle, bcolor);
	if (rc != EOK)
		return rc;

	ctl = ui_file_list_ctl(panel->flist);
	rc = ui_control_paint(ctl);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle panel keyboard event.
 *
 * @param panel Panel
 * @param event Keyboard event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t panel_kbd_event(panel_t *panel, kbd_event_t *event)
{
	ui_control_t *ctl;

	if (!panel->active)
		return ui_unclaimed;

	ctl = ui_file_list_ctl(panel->flist);
	return ui_control_kbd_event(ctl, event);
}

/** Handle panel position event.
 *
 * @param panel Panel
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t panel_pos_event(panel_t *panel, pos_event_t *event)
{
	gfx_coord2_t pos;
	ui_control_t *ctl;
	ui_evclaim_t claim;

	pos.x = event->hpos;
	pos.y = event->vpos;
	if (!gfx_pix_inside_rect(&pos, &panel->rect))
		return ui_unclaimed;

	ctl = ui_file_list_ctl(panel->flist);
	claim = ui_control_pos_event(ctl, event);
	if (claim == ui_claimed)
		return ui_claimed;

	if (!panel->active && event->type == POS_PRESS)
		panel_activate_req(panel);

	return ui_claimed;
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
	gfx_rect_t irect;

	panel->rect = *rect;

	irect.p0.x = panel->rect.p0.x + 1;
	irect.p0.y = panel->rect.p0.y + 1;
	irect.p1.x = panel->rect.p1.x;
	irect.p1.y = panel->rect.p1.y - 1;

	ui_file_list_set_rect(panel->flist, &irect);
}

/** Determine if panel is active.
 *
 * @param panel Panel
 * @return @c true iff panel is active
 */
bool panel_is_active(panel_t *panel)
{
	return panel->active;
}

/** Activate panel.
 *
 * @param panel Panel
 *
 * @return EOK on success or an error code
 */
errno_t panel_activate(panel_t *panel)
{
	errno_t rc;

	rc = ui_file_list_activate(panel->flist);
	if (rc != EOK)
		return rc;

	panel->active = true;
	(void) panel_paint(panel);
	return EOK;
}

/** Deactivate panel.
 *
 * @param panel Panel
 */
void panel_deactivate(panel_t *panel)
{
	ui_file_list_deactivate(panel->flist);
	panel->active = false;
	(void) panel_paint(panel);
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

/** Handle panel control keyboard event.
 *
 * @param arg Argument (panel_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t panel_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	panel_t *panel = (panel_t *) arg;

	return panel_kbd_event(panel, event);
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

/** Read directory into panel entry list.
 *
 * @param panel Panel
 * @param dirname Directory path
 * @return EOK on success or an error code
 */
errno_t panel_read_dir(panel_t *panel, const char *dirname)
{
	return ui_file_list_read_dir(panel->flist, dirname);
}

/** Request panel activation.
 *
 * Call back to request panel activation.
 *
 * @param panel Panel
 */
void panel_activate_req(panel_t *panel)
{
	if (panel->cb != NULL && panel->cb->activate_req != NULL)
		panel->cb->activate_req(panel->cb_arg, panel);
}

/** Open panel file entry.
 *
 * Perform Open action on a file entry (i.e. try running it).
 *
 * @param panel Panel
 * @param fname File name
 *
 * @return EOK on success or an error code
 */
static errno_t panel_open_file(panel_t *panel, const char *fname)
{
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	int retval;
	errno_t rc;
	ui_t *ui;

	ui = ui_window_get_ui(panel->window);

	/* Free up and clean console for the child task. */
	rc = ui_suspend(ui);
	if (rc != EOK)
		return rc;

	rc = task_spawnl(&id, &wait, fname, fname, NULL);
	if (rc != EOK)
		goto error;

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL))
		goto error;

	/* Resume UI operation */
	rc = ui_resume(ui);
	if (rc != EOK)
		return rc;

	(void) ui_paint(ui_window_get_ui(panel->window));
	return EOK;
error:
	(void) ui_resume(ui);
	(void) ui_paint(ui_window_get_ui(panel->window));
	return rc;
}

/** File list in panel requests activation.
 *
 * @param flist File list
 * @param arg Argument (panel_t *)
 */
static void panel_flist_activate_req(ui_file_list_t *flist, void *arg)
{
	panel_t *panel = (panel_t *)arg;

	panel_activate_req(panel);
}

/** File in panel file list was selected.
 *
 * @param flist File list
 * @param arg Argument (panel_t *)
 * @param fname File name
 */
static void panel_flist_selected(ui_file_list_t *flist, void *arg,
    const char *fname)
{
	panel_t *panel = (panel_t *)arg;

	(void) panel_open_file(panel, fname);
}

/** @}
 */
