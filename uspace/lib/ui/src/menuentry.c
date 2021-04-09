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
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/menuentry.h>
#include "../private/menubar.h"
#include "../private/menuentry.h"
#include "../private/menu.h"
#include "../private/resource.h"

enum {
	menu_entry_hpad = 4,
	menu_entry_vpad = 4,
	menu_entry_hpad_text = 1,
	menu_entry_vpad_text = 0
};

/** Create new menu entry.
 *
 * @param menu Menu
 * @param caption Caption
 * @param rmentry Place to store pointer to new menu entry
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_entry_create(ui_menu_t *menu, const char *caption,
    ui_menu_entry_t **rmentry)
{
	ui_menu_entry_t *mentry;
	gfx_coord_t width;

	mentry = calloc(1, sizeof(ui_menu_entry_t));
	if (mentry == NULL)
		return ENOMEM;

	mentry->caption = str_dup(caption);
	if (mentry->caption == NULL) {
		free(mentry);
		return ENOMEM;
	}

	mentry->menu = menu;
	list_append(&mentry->lentries, &menu->entries);

	/* Update accumulated menu entry dimensions */
	width = ui_menu_entry_width(mentry);
	if (width > menu->max_w)
		menu->max_w = width;
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

	list_remove(&mentry->lentries);
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

/** Get width of menu entry.
 *
 * @param mentry Menu entry
 * @return Width in pixels
 */
gfx_coord_t ui_menu_entry_width(ui_menu_entry_t *mentry)
{
	ui_resource_t *res;
	gfx_coord_t hpad;

	res = mentry->menu->mbar->res;

	if (res->textmode) {
		hpad = menu_entry_hpad_text;
	} else {
		hpad = menu_entry_hpad;
	}

	return gfx_text_width(res->font, mentry->caption) + 2 * hpad;
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

	res = mentry->menu->mbar->res;

	if (res->textmode) {
		vpad = menu_entry_vpad_text;
	} else {
		vpad = menu_entry_vpad;
	}

	gfx_font_get_metrics(res->font, &metrics);
	height = metrics.ascent + metrics.descent + 1;
	return height + 2 * vpad;
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
	gfx_text_fmt_t fmt;
	gfx_color_t *bg_color;
	const char *caption;
	ui_menu_entry_geom_t geom;
	errno_t rc;

	res = mentry->menu->mbar->res;

	ui_menu_entry_get_geom(mentry, pos, &geom);

	gfx_text_fmt_init(&fmt);
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	caption = mentry->caption;

	if ((mentry->held && mentry->inside) ||
	    mentry == mentry->menu->selected) {
		fmt.color = res->wnd_sel_text_color;
		bg_color = res->wnd_sel_text_bg_color;
	} else {
		fmt.color = res->wnd_text_color;
		bg_color = res->wnd_face_color;
	}

	rc = gfx_set_color(res->gc, bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &geom.outer_rect);
	if (rc != EOK)
		goto error;

	rc = gfx_puttext(res->font, &geom.text_pos, &fmt, caption);
	if (rc != EOK)
		goto error;

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
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

	if (mentry->inside) {
		/* Close menu */
		ui_menu_bar_select(mentry->menu->mbar,
		    &mentry->menu->mbar->sel_pos, NULL);

		/* Call back */
		ui_menu_entry_cb(mentry);
	}
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

	res = mentry->menu->mbar->res;

	if (res->textmode) {
		hpad = menu_entry_hpad_text;
		vpad = menu_entry_vpad_text;
	} else {
		hpad = menu_entry_hpad;
		vpad = menu_entry_vpad;
	}

	width = mentry->menu->max_w;
	geom->text_pos.x = pos->x + hpad;
	geom->text_pos.y = pos->y + vpad;

	geom->outer_rect.p0 = *pos;
	geom->outer_rect.p1.x = geom->outer_rect.p0.x + width;
	geom->outer_rect.p1.y = geom->outer_rect.p0.y +
	    ui_menu_entry_height(mentry);
}

/** @}
 */
