/*
 * Copyright (c) 2020 Jiri Svoboda
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
 * @file Window
 */

#include <display.h>
#include <errno.h>
#include <gfx/context.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/resource.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "../private/dummygc.h"
#include "../private/ui.h"
#include "../private/window.h"

/** Initialize window parameters structure.
 *
 * Window parameters structure must always be initialized using this function
 * first.
 *
 * @param params Window parameters structure
 */
void ui_window_params_init(ui_window_params_t *params)
{
	memset(params, 0, sizeof(ui_window_params_t));
}

/** Create new window.
 *
 * @param ui User interface
 * @param params Window parameters
 * @param rwindow Place to store pointer to new window
 * @return EOK on success or an error code
 */
errno_t ui_window_create(ui_t *ui, ui_window_params_t *params,
    ui_window_t **rwindow)
{
	ui_window_t *window;
	display_wnd_params_t dparams;
	display_window_t *dwindow = NULL;
	gfx_context_t *gc = NULL;
	ui_resource_t *res = NULL;
	ui_wdecor_t *wdecor = NULL;
	dummy_gc_t *dgc = NULL;
	errno_t rc;

	window = calloc(1, sizeof(ui_window_t));
	if (window == NULL)
		return ENOMEM;

	display_wnd_params_init(&dparams);

	if (ui->display != NULL) {
		rc = display_window_create(ui->display, &dparams, NULL, NULL,
		    &dwindow);
		if (rc != EOK)
			goto error;

		rc = display_window_get_gc(dwindow, &gc);
		if (rc != EOK)
			goto error;
	} else {
		/* Needed for unit tests */
		rc = dummygc_create(&dgc);
		if (rc != EOK)
			goto error;

		gc = dummygc_get_ctx(dgc);
	}

	rc = ui_resource_create(gc, &res);
	if (rc != EOK)
		goto error;

	rc = ui_wdecor_create(res, params->caption, &wdecor);
	if (rc != EOK)
		goto error;

	window->ui = ui;
	window->dwindow = dwindow;
	window->gc = gc;
	window->res = res;
	window->wdecor = wdecor;
	*rwindow = window;
	return EOK;
error:
	if (wdecor != NULL)
		ui_wdecor_destroy(wdecor);
	if (res != NULL)
		ui_resource_destroy(res);
	if (dgc != NULL)
		dummygc_destroy(dgc);
	if (dwindow != NULL)
		display_window_destroy(dwindow);
	free(window);
	return rc;
}

/** Destroy window.
 *
 * @param window Window or @c NULL
 */
void ui_window_destroy(ui_window_t *window)
{
	if (window == NULL)
		return;

	ui_wdecor_destroy(window->wdecor);
	ui_resource_destroy(window->res);
	gfx_context_delete(window->gc);
	display_window_destroy(window->dwindow);
	free(window);
}

/** @}
 */
