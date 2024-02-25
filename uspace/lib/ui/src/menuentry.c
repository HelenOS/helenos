/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * @file Menu entry
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
#include <ui/accel.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include <ui/window.h>
#include "../private/menubar.h"
#include "../private/menuentry.h"
#include "../private/menu.h"
#include "../private/resource.h"

enum {
	menu_entry_hpad = 4,
	menu_entry_vpad = 4,
	menu_entry_column_pad = 8,
	menu_entry_sep_height = 2,
	menu_entry_hpad_text = 1,
	menu_entry_vpad_text = 0,
	menu_entry_column_pad_text = 2,
	menu_entry_sep_height_text = 1
};

/** Create new menu entry.
 *
 * @param menu Menu
 * @param caption Caption
 * @param shortcut Shotcut key(s) or empty string
 * @param rmentry Place to store pointer to new menu entry
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_entry_create(ui_menu_t *menu, const char *caption,
    const char *shortcut, ui_menu_entry_t **rmentry)
{
	ui_menu_entry_t *mentry;
	gfx_coord_t caption_w;
	gfx_coord_t shortcut_w;

	mentry = calloc(1, sizeof(ui_menu_entry_t));
	if (mentry == NULL)
		return ENOMEM;

	mentry->caption = str_dup(caption);
	if (mentry->caption == NULL) {
		free(mentry);
		return ENOMEM;
	}

	mentry->shortcut = str_dup(shortcut);
	if (mentry->caption == NULL) {
		free(mentry->caption);
		free(mentry);
		return ENOMEM;
	}

	mentry->menu = menu;
	list_append(&mentry->lentries, &menu->entries);

	/* Update accumulated menu entry dimensions */
	ui_menu_entry_column_widths(mentry, &caption_w, &shortcut_w);
	if (caption_w > menu->max_caption_w)
		menu->max_caption_w = caption_w;
	if (shortcut_w > menu->max_shortcut_w)
		menu->max_shortcut_w = shortcut_w;
	menu->total_h += ui_menu_entry_height(mentry);

	*rmentry = mentry;
	return EOK;
}

/** Create new separator menu entry.
 *
 * @param menu Menu
 * @param rmentry Place to store pointer to new menu entry
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_entry_sep_create(ui_menu_t *menu, ui_menu_entry_t **rmentry)
{
	ui_menu_entry_t *mentry;
	errno_t rc;

	rc = ui_menu_entry_create(menu, "", "", &mentry);
	if (rc != EOK)
		return rc;

	/* Need to adjust menu height when changing to separator */
	menu->total_h -= ui_menu_entry_height(mentry);
	mentry->separator = true;
	menu->total_h += ui_menu_entry_height(mentry);

	*rmentry = mentry;
	return EOK;
}

/** Destroy menu entry.
 *
 * @param mentry Menu entry or @c NULL
 */
void ui_menu_entry_destroy(ui_menu_entry_t *mentry)
{
	if (mentry == NULL)
		return;

	mentry->menu->total_h -= ui_menu_entry_height(mentry);
	/* NOTE: max_caption_w/max_shortcut_w not updated (speed) */

	list_remove(&mentry->lentries);

	/*
	 * If we emptied the menu, reset accumulated dims so they
	 * can be correctly calculated when (if) the menu is
	 * re-populated.
	 */
	if (list_empty(&mentry->menu->entries)) {
		mentry->menu->total_h = 0;
		mentry->menu->max_caption_w = 0;
		mentry->menu->max_shortcut_w = 0;
	}

	free(mentry->caption);
	free(mentry);
}

/** Set menu entry callback.
 *
 * @param mentry Menu entry
 * @param cb Menu entry callback
 * @param arg Callback argument
 */
void ui_menu_entry_set_cb(ui_menu_entry_t *mentry, ui_menu_entry_cb_t cb,
    void *arg)
{
	mentry->cb = cb;
	mentry->arg = arg;
}

/** Set menu entry disabled flag.
 *
 * @param mentry Menu entry
 * @param disabled @c true iff entry is to be disabled, @c false otherwise
 */
void ui_menu_entry_set_disabled(ui_menu_entry_t *mentry, bool disabled)
{
	mentry->disabled = disabled;
}

/** Get menu entry disabled flag.
 *
 * @param mentry Menu entry
 * @return disabled @c true iff entry is disabled, @c false otherwise
 */
bool ui_menu_entry_is_disabled(ui_menu_entry_t *mentry)
{
	return mentry->disabled;
}

/** Get first menu entry in menu.
 *
 * @param menu Menu
 * @return First menu entry or @c NULL if there is none
 */
ui_menu_entry_t *ui_menu_entry_first(ui_menu_t *menu)
{
	link_t *link;

	link = list_first(&menu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_entry_t, lentries);
}

/** Get last menu entry in menu.
 *
 * @param menu Menu
 * @return Last menu entry or @c NULL if there is none
 */
ui_menu_entry_t *ui_menu_entry_last(ui_menu_t *menu)
{
	link_t *link;

	link = list_last(&menu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_entry_t, lentries);
}

/** Get next menu entry in menu.
 *
 * @param cur Current menu entry
 * @return Next menu entry or @c NULL if @a cur is the last one
 */
ui_menu_entry_t *ui_menu_entry_next(ui_menu_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->menu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_entry_t, lentries);
}

/** Get previous menu entry in menu.
 *
 * @param cur Current menu entry
 * @return Next menu entry or @c NULL if @a cur is the last one
 */
ui_menu_entry_t *ui_menu_entry_prev(ui_menu_entry_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lentries, &cur->menu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_entry_t, lentries);
}

/** Get width of menu entry.
 *
 * @param mentry Menu entry
 * @param caption_w Place to store caption width
 * @param shortcut_w Place to store shortcut width
 */
void ui_menu_entry_column_widths(ui_menu_entry_t *mentry,
    gfx_coord_t *caption_w, gfx_coord_t *shortcut_w)
{
	ui_resource_t *res;

	/*
	 * This needs to work even if the menu is not open, so we cannot
	 * use the menu's resource, which is only created after the menu
	 * is open (and its window is created). Use the parent window's
	 * resource instead.
	 */
	res = ui_window_get_res(mentry->menu->parent);

	*caption_w = ui_text_width(res->font, mentry->caption);
	*shortcut_w = ui_text_width(res->font, mentry->shortcut);
}

/** Compute width of menu entry.
 *
 * @param menu Menu
 * @param caption_w Widht of caption text
 * @param shortcut_w Width of shortcut text
 * @return Width in pixels
 */
gfx_coord_t ui_menu_entry_calc_width(ui_menu_t *menu, gfx_coord_t caption_w,
    gfx_coord_t shortcut_w)
{
	ui_resource_t *res;
	gfx_coord_t hpad;
	gfx_coord_t width;

	/*
	 * This needs to work even if the menu is not open, so we cannot
	 * use the menu's resource, which is only created after the menu
	 * is open (and its window is created). Use the parent window's
	 * resource instead.
	 */
	res = ui_window_get_res(menu->parent);

	if (res->textmode)
		hpad = menu_entry_hpad_text;
	else
		hpad = menu_entry_hpad;

	width = caption_w + 2 * hpad;

	if (shortcut_w != 0) {
		if (res->textmode)
			width += menu_entry_column_pad_text;
		else
			width += menu_entry_column_pad;

		width += shortcut_w;
	}

	return width;
}

/** Get height of menu entry.
 *
 * @param mentry Menu entry
 * @return Width in pixels
 */
gfx_coord_t ui_menu_entry_height(ui_menu_entry_t *mentry)
{
	ui_resource_t *res;
	gfx_font_metrics_t metrics;
	gfx_coord_t height;
	gfx_coord_t vpad;

	/*
	 * This needs to work even if the menu is not open, so we cannot
	 * use the menu's resource, which is only created after the menu
	 * is open (and its window is created). Use the parent window's
	 * resource instead.
	 */
	res = ui_window_get_res(mentry->menu->parent);

	if (res->textmode) {
		vpad = menu_entry_vpad_text;
	} else {
		vpad = menu_entry_vpad;
	}

	if (mentry->separator) {
		/* Separator menu entry */
		if (res->textmode)
			height = menu_entry_sep_height_text;
		else
			height = menu_entry_sep_height;
	} else {
		/* Normal menu entry */
		gfx_font_get_metrics(res->font, &metrics);
		height = metrics.ascent + metrics.descent + 1;
	}

	return height + 2 * vpad;
}

/** Get menu entry accelerator character.
 *
 * @param mentry Menu entry
 * @return Accelerator character (lowercase) or the null character if
 *         the menu entry has no accelerator.
 */
char32_t ui_menu_entry_get_accel(ui_menu_entry_t *mentry)
{
	return ui_accel_get(mentry->caption);
}

/** Paint menu entry.
 *
 * @param mentry Menu entry
 * @param pos Position where to paint entry
 * @return EOK on success or an error code
 */
errno_t ui_menu_entry_paint(ui_menu_entry_t *mentry, gfx_coord2_t *pos)
{
	ui_resource_t *res;
	ui_text_fmt_t fmt;
	gfx_color_t *bg_color;
	ui_menu_entry_geom_t geom;
	gfx_rect_t rect;
	errno_t rc;

	res = ui_menu_get_res(mentry->menu);

	ui_menu_entry_get_geom(mentry, pos, &geom);

	ui_text_fmt_init(&fmt);
	fmt.font = res->font;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	if ((mentry->held && mentry->inside) ||
	    mentry == mentry->menu->selected) {
		fmt.color = res->wnd_sel_text_color;
		fmt.hgl_color = res->wnd_sel_text_hgl_color;
		bg_color = res->wnd_sel_text_bg_color;
	} else if (mentry->disabled) {
		fmt.color = res->wnd_dis_text_color;
		fmt.hgl_color = res->wnd_dis_text_color;
		bg_color = res->wnd_face_color;
	} else {
		fmt.color = res->wnd_text_color;
		fmt.hgl_color = res->wnd_text_hgl_color;
		bg_color = res->wnd_face_color;
	}

	rc = gfx_set_color(res->gc, bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &geom.outer_rect);
	if (rc != EOK)
		goto error;

	rc = ui_paint_text(&geom.caption_pos, &fmt, mentry->caption);
	if (rc != EOK)
		goto error;

	fmt.halign = gfx_halign_right;

	rc = ui_paint_text(&geom.shortcut_pos, &fmt, mentry->shortcut);
	if (rc != EOK)
		goto error;

	if (mentry->separator) {
		if (res->textmode) {
			rect = geom.outer_rect;
			rect.p0.x -= 1;
			rect.p1.x += 1;

			rc = ui_paint_text_hbrace(res, &rect, ui_box_single,
			    res->wnd_face_color);
			if (rc != EOK)
				goto error;
		} else {
			rect.p0 = geom.caption_pos;
			rect.p1.x = geom.shortcut_pos.x;
			rect.p1.y = rect.p0.y + 2;
			rc = ui_paint_bevel(res->gc, &rect, res->wnd_shadow_color,
			    res->wnd_highlight_color, 1, NULL);
			if (rc != EOK)
				goto error;
		}
	}

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Determine if entry is selectable.
 *
 * @return @c true iff entry is selectable
 */
bool ui_menu_entry_selectable(ui_menu_entry_t *mentry)
{
	return !mentry->separator;
}

/** Handle button press in menu entry.
 *
 * @param mentry Menu entry
 * @param pos Menu entry position
 */
void ui_menu_entry_press(ui_menu_entry_t *mentry, gfx_coord2_t *pos)
{
	if (mentry->held)
		return;

	if (mentry->separator || mentry->disabled)
		return;

	mentry->inside = true;
	mentry->held = true;
	ui_menu_entry_paint(mentry, pos);
}

/** Handle button release in menu entry.
 *
 * @param mentry Menu entry
 */
void ui_menu_entry_release(ui_menu_entry_t *mentry)
{
	if (!mentry->held)
		return;

	mentry->held = false;

	if (mentry->inside)
		ui_menu_entry_activate(mentry);
}

/** Activate menu entry.
 *
 * @param mentry Menu entry
 */
void ui_menu_entry_activate(ui_menu_entry_t *mentry)
{
	/* Close menu */
	ui_menu_close_req(mentry->menu);

	/* Call back */
	ui_menu_entry_cb(mentry);
}

/** Call menu entry callback.
 *
 * @param mentry Menu entry
 */
void ui_menu_entry_cb(ui_menu_entry_t *mentry)
{
	if (mentry->cb != NULL)
		mentry->cb(mentry, mentry->arg);
}

/** Pointer entered menu entry.
 *
 * @param mentry Menu entry
 * @param pos Menu entry position
 */
void ui_menu_entry_enter(ui_menu_entry_t *mentry, gfx_coord2_t *pos)
{
	if (mentry->inside)
		return;

	mentry->inside = true;
	if (mentry->held)
		(void) ui_menu_entry_paint(mentry, pos);
}

/** Pointer left menu entry.
 *
 * @param mentry Menu entry
 * @param pos Menu entry position
 */
void ui_menu_entry_leave(ui_menu_entry_t *mentry, gfx_coord2_t *pos)
{
	if (!mentry->inside)
		return;

	mentry->inside = false;
	if (mentry->held)
		(void) ui_menu_entry_paint(mentry, pos);
}

/** Handle menu entry position event.
 *
 * @param mentry Menu entry
 * @param pos Menu entry position (top-left corner)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_menu_entry_pos_event(ui_menu_entry_t *mentry,
    gfx_coord2_t *pos, pos_event_t *event)
{
	ui_menu_entry_geom_t geom;
	gfx_coord2_t ppos;
	bool inside;

	ppos.x = event->hpos;
	ppos.y = event->vpos;

	ui_menu_entry_get_geom(mentry, pos, &geom);
	inside = gfx_pix_inside_rect(&ppos, &geom.outer_rect);

	switch (event->type) {
	case POS_PRESS:
		if (inside) {
			ui_menu_entry_press(mentry, pos);
			return ui_claimed;
		}
		break;
	case POS_RELEASE:
		if (mentry->held) {
			ui_menu_entry_release(mentry);
			return ui_claimed;
		}
		break;
	case POS_UPDATE:
		if (inside && !mentry->inside) {
			ui_menu_entry_enter(mentry, pos);
			return ui_claimed;
		} else if (!inside && mentry->inside) {
			ui_menu_entry_leave(mentry, pos);
		}
		break;
	case POS_DCLICK:
		break;
	}

	return ui_unclaimed;
}

/** Get menu entry geometry.
 *
 * @param mentry Menu entry
 * @param spos Entry position
 * @param geom Structure to fill in with computed geometry
 */
void ui_menu_entry_get_geom(ui_menu_entry_t *mentry, gfx_coord2_t *pos,
    ui_menu_entry_geom_t *geom)
{
	ui_resource_t *res;
	gfx_coord_t hpad;
	gfx_coord_t vpad;
	gfx_coord_t width;

	res = ui_menu_get_res(mentry->menu);

	if (res->textmode) {
		hpad = menu_entry_hpad_text;
		vpad = menu_entry_vpad_text;
	} else {
		hpad = menu_entry_hpad;
		vpad = menu_entry_vpad;
	}

	/* Compute total width of menu entry */
	width = ui_menu_entry_calc_width(mentry->menu,
	    mentry->menu->max_caption_w, mentry->menu->max_shortcut_w);

	geom->outer_rect.p0 = *pos;
	geom->outer_rect.p1.x = geom->outer_rect.p0.x + width;
	geom->outer_rect.p1.y = geom->outer_rect.p0.y +
	    ui_menu_entry_height(mentry);

	geom->caption_pos.x = pos->x + hpad;
	geom->caption_pos.y = pos->y + vpad;

	geom->shortcut_pos.x = geom->outer_rect.p1.x - hpad;
	geom->shortcut_pos.y = pos->y + vpad;
}

/** @}
 */
