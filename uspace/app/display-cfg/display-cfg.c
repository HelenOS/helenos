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

/** @addtogroup display-cfg
 * @{
 */
/** @file Display configuration utility (UI)
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/tabset.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "display-cfg.h"
#include "seats.h"

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (dcfg)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	display_cfg_t *dcfg = (display_cfg_t *) arg;

	ui_quit(dcfg->ui);
}

/** Create display configuration dialog.
 *
 * @param display_spec Display specification
 * @param dcfg_svc Display configuration service name or DISPCFG_DEFAULT
 * @param rdcfg Place to store pointer to new display configuration
 * @return EOK on success or an error code
 */
errno_t display_cfg_create(const char *display_spec, display_cfg_t **rdcfg)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	display_cfg_t *dcfg = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	errno_t rc;

	dcfg = calloc(1, sizeof(display_cfg_t));
	if (dcfg == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Display Configuration";
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

	dcfg->ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &window_cb, (void *)dcfg);
	dcfg->window = window;

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&dcfg->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_tab_set_create(ui_res, &dcfg->tabset);
	if (rc != EOK) {
		printf("Error creating tab set.\n");
		return rc;
	}

	ui_window_get_app_rect(window, &rect);
	ui_tab_set_set_rect(dcfg->tabset, &rect);

	rc = ui_fixed_add(dcfg->fixed, ui_tab_set_ctl(dcfg->tabset));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = dcfg_seats_create(dcfg, &dcfg->seats);
	if (rc != EOK)
		goto error;

	ui_window_add(window, ui_fixed_ctl(dcfg->fixed));

	*rdcfg = dcfg;
	return EOK;
error:
	if (dcfg->seats != NULL)
		dcfg_seats_destroy(dcfg->seats);
	if (dcfg->tabset != NULL)
		ui_tab_set_destroy(dcfg->tabset);
	if (dcfg->fixed != NULL)
		ui_fixed_destroy(dcfg->fixed);
	if (dcfg->ui != NULL)
		ui_destroy(ui);
	free(dcfg);
	return rc;
}

/** Open display configuration service.
 *
 * @param dcfg Display configuration dialog
 * @param dcfg_svc Display configuration service name or DISPCFG_DEFAULT
 * @return EOK on success or an error code
 */
errno_t display_cfg_open(display_cfg_t *dcfg, const char *dcfg_svc)
{
	errno_t rc;

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dcfg->dispcfg);
	if (rc != EOK) {
		printf("Error opening display configuration service.\n");
		goto error;
	}

	return EOK;
error:
	return rc;
}

/** Populate display configuration from isplay configuration service.
 *
 * @param dcfg Display configuration dialog
 * @return EOK on success or an error code
 */
errno_t display_cfg_populate(display_cfg_t *dcfg)
{
	errno_t rc;

	rc = dcfg_seats_populate(dcfg->seats);
	if (rc != EOK)
		return rc;

	rc = ui_window_paint(dcfg->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	return EOK;
}

/** Destroy display configuration dialog.
 *
 * @param dcfg Display configuration dialog
 */
void display_cfg_destroy(display_cfg_t *dcfg)
{
	if (dcfg->dispcfg != NULL)
		dispcfg_close(dcfg->dispcfg);
	ui_window_destroy(dcfg->window);
	ui_destroy(dcfg->ui);
}

/** @}
 */
