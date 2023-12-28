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

/** @addtogroup taskbar-cfg
 * @{
 */
/** @file Taskbar configuration utility (UI)
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/tabset.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "taskbar-cfg.h"
#include "startmenu.h"

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (tbcfg)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	taskbar_cfg_t *tbcfg = (taskbar_cfg_t *) arg;

	ui_quit(tbcfg->ui);
}

/** Create Taskbar configuration window.
 *
 * @param display_spec Display specification
 * @param rdcfg Place to store pointer to new taskbar configuration
 * @return EOK on success or an error code
 */
errno_t taskbar_cfg_create(const char *display_spec, taskbar_cfg_t **rdcfg)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	taskbar_cfg_t *tbcfg = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	errno_t rc;

	tbcfg = calloc(1, sizeof(taskbar_cfg_t));
	if (tbcfg == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Taskbar Configuration";
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 70;
		params.rect.p1.y = 23;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 470;
		params.rect.p1.y = 350;
	}

	tbcfg->ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &window_cb, (void *)tbcfg);
	tbcfg->window = window;

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&tbcfg->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_tab_set_create(ui_res, &tbcfg->tabset);
	if (rc != EOK) {
		printf("Error creating tab set.\n");
		return rc;
	}

	ui_window_get_app_rect(window, &rect);
	ui_tab_set_set_rect(tbcfg->tabset, &rect);

	rc = ui_fixed_add(tbcfg->fixed, ui_tab_set_ctl(tbcfg->tabset));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = startmenu_create(tbcfg, &tbcfg->startmenu);
	if (rc != EOK)
		goto error;

	ui_window_add(window, ui_fixed_ctl(tbcfg->fixed));

	*rdcfg = tbcfg;
	return EOK;
error:
	if (tbcfg->startmenu != NULL)
		startmenu_destroy(tbcfg->startmenu);
	if (tbcfg->tabset != NULL)
		ui_tab_set_destroy(tbcfg->tabset);
	if (tbcfg->fixed != NULL)
		ui_fixed_destroy(tbcfg->fixed);
	if (tbcfg->ui != NULL)
		ui_destroy(ui);
	free(tbcfg);
	return rc;
}

/** Open Taskbar configuration.
 *
 * @param tbcfg Taskbar configuration dialog
 * @param cfg_repo Path to configuration repository
 * @return EOK on success or an error code
 */
errno_t taskbar_cfg_open(taskbar_cfg_t *tbcfg, const char *cfg_repo)
{
	errno_t rc;

	rc = tbarcfg_open(cfg_repo, &tbcfg->tbarcfg);
	if (rc != EOK) {
		rc = tbarcfg_create(cfg_repo, &tbcfg->tbarcfg);
		if (rc != EOK) {
			printf("Error opening Taskbar configuration.\n");
			goto error;
		}
	}

	return EOK;
error:
	return rc;
}

/** Populate task configuration from configuration repository.
 *
 * @param tbcfg Taskbar configuration dialog
 * @return EOK on success or an error code
 */
errno_t taskbar_cfg_populate(taskbar_cfg_t *tbcfg)
{
	errno_t rc;

	rc = startmenu_populate(tbcfg->startmenu, tbcfg->tbarcfg);
	if (rc != EOK)
		return rc;

	rc = ui_window_paint(tbcfg->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	return EOK;
}

/** Destroy Taskbar configuration window.
 *
 * @param tbcfg Taskbar configuration window
 */
void taskbar_cfg_destroy(taskbar_cfg_t *tbcfg)
{
	if (tbcfg->tbarcfg != NULL)
		tbarcfg_close(tbcfg->tbarcfg);
	ui_window_destroy(tbcfg->window);
	ui_destroy(tbcfg->ui);
}

/** @}
 */
