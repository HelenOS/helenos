/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/cursor.h>
#include <gfx/render.h>
#include <io/console.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <ui/clickmatic.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "../private/wdecor.h"
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
		} else if (str_lcmp(ospec, "@", str_length("@")) == 0) {
			*ws = ui_ws_any;
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

	if (ws == ui_ws_display || ws == ui_ws_any) {
		rc = display_open(osvc != NULL ? osvc : DISPLAY_DEFAULT,
		    &display);
		if (rc != EOK)
			goto disp_fail;

		rc = ui_create_disp(display, &ui);
		if (rc != EOK) {
			display_close(display);
			goto disp_fail;
		}

		ui->myoutput = true;
		*rui = ui;
		return EOK;
	}

disp_fail:
	if (ws == ui_ws_console || ws == ui_ws_any) {
		console = console_init(stdin, stdout);
		if (console == NULL)
			goto cons_fail;

		rc = console_get_size(console, &cols, &rows);
		if (rc != EOK) {
			console_done(console);
			goto cons_fail;
		}

		console_cursor_visibility(console, false);

		/* ws == ui_ws_console */
		rc = ui_create_cons(console, &ui);
		if (rc != EOK) {
			console_done(console);
			goto cons_fail;
		}

		rc = console_gc_create(console, NULL, &cgc);
		if (rc != EOK) {
			ui_destroy(ui);
			console_done(console);
			goto cons_fail;
		}

		ui->cgc = cgc;
		ui->rect.p0.x = 0;
		ui->rect.p0.y = 0;
		ui->rect.p1.x = cols;
		ui->rect.p1.y = rows;

		(void) ui_paint(ui);
		ui->myoutput = true;
		*rui = ui;
		return EOK;
	}

cons_fail:
	if (ws == ui_ws_null) {
		rc = ui_create_disp(NULL, &ui);
		if (rc != EOK)
			return rc;

		ui->myoutput = true;
		*rui = ui;
		return EOK;
	}

	return EINVAL;
}

/** Create new user interface using console service.
 *
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create_cons(console_ctrl_t *console, ui_t **rui)
{
	ui_t *ui;
	errno_t rc;

	ui = calloc(1, sizeof(ui_t));
	if (ui == NULL)
		return ENOMEM;

	rc = ui_clickmatic_create(ui, &ui->clickmatic);
	if (rc != EOK) {
		free(ui);
		return rc;
	}

	ui->console = console;
	list_initialize(&ui->windows);
	fibril_mutex_initialize(&ui->lock);
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
	errno_t rc;

	ui = calloc(1, sizeof(ui_t));
	if (ui == NULL)
		return ENOMEM;

	rc = ui_clickmatic_create(ui, &ui->clickmatic);
	if (rc != EOK) {
		free(ui);
		return rc;
	}

	ui->display = disp;
	list_initialize(&ui->windows);
	fibril_mutex_initialize(&ui->lock);
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

/** Free up console for other users.
 *
 * Release console resources for another application (that the current
 * task is starting). After the other application finishes, resume
 * operation with ui_resume(). No calls to UI must happen inbetween
 * and no events must be processed (i.e. the calling function must not
 * return control to UI.
 *
 * @param ui UI
 * @return EOK on success or an error code
 */
errno_t ui_suspend(ui_t *ui)
{
	if (ui->cgc == NULL)
		return EOK;

	(void) console_set_caption(ui->console, "");
	return console_gc_suspend(ui->cgc);
}

/** Resume suspended UI.
 *
 * Reclaim console resources (after child application has finished running)
 * and restore UI operation previously suspended by calling ui_suspend().
 *
 * @param ui UI
 * @return EOK on success or an error code
 */
errno_t ui_resume(ui_t *ui)
{
	errno_t rc;
	ui_window_t *awnd;
	sysarg_t col;
	sysarg_t row;
	cons_event_t ev;

	if (ui->cgc == NULL)
		return EOK;

	rc = console_get_pos(ui->console, &col, &row);
	if (rc != EOK)
		return rc;

	/*
	 * Here's a little heuristic to help determine if we need
	 * to pause before returning to the UI. If we are in the
	 * top-left corner, chances are the screen is empty and
	 * there is no need to pause.
	 */
	if (col != 0 || row != 0) {
		printf("Press any key or button to continue...\n");

		while (true) {
			rc = console_get_event(ui->console, &ev);
			if (rc != EOK)
				return EIO;

			if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS)
				break;

			if (ev.type == CEV_POS && ev.ev.pos.type == POS_PRESS)
				break;
		}
	}

	rc = console_gc_resume(ui->cgc);
	if (rc != EOK)
		return rc;

	awnd = ui_window_get_active(ui);
	if (awnd != NULL)
		(void) console_set_caption(ui->console, awnd->wdecor->caption);

	return gfx_cursor_set_visible(console_gc_get_ctx(ui->cgc), false);
}

/** Lock UI.
 *
 * Block UI from calling window callbacks. @c ui_lock() and @c ui_unlock()
 * must be used when accessing UI resources from a fibril (as opposed to
 * from a window callback).
 *
 * @param ui UI
 */
void ui_lock(ui_t *ui)
{
	fibril_mutex_lock(&ui->lock);
}

/** Unlock UI.
 *
 * Allow UI to call window callbacks. @c ui_lock() and @c ui_unlock()
 * must be used when accessing window resources from a fibril (as opposed to
 * from a window callback).
 *
 * @param ui UI
 */
void ui_unlock(ui_t *ui)
{
	fibril_mutex_unlock(&ui->lock);
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

/** Get clickmatic from UI.
 *
 * @pararm ui UI
 * @return Clickmatic
 */
ui_clickmatic_t *ui_get_clickmatic(ui_t *ui)
{
	return ui->clickmatic;
}

/** @}
 */
