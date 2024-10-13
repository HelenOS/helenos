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
#include <types/common.h>
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
 * @param ridev_id Place to store input device ID
 * @return EOK on success, EINVAL if syntax is invalid, ENOMEM if out of
 *         memory
 */
static errno_t ui_ospec_parse(const char *ospec, ui_winsys_t *ws,
    char **osvc, sysarg_t *ridev_id)
{
	const char *cp;
	const char *qm;
	const char *endptr;
	uint64_t idev_id;
	errno_t rc;

	*ridev_id = 0;

	cp = ospec;
	while (isalpha(*cp))
		++cp;

	/* Window system / protocol */
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

		++cp;
	} else {
		*ws = ui_ws_display;
	}

	/* Output service is the part before question mark */
	qm = str_chr(cp, '?');
	if (qm != NULL) {
		*osvc = str_ndup(cp, qm - cp);
	} else {
		/* No question mark */
		*osvc = str_dup(cp);
	}

	if (*osvc == NULL)
		return ENOMEM;

	if (qm != NULL) {
		/* The part after the question mark */
		cp = qm + 1;

		/* Input device ID parameter */
		if (str_lcmp(cp, "idev=", str_length("idev=")) == 0) {
			cp += str_length("idev=");

			rc = str_uint64_t(cp, &endptr, 10, false, &idev_id);
			if (rc != EOK)
				goto error;

			*ridev_id = idev_id;
			cp = endptr;
		}
	}

	if (*cp != '\0') {
		rc = EINVAL;
		goto error;
	}

	return EOK;
error:
	free(*osvc);
	*osvc = NULL;
	return rc;
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
	char *osvc;
	sysarg_t cols;
	sysarg_t rows;
	sysarg_t idev_id;
	ui_t *ui;

	rc = ui_ospec_parse(ospec, &ws, &osvc, &idev_id);
	if (rc != EOK)
		return rc;

	if (ws == ui_ws_display || ws == ui_ws_any) {
		rc = display_open((str_cmp(osvc, "") != 0) ? osvc :
		    DISPLAY_DEFAULT, &display);
		if (rc != EOK)
			goto disp_fail;

		rc = ui_create_disp(display, &ui);
		if (rc != EOK) {
			display_close(display);
			goto disp_fail;
		}

		free(osvc);
		ui->myoutput = true;
		ui->idev_id = idev_id;
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

		free(osvc);

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
		free(osvc);
		rc = ui_create_disp(NULL, &ui);
		if (rc != EOK)
			return rc;

		ui->myoutput = true;
		*rui = ui;
		return EOK;
	}

	free(osvc);
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
		ui_lock(ui);
		ui_window_send_kbd(awnd, &event->ev.key);
		ui_unlock(ui);
		break;
	case CEV_POS:
		pos = event->ev.pos;
		/* Translate event to window-relative coordinates */
		pos.hpos -= awnd->dpos.x;
		pos.vpos -= awnd->dpos.y;

		claim = ui_wdecor_pos_event(awnd->wdecor, &pos);
		/* Note: If event is claimed, awnd might not be valid anymore */
		if (claim == ui_unclaimed) {
			ui_lock(ui);
			ui_window_send_pos(awnd, &pos);
			ui_unlock(ui);
		}

		break;
	case CEV_RESIZE:
		ui_lock(ui);
		ui_window_send_resize(awnd);
		ui_unlock(ui);
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
	errno_t rc;

	assert(!ui->suspended);

	if (ui->cgc == NULL) {
		ui->suspended = true;
		return EOK;
	}

	(void) console_set_caption(ui->console, "");
	rc = console_gc_suspend(ui->cgc);
	if (rc != EOK)
		return rc;

	ui->suspended = true;
	return EOK;
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

	assert(ui->suspended);

	if (ui->cgc == NULL) {
		ui->suspended = false;
		return EOK;
	}

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

	ui->suspended = false;

	awnd = ui_window_get_active(ui);
	if (awnd != NULL)
		(void) console_set_caption(ui->console, awnd->wdecor->caption);

	rc = gfx_cursor_set_visible(console_gc_get_ctx(ui->cgc), false);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Determine if UI is suspended.
 *
 * @param ui UI
 * @return @c true iff UI is suspended
 */
bool ui_is_suspended(ui_t *ui)
{
	return ui->suspended;
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

/** Get UI screen rectangle.
 *
 * @param ui User interface
 * @param rect Place to store bounding rectangle
 */
errno_t ui_get_rect(ui_t *ui, gfx_rect_t *rect)
{
	display_info_t info;
	sysarg_t cols, rows;
	errno_t rc;

	if (ui->display != NULL) {
		rc = display_get_info(ui->display, &info);
		if (rc != EOK)
			return rc;

		*rect = info.rect;
	} else if (ui->console != NULL) {
		rc = console_get_size(ui->console, &cols, &rows);
		if (rc != EOK)
			return rc;

		rect->p0.x = 0;
		rect->p0.y = 0;
		rect->p1.x = cols;
		rect->p1.y = rows;
	} else {
		return ENOTSUP;
	}

	return EOK;
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
