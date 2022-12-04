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

/** @addtogroup taskbar
 * @{
 */
/** @file Task Bar
 */

#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <wndmgt.h>
#include "clock.h"
#include "taskbar.h"
#include "wndlist.h"

static void taskbar_wnd_close(ui_window_t *, void *);
static void taskbar_wnd_pos(ui_window_t *, void *, pos_event_t *);

static ui_window_cb_t window_cb = {
	.close = taskbar_wnd_close,
	.pos = taskbar_wnd_pos
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (taskbar)
 */
static void taskbar_wnd_close(ui_window_t *window, void *arg)
{
	taskbar_t *taskbar = (taskbar_t *) arg;

	ui_quit(taskbar->ui);
}

/** Window received position event.
 *
 * @param window Window
 * @param arg Argument (taskbar)
 * @param event Position event
 */
static void taskbar_wnd_pos(ui_window_t *window, void *arg, pos_event_t *event)
{
	taskbar_t *taskbar = (taskbar_t *) arg;

	/* Remember ID of device that sent the last event */
	taskbar->wndlist->ev_pos_id = event->pos_id;

	ui_window_def_pos(window, event);
}

/** Create task bar.
 *
 * @param display_spec Display specification
 * @param wndmgt_svc Window management service (or WNDMGT_DEFAULT)
 * @param rtaskbar Place to store pointer to new task bar
 * @return @c EOK on success or an error coe
 */
errno_t taskbar_create(const char *display_spec, const char *wndmgt_svc,
    taskbar_t **rtaskbar)
{
	ui_wnd_params_t params;
	taskbar_t *taskbar = NULL;
	gfx_rect_t scr_rect;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	errno_t rc;

	taskbar = calloc(1, sizeof(taskbar_t));
	if (taskbar == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_create(display_spec, &taskbar->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	rc = ui_get_rect(taskbar->ui, &scr_rect);
	if (rc != EOK) {
		if (str_cmp(display_spec, UI_DISPLAY_NULL) != 0) {
			printf("Error getting screen dimensions.\n");
			goto error;
		}

		/* For the sake of unit tests */
		scr_rect.p0.x = 0;
		scr_rect.p0.y = 0;
		scr_rect.p1.x = 100;
		scr_rect.p1.y = 100;
	}

	ui_wnd_params_init(&params);
	params.caption = "Task Bar";
	params.placement = ui_wnd_place_bottom_left;

	/* Window has no titlebar */
	params.style &= ~ui_wds_titlebar;

	/* Window is not obscured by other windows */
	params.flags |= ui_wndf_topmost;

	/* Prevent taskbar window from being listed in taskbar */
	params.flags |= ui_wndf_system;

	/* Make maximized windows avoid taskbar */
	params.flags |= ui_wndf_avoid;

	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = scr_rect.p1.x - scr_rect.p0.x;

	if (ui_is_textmode(taskbar->ui)) {
		params.rect.p1.y = 1;
		params.style &= ~ui_wds_frame;
	} else {
		params.rect.p1.y = 32;
	}

	rc = ui_window_create(taskbar->ui, &params, &taskbar->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(taskbar->window, &window_cb, (void *)taskbar);
	ui_res = ui_window_get_res(taskbar->window);

	rc = ui_fixed_create(&taskbar->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	rc = ui_label_create(ui_res, "HelenOS", &taskbar->label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	ui_window_get_app_rect(taskbar->window, &rect);
	if (ui_is_textmode(taskbar->ui)) {
		rect.p0.x += 1;
	} else {
		rect.p0.x += 10;
	}
	ui_label_set_rect(taskbar->label, &rect);
	ui_label_set_halign(taskbar->label, gfx_halign_left);
	ui_label_set_valign(taskbar->label, gfx_valign_center);

	rc = ui_fixed_add(taskbar->fixed, ui_label_ctl(taskbar->label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		ui_label_destroy(taskbar->label);
		goto error;
	}

	rc = wndlist_create(taskbar->window, taskbar->fixed, &taskbar->wndlist);
	if (rc != EOK) {
		printf("Error creating window list.\n");
		goto error;
	}

	if (ui_is_textmode(taskbar->ui)) {
		rect.p0.x = params.rect.p0.x + 9;
		rect.p0.y = 0;
		rect.p1.x = params.rect.p1.x - 10;
		rect.p1.y = 1;
	} else {
		rect.p0.x = params.rect.p0.x + 90;
		rect.p0.y = 4;
		rect.p1.x = params.rect.p1.x - 84;
		rect.p1.y = 32 - 4;
	}
	wndlist_set_rect(taskbar->wndlist, &rect);

	rc = wndlist_open_wm(taskbar->wndlist, wndmgt_svc);
	if (rc != EOK) {
		printf("Error attaching window management service.\n");
		goto error;
	}

	rc = taskbar_clock_create(taskbar->window, &taskbar->clock);
	if (rc != EOK)
		goto error;

	if (ui_is_textmode(taskbar->ui)) {
		rect.p0.x = params.rect.p1.x - 10;
		rect.p0.y = 0;
		rect.p1.x = params.rect.p1.x;
		rect.p1.y = 1;
	} else {
		rect.p0.x = params.rect.p1.x - 80;
		rect.p0.y = 4;
		rect.p1.x = params.rect.p1.x - 4;
		rect.p1.y = 32 - 4;
	}

	taskbar_clock_set_rect(taskbar->clock, &rect);

	rc = ui_fixed_add(taskbar->fixed, taskbar_clock_ctl(taskbar->clock));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		taskbar_clock_destroy(taskbar->clock);
		goto error;
	}

	ui_window_add(taskbar->window, ui_fixed_ctl(taskbar->fixed));

	rc = ui_window_paint(taskbar->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	*rtaskbar = taskbar;
	return EOK;
error:
	if (taskbar->clock != NULL)
		taskbar_clock_destroy(taskbar->clock);
	if (taskbar->wndlist != NULL)
		wndlist_destroy(taskbar->wndlist);
	if (taskbar->window != NULL)
		ui_window_destroy(taskbar->window);
	if (taskbar->ui != NULL)
		ui_destroy(taskbar->ui);
	return rc;

}

/** Destroy task bar. */
void taskbar_destroy(taskbar_t *taskbar)
{
	ui_fixed_remove(taskbar->fixed, taskbar_clock_ctl(taskbar->clock));
	taskbar_clock_destroy(taskbar->clock);
	ui_window_destroy(taskbar->window);
	ui_destroy(taskbar->ui);
}

/** @}
 */
