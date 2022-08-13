/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/** @file Navigator.
 *
 * HelenOS file manager.
 */

#include <errno.h>
#include <stdlib.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include "menu.h"
#include "nav.h"

/** Create navigator menu.
 *
 * @param window Navigator window
 * @param rmenu Place to store pointer to new menu
 * @return EOK on success or an error code
 */
errno_t nav_menu_create(ui_window_t *window, nav_menu_t **rmenu)
{
	nav_menu_t *menu;
	ui_menu_t *mfile;
	ui_menu_entry_t *mopen;
	ui_menu_entry_t *mfsep;
	ui_menu_entry_t *mexit;
	gfx_rect_t arect;
	gfx_rect_t rect;
	errno_t rc;

	menu = calloc(1, sizeof(nav_menu_t));
	if (menu == NULL)
		return ENOMEM;

	menu->window = window;
	menu->ui = ui_window_get_ui(window);

	rc = ui_menu_bar_create(menu->ui, menu->window,
	    &menu->menubar);
	if (rc != EOK)
		goto error;

	rc = ui_menu_create(menu->menubar, "~F~ile", &mfile);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(mfile, "~O~pen", "Enter", &mopen);
	if (rc != EOK)
		goto error;

	ui_menu_entry_set_cb(mopen, nav_menu_file_open, (void *) menu);

	rc = ui_menu_entry_sep_create(mfile, &mfsep);
	if (rc != EOK)
		goto error;

	rc = ui_menu_entry_create(mfile, "E~x~it", "Ctrl-Q", &mexit);
	if (rc != EOK)
		goto error;

	ui_menu_entry_set_cb(mexit, nav_menu_file_exit, (void *) menu);

	ui_window_get_app_rect(menu->window, &arect);

	rect.p0 = arect.p0;
	rect.p1.x = arect.p1.x;
	rect.p1.y = arect.p0.y + 1;
	ui_menu_bar_set_rect(menu->menubar, &rect);

	*rmenu = menu;
	return EOK;
error:
	nav_menu_destroy(menu);
	return rc;
}

/** Set navigator menu callbacks.
 *
 * @param menu Menu
 * @param cb Callbacks
 * @param arg Argument to callback functions
 */
void nav_menu_set_cb(nav_menu_t *menu, nav_menu_cb_t *cb, void *arg)
{
	menu->cb = cb;
	menu->cb_arg = arg;
}

/** Destroy navigator menu.
 *
 * @param menu Menu
 */
void nav_menu_destroy(nav_menu_t *menu)
{
	if (menu->menubar != NULL)
		ui_menu_bar_destroy(menu->menubar);

	free(menu);
}

/** Return base UI control for the menu bar.
 *
 * @param menu Navigator menu
 * @return UI control
 */
ui_control_t *nav_menu_ctl(nav_menu_t *menu)
{
	return ui_menu_bar_ctl(menu->menubar);
}

/** File / Open menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (navigator_t *)
 */
void nav_menu_file_open(ui_menu_entry_t *mentry, void *arg)
{
	nav_menu_t *menu = (nav_menu_t *)arg;

	if (menu->cb != NULL && menu->cb->file_open != NULL)
		menu->cb->file_open(menu->cb_arg);
}

/** File / Exit menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (navigator_t *)
 */
void nav_menu_file_exit(ui_menu_entry_t *mentry, void *arg)
{
	nav_menu_t *menu = (nav_menu_t *)arg;

	if (menu->cb != NULL && menu->cb->file_exit != NULL)
		menu->cb->file_exit(menu->cb_arg);
}

/** @}
 */
