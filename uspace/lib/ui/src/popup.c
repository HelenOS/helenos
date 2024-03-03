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
 * @file Popup window
 */

#include <errno.h>
#include <gfx/context.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/control.h>
#include <ui/popup.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/popup.h"

static void ui_popup_window_close(ui_window_t *, void *);
static void ui_popup_window_kbd(ui_window_t *, void *, kbd_event_t *);
static void ui_popup_window_pos(ui_window_t *, void *, pos_event_t *);

static ui_window_cb_t ui_popup_window_cb = {
	.close = ui_popup_window_close,
	.kbd = ui_popup_window_kbd,
	.pos = ui_popup_window_pos
};

/** Initialize popup parameters structure.
 *
 * Popup parameters structure must always be initialized using this function
 * first.
 *
 * @param params Popup parameters structure
 */
void ui_popup_params_init(ui_popup_params_t *params)
{
	memset(params, 0, sizeof(ui_popup_params_t));
}

/** Create new popup window.
 *
 * @param ui User interface
 * @param parent Parent window
 * @param params Popup parameters
 * @param rpopup Place to store pointer to new popup window
 * @return EOK on success or an error code
 */
errno_t ui_popup_create(ui_t *ui, ui_window_t *parent,
    ui_popup_params_t *params, ui_popup_t **rpopup)
{
	ui_popup_t *popup;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	gfx_coord2_t parent_pos;
	errno_t rc;

	popup = calloc(1, sizeof(ui_popup_t));
	if (popup == NULL)
		return ENOMEM;

	rc = ui_window_get_pos(parent, &parent_pos);
	if (rc != EOK)
		goto error;

	ui_wnd_params_init(&wparams);
	wparams.rect = params->rect;
	wparams.caption = "";
	wparams.style &= ~ui_wds_decorated;
	wparams.placement = ui_wnd_place_popup;
	wparams.flags |= ui_wndf_popup | ui_wndf_topmost;
	wparams.idev_id = params->idev_id;

	/* Compute position of parent rectangle relative to the screen */
	gfx_rect_translate(&parent_pos, &params->place, &wparams.prect);

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	popup->ui = ui;
	popup->parent = parent;
	popup->window = window;

	ui_window_set_cb(window, &ui_popup_window_cb, popup);

	*rpopup = popup;
	return EOK;
error:
	free(popup);
	return rc;
}

/** Destroy popup window.
 *
 * @param popup Popup window or @c NULL
 */
void ui_popup_destroy(ui_popup_t *popup)
{
	if (popup == NULL)
		return;

	ui_window_destroy(popup->window);
	free(popup);
}

/** Add control to popup window.
 *
 * Only one control can be added to a popup window. If more than one control
 * is added, the results are undefined.
 *
 * @param popup Popup window
 * @param control Control
 * @return EOK on success, ENOMEM if out of memory
 */
void ui_popup_add(ui_popup_t *popup, ui_control_t *control)
{
	ui_window_add(popup->window, control);
}

/** Remove control from popup window.
 *
 * @param popup Popup window
 * @param control Control
 */
void ui_popup_remove(ui_popup_t *popup, ui_control_t *control)
{
	ui_window_remove(popup->window, control);
}

/** Set popup window callbacks.
 *
 * @param popup Popup window
 * @param cb Popup window callbacks
 * @param arg Callback argument
 */
void ui_popup_set_cb(ui_popup_t *popup, ui_popup_cb_t *cb, void *arg)
{
	popup->cb = cb;
	popup->arg = arg;
}

/** Get UI resource from popup window.
 *
 * @param window Window
 * @return UI resource
 */
ui_resource_t *ui_popup_get_res(ui_popup_t *popup)
{
	return ui_window_get_res(popup->window);
}

/** Get popup window GC.
 *
 * @param popup Popup window
 * @return GC (relative to popup window)
 */
gfx_context_t *ui_popup_get_gc(ui_popup_t *popup)
{
	return ui_window_get_gc(popup->window);
}

/** Get ID of device that sent the last position event.
 *
 * @param popup Popup window
 * @return Input device ID
 */
sysarg_t ui_popup_get_idev_id(ui_popup_t *popup)
{
	return popup->idev_id;
}

/** Handle close event in popup window.
 *
 * @param window Window
 * @param arg Argument (ui_popup_t *)
 */
static void ui_popup_window_close(ui_window_t *window, void *arg)
{
	ui_popup_t *popup = (ui_popup_t *)arg;

	if (popup->cb != NULL && popup->cb->close != NULL)
		popup->cb->close(popup, popup->arg);
}

/** Handle keyboard event in popup window.
 *
 * @param window Window
 * @param arg Argument (ui_popup_t *)
 * @param event Keyboard event
 */
static void ui_popup_window_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	ui_popup_t *popup = (ui_popup_t *)arg;

	/* Remember ID of device that sent the last event */
	popup->idev_id = event->kbd_id;

	if (popup->cb != NULL && popup->cb->kbd != NULL)
		popup->cb->kbd(popup, popup->arg, event);
}

/** Handle position event in popup window.
 *
 * @param window Window
 * @param arg Argument (ui_popup_t *)
 * @param event Position event
 */
static void ui_popup_window_pos(ui_window_t *window, void *arg,
    pos_event_t *event)
{
	ui_popup_t *popup = (ui_popup_t *)arg;

	/* Remember ID of device that sent the last event */
	popup->idev_id = event->pos_id;

	if (popup->cb != NULL && popup->cb->pos != NULL)
		popup->cb->pos(popup, popup->arg, event);
}

/** @}
 */
