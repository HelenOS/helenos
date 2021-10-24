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
 * @file User interface
 */

#include <adt/list.h>
#include <ctype.h>
#include <display.h>
#include <errno.h>
#include <fibril.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <io/console.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "../private/window.h"
#include "../private/ui.h"

/** Parse output specification.
 *
 * Output specification has the form <proto>@<service> where proto is
 * eiher 'disp' for display service, 'cons' for console, 'null'
 * for dummy output. Service is a location ID service name (e.g. hid/display).
 *
 * @param ospec Output specification
 * @param ws Place to store window system type (protocol)
 * @param osvc Place to store pointer to output service name
 */
static void ui_ospec_parse(const char *ospec, ui_winsys_t *ws,
    const char **osvc)
{
	const char *cp;

	if (ospec == UI_DISPLAY_DEFAULT) {
		*ws = ui_ws_display;
		*osvc = DISPLAY_DEFAULT;
		return;
	}

	cp = ospec;
	while (isalpha(*cp))
		++cp;

	if (*cp == '@') {
		if (str_lcmp(ospec, "disp@", str_length("disp@")) == 0) {
			*ws = ui_ws_display;
		} else if (str_lcmp(ospec, "cons@", str_length("cons@")) == 0) {
			*ws = ui_ws_console;
		} else if (str_lcmp(ospec, "null@", str_length("null@")) == 0) {
			*ws = ui_ws_null;
		} else {
			*ws = ui_ws_unknown;
		}

		if (cp[1] != '\0')
			*osvc = cp + 1;
		else
			*osvc = NULL;
	} else {
		*ws = ui_ws_display;
		*osvc = ospec;
	}
}

/** Create new user interface.
 *
 * @param ospec Output specification or @c UI_DISPLAY_DEFAULT to use
 *              the default display service, UI_CONSOLE_DEFAULT to use
 *		the default console service, UI_DISPLAY_NULL to use
 *		dummy output.
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create(const char *ospec, ui_t **rui)
{
	errno_t rc;
	display_t *display;
	console_ctrl_t *console;
	console_gc_t *cgc;
	ui_winsys_t ws;
	const char *osvc;
	sysarg_t cols;
	sysarg_t rows;
	ui_t *ui;

	ui_ospec_parse(ospec, &ws, &osvc);

	if (ws == ui_ws_display) {
		rc = display_open(osvc, &display);
		if (rc != EOK)
			return rc;

		rc = ui_create_disp(display, &ui);
		if (rc != EOK) {
			display_close(display);
			return rc;
		}
	} else if (ws == ui_ws_console) {
		console = console_init(stdin, stdout);
		if (console == NULL)
			return EIO;

		rc = console_get_size(console, &cols, &rows);
		if (rc != EOK) {
			console_done(console);
			return rc;
		}

		console_cursor_visibility(console, false);

		/* ws == ui_ws_console */
		rc = ui_create_cons(console, &ui);
		if (rc != EOK) {
			console_done(console);
			return rc;
		}

		rc = console_gc_create(console, NULL, &cgc);
		if (rc != EOK) {
			ui_destroy(ui);
			console_done(console);
			return rc;
		}

		ui->cgc = cgc;
		ui->rect.p0.x = 0;
		ui->rect.p0.y = 0;
		ui->rect.p1.x = cols;
		ui->rect.p1.y = rows;

		(void) ui_paint(ui);
	} else if (ws == ui_ws_null) {
		rc = ui_create_disp(NULL, &ui);
		if (rc != EOK)
			return rc;
	} else {
		return EINVAL;
	}

	ui->myoutput = true;
	*rui = ui;
	return EOK;
}

/** Create new user interface using console service.
 *
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create_cons(console_ctrl_t *console, ui_t **rui)
{
	ui_t *ui;

	ui = calloc(1, sizeof(ui_t));
	if (ui == NULL)
		return ENOMEM;

	ui->console = console;
	list_initialize(&ui->windows);
	*rui = ui;
	return EOK;
}

/** Create new user interface using display service.
 *
 * @param disp Display
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create_disp(display_t *disp, ui_t **rui)
{
	ui_t *ui;

	ui = calloc(1, sizeof(ui_t));
	if (ui == NULL)
		return ENOMEM;

	ui->display = disp;
	list_initialize(&ui->windows);
	*rui = ui;
	return EOK;
}

/** Destroy user interface.
 *
 * @param ui User interface or @c NULL
 */
void ui_destroy(ui_t *ui)
{
	if (ui == NULL)
		return;

	if (ui->myoutput) {
		if (ui->cgc != NULL)
			console_gc_delete(ui->cgc);
		if (ui->console != NULL) {
			console_cursor_visibility(ui->console, true);
			console_done(ui->console);
		}
		if (ui->display != NULL)
			display_close(ui->display);
	}

	free(ui);
}

static void ui_cons_event_process(ui_t *ui, cons_event_t *event)
{
	ui_window_t *awnd;
	ui_evclaim_t claim;
	pos_event_t pos;

	awnd = ui_window_get_active(ui);
	if (awnd == NULL)
		return;

	switch (event->type) {
	case CEV_KEY:
		ui_window_send_kbd(awnd, &event->ev.key);
		break;
	case CEV_POS:
		pos = event->ev.pos;
		/* Translate event to window-relative coordinates */
		pos.hpos -= awnd->dpos.x;
		pos.vpos -= awnd->dpos.y;

		claim = ui_wdecor_pos_event(awnd->wdecor, &pos);
		/* Note: If event is claimed, awnd might not be valid anymore */
		if (claim == ui_unclaimed)
			ui_window_send_pos(awnd, &pos);

		break;
	}
}

/** Execute user interface.
 *
 * Return task exit code of zero and block unitl the application starts
 * the termination process by calling ui_quit(@a ui).
 *
 * @param ui User interface
 */
void ui_run(ui_t *ui)
{
	cons_event_t event;
	usec_t timeout;
	errno_t rc;

	/* Only return command prompt if we are running in a separate window */
	if (ui->display != NULL)
		task_retval(0);

	while (!ui->quit) {
		if (ui->console != NULL) {
			timeout = 100000;
			rc = console_get_event_timeout(ui->console,
			    &event, &timeout);

			/* Do we actually have an event? */
			if (rc == EOK) {
				ui_cons_event_process(ui, &event);
			} else if (rc != ETIMEOUT) {
				/* Error, quit */
				break;
			}
		} else {
			fibril_usleep(100000);
		}
	}
}

/** Repaint UI (only used in fullscreen mode).
 *
 * This is used when an area is exposed in fullscreen mode.
 *
 * @param ui UI
 * @return @c EOK on success or an error code
 */
errno_t ui_paint(ui_t *ui)
{
	errno_t rc;
	gfx_context_t *gc;
	ui_window_t *awnd;
	gfx_color_t *color = NULL;

	/* In case of null output */
	if (ui->cgc == NULL)
		return EOK;

	gc = console_gc_get_ctx(ui->cgc);

	rc = gfx_color_new_ega(0x11, &color);
	if (rc != EOK)
		return rc;

	rc = gfx_set_color(gc, color);
	if (rc != EOK) {
		gfx_color_delete(color);
		return rc;
	}

	rc = gfx_fill_rect(gc, &ui->rect);
	if (rc != EOK) {
		gfx_color_delete(color);
		return rc;
	}

	gfx_color_delete(color);

	/* XXX Should repaint all windows */
	awnd = ui_window_get_active(ui);
	if (awnd == NULL)
		return EOK;

	rc = ui_wdecor_paint(awnd->wdecor);
	if (rc != EOK)
		return rc;

	return ui_window_paint(awnd);
}

/** Terminate user interface.
 *
 * Calling this function causes the user interface to terminate
 * (i.e. exit from ui_run()). This would be typically called from
 * an event handler.
 *
 * @param ui User interface
 */
void ui_quit(ui_t *ui)
{
	ui->quit = true;
}

/** Determine if we are running in text mode.
 *
 * @param ui User interface
 * @return @c true iff we are running in text mode
 */
bool ui_is_textmode(ui_t *ui)
{
	/*
	 * XXX Currently console is always text and display is always
	 * graphics, but this need not always be true.
	 */
	return (ui->console != NULL);
}

/** Determine if we are emulating windows.
 *
 * @param ui User interface
 * @return @c true iff we are running in text mode
 */
bool ui_is_fullscreen(ui_t *ui)
{
	return (ui->display == NULL);
}

/** @}
 */
