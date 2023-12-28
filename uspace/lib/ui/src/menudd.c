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
 * @file Menu drop-down
 *
 * One of the drop-down menus of a menu bar. This class takes the generic
 * ui_menu and ties it to the menu bar.
 */

#include <adt/list.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <ui/accel.h>
#include <ui/menu.h>
#include <ui/menudd.h>
#include <ui/menubar.h>
#include <ui/resource.h>
#include "../private/menubar.h"
#include "../private/menudd.h"

static void ui_menu_dd_left(ui_menu_t *, void *, sysarg_t);
static void ui_menu_dd_right(ui_menu_t *, void *, sysarg_t);
static void ui_menu_dd_close_req(ui_menu_t *, void *);
static void ui_menu_dd_press_accel(ui_menu_t *, void *, char32_t, sysarg_t);

static ui_menu_cb_t ui_menu_dd_menu_cb = {
	.left = ui_menu_dd_left,
	.right = ui_menu_dd_right,
	.close_req = ui_menu_dd_close_req,
	.press_accel = ui_menu_dd_press_accel
};

/** Create new menu drop-down.
 *
 * @param mbar Menu bar
 * @param caption Caption
 * @param rmdd Place to store pointer to new menu drop-down or @c NULL
 * @param rmenu Place to store pointer to new menu or @c NULL
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_dd_create(ui_menu_bar_t *mbar, const char *caption,
    ui_menu_dd_t **rmdd, ui_menu_t **rmenu)
{
	errno_t rc;
	ui_menu_dd_t *mdd;

	mdd = calloc(1, sizeof(ui_menu_dd_t));
	if (mdd == NULL)
		return ENOMEM;

	mdd->caption = str_dup(caption);
	if (mdd->caption == NULL) {
		free(mdd);
		return ENOMEM;
	}

	/* Create menu */
	rc = ui_menu_create(mbar->window, &mdd->menu);
	if (rc != EOK) {
		free(mdd->caption);
		free(mdd);
		return rc;
	}

	mdd->mbar = mbar;
	list_append(&mdd->lmenudds, &mbar->menudds);

	ui_menu_set_cb(mdd->menu, &ui_menu_dd_menu_cb, (void *)mdd);

	if (rmdd != NULL)
		*rmdd = mdd;
	if (rmenu != NULL)
		*rmenu = mdd->menu;
	return EOK;
}

/** Destroy menu drop-down.
 *
 * @param menu Menu or @c NULL
 */
void ui_menu_dd_destroy(ui_menu_dd_t *mdd)
{
	if (mdd == NULL)
		return;

	/* Destroy menu */
	ui_menu_destroy(mdd->menu);

	list_remove(&mdd->lmenudds);
	free(mdd->caption);
	free(mdd);
}

/** Get first menu drop-down in menu bar.
 *
 * @param mbar Menu bar
 * @return First menu or @c NULL if there is none
 */
ui_menu_dd_t *ui_menu_dd_first(ui_menu_bar_t *mbar)
{
	link_t *link;

	link = list_first(&mbar->menudds);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_dd_t, lmenudds);
}

/** Get next menu drop-down in menu bar.
 *
 * @param cur Current menu drop-down
 * @return Next menu drop-down or @c NULL if @a cur is the last one
 */
ui_menu_dd_t *ui_menu_dd_next(ui_menu_dd_t *cur)
{
	link_t *link;

	link = list_next(&cur->lmenudds, &cur->mbar->menudds);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_dd_t, lmenudds);
}

/** Get last menu drop-down in menu bar.
 *
 * @param mbar Menu bar
 * @return Last menu drop-down or @c NULL if there is none
 */
ui_menu_dd_t *ui_menu_dd_last(ui_menu_bar_t *mbar)
{
	link_t *link;

	link = list_last(&mbar->menudds);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_dd_t, lmenudds);
}

/** Get previous menu drop-down in menu bar.
 *
 * @param cur Current menu drop-down
 * @return Previous menu drop-down or @c NULL if @a cur is the fist one
 */
ui_menu_dd_t *ui_menu_dd_prev(ui_menu_dd_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lmenudds, &cur->mbar->menudds);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_dd_t, lmenudds);
}

/** Get menu drop-down caption.
 *
 * @param mdd Menu drop-down
 * @return Caption (owned by @a menu)
 */
const char *ui_menu_dd_caption(ui_menu_dd_t *mdd)
{
	return mdd->caption;
}

/** Get menu drop-down accelerator character.
 *
 * @param mdd Menu drop-down
 * @return Accelerator character (lowercase) or the null character if
 *         the menu has no accelerator.
 */
char32_t ui_menu_dd_get_accel(ui_menu_dd_t *mdd)
{
	return ui_accel_get(mdd->caption);
}

/** Open menu drop-down.
 *
 * @param mdd Menu drop-down
 * @param prect Parent rectangle around which the drop-down should be placed
 * @param idev_id Input device associated with the drop-down's seat
 */
errno_t ui_menu_dd_open(ui_menu_dd_t *mdd, gfx_rect_t *prect, sysarg_t idev_id)
{
	return ui_menu_open(mdd->menu, prect, idev_id);
}

/** Close menu drop-down.
 *
 * @param mdd Menu drop-down
 */
void ui_menu_dd_close(ui_menu_dd_t *mdd)
{
	ui_menu_close(mdd->menu);
}

/** Determine if menu drop-down is open.
 *
 * @param mdd Menu drop-down
 * @return @c true iff menu drop-down is open
 */
bool ui_menu_dd_is_open(ui_menu_dd_t *mdd)
{
	return ui_menu_is_open(mdd->menu);
}

/** Handle menu left event.
 *
 * @param menu Menu
 * @param arg Argument (ui_menu_dd_t *)
 * @param idev_id Input device ID
 */
static void ui_menu_dd_left(ui_menu_t *menu, void *arg, sysarg_t idev_id)
{
	ui_menu_dd_t *mdd = (ui_menu_dd_t *)arg;

	(void)menu;

	ui_menu_bar_left(mdd->mbar, idev_id);
}

/** Handle menu right event.
 *
 * @param menu Menu
 * @param arg Argument (ui_menu_dd_t *)
 * @param idev_id Input device ID
 */
static void ui_menu_dd_right(ui_menu_t *menu, void *arg, sysarg_t idev_id)
{
	ui_menu_dd_t *mdd = (ui_menu_dd_t *)arg;

	(void)menu;

	ui_menu_bar_right(mdd->mbar, idev_id);
}

/** Handle menu close request.
 *
 * @param menu Menu
 * @param arg Argument (ui_menu_dd_t *)
 */
static void ui_menu_dd_close_req(ui_menu_t *menu, void *arg)
{
	ui_menu_dd_t *mdd = (ui_menu_dd_t *)arg;

	(void)menu;
	ui_menu_bar_deactivate(mdd->mbar);
}

/** Handle menu accelerator key press event.
 *
 * @param menu Menu
 * @param arg Argument (ui_menu_dd_t *)
 * @param c Character
 * @param kbd_id Keyboard ID
 */
static void ui_menu_dd_press_accel(ui_menu_t *menu, void *arg, char32_t c,
    sysarg_t kbd_id)
{
	ui_menu_dd_t *mdd = (ui_menu_dd_t *)arg;

	(void)menu;
	ui_menu_bar_press_accel(mdd->mbar, c, kbd_id);
}

/** @}
 */
