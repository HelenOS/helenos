/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu bar
 */

#include <adt/list.h>
#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/window.h>
#include "../private/menubar.h"
#include "../private/resource.h"

enum {
	menubar_hpad = 4,
	menubar_vpad = 4,
	menubar_hpad_text = 1,
	menubar_vpad_text = 0
};

static void ui_menu_bar_ctl_destroy(void *);
static errno_t ui_menu_bar_ctl_paint(void *);
static ui_evclaim_t ui_menu_bar_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_menu_bar_ctl_pos_event(void *, pos_event_t *);

/** Menu bar control ops */
ui_control_ops_t ui_menu_bar_ops = {
	.destroy = ui_menu_bar_ctl_destroy,
	.paint = ui_menu_bar_ctl_paint,
	.kbd_event = ui_menu_bar_ctl_kbd_event,
	.pos_event = ui_menu_bar_ctl_pos_event
};

/** Create new menu bar.
 *
 * @param ui UI
 * @param window Window that will contain the menu bar
 * @param rmbar Place to store pointer to new menu bar
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_bar_create(ui_t *ui, ui_window_t *window, ui_menu_bar_t **rmbar)
{
	ui_menu_bar_t *mbar;
	errno_t rc;

	mbar = calloc(1, sizeof(ui_menu_bar_t));
	if (mbar == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_menu_bar_ops, (void *) mbar, &mbar->control);
	if (rc != EOK) {
		free(mbar);
		return rc;
	}

	mbar->ui = ui;
	mbar->window = window;
	list_initialize(&mbar->menus);
	*rmbar = mbar;
	return EOK;
}

/** Destroy menu bar
 *
 * @param mbar Menu bar or @c NULL
 */
void ui_menu_bar_destroy(ui_menu_bar_t *mbar)
{
	ui_menu_t *menu;

	if (mbar == NULL)
		return;

	/* Destroy menus */
	menu = ui_menu_first(mbar);
	while (menu != NULL) {
		ui_menu_destroy(menu);
		menu = ui_menu_first(mbar);
	}

	ui_control_delete(mbar->control);
	free(mbar);
}

/** Get base control from menu bar.
 *
 * @param mbar Menu bar
 * @return Control
 */
ui_control_t *ui_menu_bar_ctl(ui_menu_bar_t *mbar)
{
	return mbar->control;
}

/** Set menu bar rectangle.
 *
 * @param mbar Menu bar
 * @param rect New menu bar rectangle
 */
void ui_menu_bar_set_rect(ui_menu_bar_t *mbar, gfx_rect_t *rect)
{
	mbar->rect = *rect;
}

/** Paint menu bar.
 *
 * @param mbar Menu bar
 * @return EOK on success or an error code
 */
errno_t ui_menu_bar_paint(ui_menu_bar_t *mbar)
{
	ui_resource_t *res;
	ui_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_coord2_t tpos;
	gfx_rect_t rect;
	gfx_color_t *bg_color;
	ui_menu_t *menu;
	const char *caption;
	gfx_coord_t width;
	gfx_coord_t hpad;
	gfx_coord_t vpad;
	errno_t rc;

	res = ui_window_get_res(mbar->window);

	/* Paint menu bar background */

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &mbar->rect);
	if (rc != EOK)
		goto error;

	if (res->textmode) {
		hpad = menubar_hpad_text;
		vpad = menubar_vpad_text;
	} else {
		hpad = menubar_hpad;
		vpad = menubar_vpad;
	}

	pos = mbar->rect.p0;

	ui_text_fmt_init(&fmt);
	fmt.font = res->font;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	menu = ui_menu_first(mbar);
	while (menu != NULL) {
		caption = ui_menu_caption(menu);
		width = ui_text_width(res->font, caption) + 2 * hpad;
		tpos.x = pos.x + hpad;
		tpos.y = pos.y + vpad;

		rect.p0 = pos;
		rect.p1.x = rect.p0.x + width;
		rect.p1.y = mbar->rect.p1.y;

		if (menu == mbar->selected) {
			fmt.color = res->wnd_sel_text_color;
			fmt.hgl_color = res->wnd_sel_text_hgl_color;
			bg_color = res->wnd_sel_text_bg_color;
		} else {
			fmt.color = res->wnd_text_color;
			fmt.hgl_color = res->wnd_text_hgl_color;
			bg_color = res->wnd_face_color;
		}

		rc = gfx_set_color(res->gc, bg_color);
		if (rc != EOK)
			goto error;

		rc = gfx_fill_rect(res->gc, &rect);
		if (rc != EOK)
			goto error;

		rc = ui_paint_text(&tpos, &fmt, caption);
		if (rc != EOK)
			goto error;

		pos.x += width;
		menu = ui_menu_next(menu);
	}

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Select or deselect menu from menu bar.
 *
 * Select @a menu. If @a menu is @c NULL or it is already selected,
 * then select none.
 *
 * @param mbar Menu bar
 * @param menu Menu to select (or deselect if selected) or @c NULL
 * @param openup Open menu even if not currently open
 */
void ui_menu_bar_select(ui_menu_bar_t *mbar, ui_menu_t *menu, bool openup)
{
	ui_menu_t *old_menu;
	gfx_rect_t rect;
	bool was_open;

	old_menu = mbar->selected;

	mbar->selected = menu;

	/* Close previously open menu */
	if (old_menu != NULL && ui_menu_is_open(old_menu)) {
		was_open = true;
		(void) ui_menu_close(old_menu);
	} else {
		was_open = false;
	}

	(void) ui_menu_bar_paint(mbar);

	if (mbar->selected != NULL) {
		ui_menu_bar_entry_rect(mbar, mbar->selected, &rect);
		if (openup || was_open) {
			/*
			 * Open the newly selected menu if either
			 * the old menu was open or @a openup was
			 * specified.
			 */
			(void) ui_menu_open(mbar->selected, &rect);
		}
	}
}

/** Move one entry left.
 *
 * If the selected menu is open, the newly selected menu will be open
 * as well. If we are already at the first entry, we wrap around.
 *
 * @param mbar Menu bar
 */
void ui_menu_bar_left(ui_menu_bar_t *mbar)
{
	ui_menu_t *nmenu;

	if (mbar->selected == NULL)
		return;

	nmenu = ui_menu_prev(mbar->selected);
	if (nmenu == NULL)
		nmenu = ui_menu_last(mbar);

	if (nmenu != mbar->selected)
		ui_menu_bar_select(mbar, nmenu, false);
}

/** Move one entry right.
 *
 * If the selected menu is open, the newly selected menu will be open
 * as well. If we are already at the last entry, we wrap around.
 *
 * @param mbar Menu bar
 */
void ui_menu_bar_right(ui_menu_bar_t *mbar)
{
	ui_menu_t *nmenu;

	if (mbar->selected == NULL)
		return;

	nmenu = ui_menu_next(mbar->selected);
	if (nmenu == NULL)
		nmenu = ui_menu_first(mbar);

	if (nmenu != mbar->selected)
		ui_menu_bar_select(mbar, nmenu, false);
}

/** Handle menu bar key press without modifiers.
 *
 * @param mbar Menu bar
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_bar_key_press_unmod(ui_menu_bar_t *mbar, kbd_event_t *event)
{
	gfx_rect_t rect;

	if (event->key == KC_F10) {
		ui_menu_bar_activate(mbar);
		return ui_claimed;
	}

	if (!mbar->active)
		return ui_unclaimed;

	if (event->key == KC_ESCAPE) {
		ui_menu_bar_deactivate(mbar);
		return ui_claimed;
	}

	if (event->key == KC_LEFT)
		ui_menu_bar_left(mbar);

	if (event->key == KC_RIGHT)
		ui_menu_bar_right(mbar);

	if (event->key == KC_ENTER || event->key == KC_DOWN) {
		if (mbar->selected != NULL && !ui_menu_is_open(mbar->selected)) {
			ui_menu_bar_entry_rect(mbar, mbar->selected,
			    &rect);
			ui_menu_open(mbar->selected, &rect);
		}

		return ui_claimed;
	}

	if (event->c != '\0' && !ui_menu_is_open(mbar->selected)) {
		/* Check if it is an accelerator. */
		ui_menu_bar_press_accel(mbar, event->c);
	}

	return ui_claimed;
}

/** Handle menu bar keyboard event.
 *
 * @param mbar Menu bar
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_bar_kbd_event(ui_menu_bar_t *mbar, kbd_event_t *event)
{
	if ((event->mods & KM_ALT) != 0 &&
	    (event->mods & (KM_CTRL | KM_SHIFT)) == 0 && event->c != '\0') {
		/* Check if it is an accelerator. */
		ui_menu_bar_press_accel(mbar, event->c);
	}

	if (event->type == KEY_PRESS && (event->mods &
	    (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
		return ui_menu_bar_key_press_unmod(mbar, event);
	}

	if (mbar->active)
		return ui_claimed;

	return ui_unclaimed;
}

/** Accelerator key press.
 *
 * If @a c matches an accelerator key, open the respective menu.
 *
 * @param mbar Menu bar
 * @param c Character
 */
void ui_menu_bar_press_accel(ui_menu_bar_t *mbar, char32_t c)
{
	ui_menu_t *menu;
	char32_t maccel;

	menu = ui_menu_first(mbar);
	while (menu != NULL) {
		maccel = ui_menu_get_accel(menu);
		if (c == maccel) {
			ui_menu_bar_select(mbar, menu, true);
			return;
		}

		menu = ui_menu_next(menu);
	}
}

/** Handle menu bar position event.
 *
 * @param mbar Menu bar
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_bar_pos_event(ui_menu_bar_t *mbar, pos_event_t *event)
{
	ui_resource_t *res;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_menu_t *menu;
	const char *caption;
	gfx_coord_t width;
	gfx_coord_t hpad;
	gfx_coord2_t ppos;

	res = ui_window_get_res(mbar->window);

	ppos.x = event->hpos;
	ppos.y = event->vpos;

	if (res->textmode) {
		hpad = menubar_hpad_text;
	} else {
		hpad = menubar_hpad;
	}

	pos = mbar->rect.p0;

	menu = ui_menu_first(mbar);
	while (menu != NULL) {
		caption = ui_menu_caption(menu);
		width = ui_text_width(res->font, caption) + 2 * hpad;

		rect.p0 = pos;
		rect.p1.x = rect.p0.x + width;
		rect.p1.y = mbar->rect.p1.y;

		/* Check if press is inside menu bar entry */
		if (event->type == POS_PRESS &&
		    gfx_pix_inside_rect(&ppos, &rect)) {
			mbar->active = true;

			/* Open the menu, close if already open. */
			if (menu == mbar->selected)
				ui_menu_bar_select(mbar, NULL, false);
			else
				ui_menu_bar_select(mbar, menu, true);

			return ui_claimed;
		}

		pos.x += width;
		menu = ui_menu_next(menu);
	}

	return ui_unclaimed;
}

/** Handle menu bar position event.
 *
 * @param mbar Menu bar
 * @param menu Menu whose entry's rectangle is to be returned
 * @param rrect Place to store entry rectangle
 */
void ui_menu_bar_entry_rect(ui_menu_bar_t *mbar, ui_menu_t *menu,
    gfx_rect_t *rrect)
{
	ui_resource_t *res;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_menu_t *cur;
	const char *caption;
	gfx_coord_t width;
	gfx_coord_t hpad;

	res = ui_window_get_res(mbar->window);

	if (res->textmode) {
		hpad = menubar_hpad_text;
	} else {
		hpad = menubar_hpad;
	}

	pos = mbar->rect.p0;

	cur = ui_menu_first(mbar);
	while (cur != NULL) {
		caption = ui_menu_caption(cur);
		width = ui_text_width(res->font, caption) + 2 * hpad;

		rect.p0 = pos;
		rect.p1.x = rect.p0.x + width;
		rect.p1.y = mbar->rect.p1.y;

		if (cur == menu) {
			*rrect = rect;
			return;
		}

		pos.x += width;
		cur = ui_menu_next(cur);
	}

	/* We should never get here */
	assert(false);
}

/** Activate menu bar.
 *
 * @param mbar Menu bar
 */
void ui_menu_bar_activate(ui_menu_bar_t *mbar)
{
	if (mbar->active)
		return;

	mbar->active = true;
	if (mbar->selected == NULL)
		mbar->selected = ui_menu_first(mbar);

	(void) ui_menu_bar_paint(mbar);
}

void ui_menu_bar_deactivate(ui_menu_bar_t *mbar)
{
	ui_menu_bar_select(mbar, NULL, false);
	mbar->active = false;
}

/** Destroy menu bar control.
 *
 * @param arg Argument (ui_menu_bar_t *)
 */
void ui_menu_bar_ctl_destroy(void *arg)
{
	ui_menu_bar_t *mbar = (ui_menu_bar_t *) arg;

	ui_menu_bar_destroy(mbar);
}

/** Paint menu bar control.
 *
 * @param arg Argument (ui_menu_bar_t *)
 * @return EOK on success or an error code
 */
errno_t ui_menu_bar_ctl_paint(void *arg)
{
	ui_menu_bar_t *mbar = (ui_menu_bar_t *) arg;

	return ui_menu_bar_paint(mbar);
}

/** Handle menu bar control keyboard event.
 *
 * @param arg Argument (ui_menu_bar_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_bar_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_menu_bar_t *mbar = (ui_menu_bar_t *) arg;

	return ui_menu_bar_kbd_event(mbar, event);
}

/** Handle menu bar control position event.
 *
 * @param arg Argument (ui_menu_bar_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_bar_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_menu_bar_t *mbar = (ui_menu_bar_t *) arg;

	return ui_menu_bar_pos_event(mbar, event);
}

/** @}
 */
