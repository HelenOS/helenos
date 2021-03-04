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

#include <ctype.h>
#include <display.h>
#include <errno.h>
#include <fibril.h>
#include <io/console.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include "../private/window.h"
#include "../private/ui.h"

/** Parse output specification.
 *
 * Output specification has the form <proto>@<service> where proto is
 * eiher 'disp' for display service or 'cons' for console. Service
 * is a location ID service name (e.g. hid/display).
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
 *              the default output
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create(const char *ospec, ui_t **rui)
{
	errno_t rc;
	display_t *display;
	console_ctrl_t *console;
	ui_winsys_t ws;
	const char *osvc;
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

		/* ws == ui_ws_console */
		rc = ui_create_cons(console, &ui);
		if (rc != EOK) {
			console_done(console);
			return rc;
		}
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
		if (ui->console != NULL)
			console_done(ui->console);
		if (ui->display != NULL)
			display_close(ui->display);
	}

	free(ui);
}

static void ui_cons_event_process(ui_t *ui, cons_event_t *event)
{
	if (ui->root_wnd == NULL)
		return;

	switch (event->type) {
	case CEV_KEY:
		ui_window_send_kbd(ui->root_wnd, &event->ev.key);
		break;
	case CEV_POS:
		ui_wdecor_pos_event(ui->root_wnd->wdecor, &event->ev.pos);
		ui_window_send_pos(ui->root_wnd, &event->ev.pos);
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

/** @}
 */
