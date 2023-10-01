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

/** @addtogroup taskbar
 * @{
 */
/** @file Task bar start menu
 */

#include <gfx/coord.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/menu.h>
#include <ui/menuentry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "tbsmenu.h"

static void tbsmenu_smenu_close_req(ui_menu_t *, void *);

/** Start menu callbacks */
static ui_menu_cb_t tbsmenu_smenu_cb = {
	.close_req = tbsmenu_smenu_close_req,
};

static void tbsmenu_button_clicked(ui_pbutton_t *, void *);

/** Start button callbacks */
static ui_pbutton_cb_t tbsmenu_button_cb = {
	.clicked = tbsmenu_button_clicked
};

/** Create task bar start menu.
 *
 * @param window Containing window
 * @param fixed Fixed layout to which start button will be added
 * @param rtbsmenu Place to store pointer to new start menu
 * @return @c EOK on success or an error code
 */
errno_t tbsmenu_create(ui_window_t *window, ui_fixed_t *fixed,
    tbsmenu_t **rtbsmenu)
{
	ui_resource_t *res = ui_window_get_res(window);
	tbsmenu_t *tbsmenu = NULL;
	ui_menu_entry_t *tentry;
	errno_t rc;

	tbsmenu = calloc(1, sizeof(tbsmenu_t));
	if (tbsmenu == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_pbutton_create(res, "Start", &tbsmenu->sbutton);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(tbsmenu->sbutton, &tbsmenu_button_cb,
	    (void *)tbsmenu);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(tbsmenu->sbutton));
	if (rc != EOK)
		goto error;

	rc = ui_menu_create(window, &tbsmenu->smenu);
	if (rc != EOK)
		goto error;

	ui_menu_set_cb(tbsmenu->smenu, &tbsmenu_smenu_cb, (void *)tbsmenu);

	rc = ui_menu_entry_create(tbsmenu->smenu, "~N~avigator", "", &tentry);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(tbsmenu->smenu, "Text ~E~ditor", "", &tentry);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(tbsmenu->smenu, "~T~erminal", "", &tentry);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(tbsmenu->smenu, "~C~alculator", "", &tentry);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(tbsmenu->smenu, "~U~I Demo", "", &tentry);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(tbsmenu->smenu, "~G~FX Demo", "", &tentry);
	if (rc != EOK)
		goto error;

	tbsmenu->window = window;
	tbsmenu->fixed = fixed;
	list_initialize(&tbsmenu->entries);

	*rtbsmenu = tbsmenu;
	return EOK;
error:
	if (tbsmenu != NULL)
		ui_pbutton_destroy(tbsmenu->sbutton);
	if (tbsmenu != NULL)
		free(tbsmenu);
	return rc;
}

/** Set window list rectangle.
 *
 * @param tbsmenu Window list
 * @param rect Rectangle
 */
void tbsmenu_set_rect(tbsmenu_t *tbsmenu, gfx_rect_t *rect)
{
	tbsmenu->rect = *rect;
	ui_pbutton_set_rect(tbsmenu->sbutton, rect);
}

/** Destroy task bar start menu.
 *
 * @param tbsmenu Start menu
 */
void tbsmenu_destroy(tbsmenu_t *tbsmenu)
{
	tbsmenu_entry_t *entry;

	// TODO Close libstartmenu

	/* Destroy entries */
	entry = tbsmenu_first(tbsmenu);
	while (entry != NULL) {
		(void)tbsmenu_remove(tbsmenu, entry, false);
		entry = tbsmenu_first(tbsmenu);
	}

	ui_fixed_remove(tbsmenu->fixed, ui_pbutton_ctl(tbsmenu->sbutton));
	ui_pbutton_destroy(tbsmenu->sbutton);
	ui_menu_destroy(tbsmenu->smenu);

	free(tbsmenu);
}

/** Remove entry from window list.
 *
 * @param tbsmenu Window list
 * @param entry Window list entry
 * @param paint @c true to repaint window list
 * @return @c EOK on success or an error code
 */
errno_t tbsmenu_remove(tbsmenu_t *tbsmenu, tbsmenu_entry_t *entry,
    bool paint)
{
	errno_t rc = EOK;

	assert(entry->tbsmenu == tbsmenu);

	list_remove(&entry->lentries);

	// TODO Destroy menu entry
	free(entry);
	return rc;
}

/** Handle start menu close request.
 *
 * @param menu Menu
 * @param arg Argument (tbsmenu_t *)
 * @param wnd_id Window ID
 */
static void tbsmenu_smenu_close_req(ui_menu_t *menu, void *arg)
{
	tbsmenu_t *tbsmenu = (tbsmenu_t *)arg;

	(void)tbsmenu;
	ui_menu_close(menu);
}

/** Get first start menu entry.
 *
 * @param tbsmenu Start menu
 * @return First entry or @c NULL if the menu is empty
 */
tbsmenu_entry_t *tbsmenu_first(tbsmenu_t *tbsmenu)
{
	link_t *link;

	link = list_first(&tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get last start menu entry.
 *
 * @param tbsmenu Start menu
 * @return Last entry or @c NULL if the menu is empty
 */
tbsmenu_entry_t *tbsmenu_last(tbsmenu_t *tbsmenu)
{
	link_t *link;

	link = list_last(&tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get next start menu entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
tbsmenu_entry_t *tbsmenu_next(tbsmenu_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get number of start menu entries.
 *
 * @param tbsmenu Start menu
 * @return Number of entries
 */
size_t tbsmenu_count(tbsmenu_t *tbsmenu)
{
	return list_count(&tbsmenu->entries);
}

/** Start button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (tbsmenu_entry_t *)
 */
static void tbsmenu_button_clicked(ui_pbutton_t *pbutton, void *arg)
{
	tbsmenu_t *tbsmenu = (tbsmenu_t *)arg;

	if (!ui_menu_is_open(tbsmenu->smenu)) {
		// XXX ev_pos_id is not set!!!
		(void) ui_menu_open(tbsmenu->smenu, &tbsmenu->rect,
		    tbsmenu->ev_pos_id);
	} else {
		/* menu is open */
		ui_menu_close(tbsmenu->smenu);
	}
}

/** @}
 */
