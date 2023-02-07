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
 * @file Tab
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
#include <uchar.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/popup.h>
#include <ui/tab.h>
#include <ui/tabset.h>
#include <ui/resource.h>
#include <ui/window.h>
#include "../private/control.h"
#include "../private/tab.h"
#include "../private/tabset.h"
#include "../private/resource.h"

enum {
	/** Horizontal margin before first tab handle */
	tab_start_hmargin = 6,
	/** Horizontal margin before first tab handle in text mode */
	tab_start_hmargin_text = 1,
	/** Tab handle horizontal internal padding */
	tab_handle_hpad = 6,
	/** Tab handle top internal padding */
	tab_handle_top_pad = 5,
	/** Tab handle bottom internal padding */
	tab_handle_bottom_pad = 5,
	/** Tab handle horizontal internal padding in text mode */
	tab_handle_hpad_text = 1,
	/** Tab handle top internal padding in text mode */
	tab_handle_top_pad_text = 0,
	/** Tab handle bototm internal padding in text mode */
	tab_handle_bottom_pad_text = 1,
	/** Tab handle chamfer */
	tab_handle_chamfer = 3,
	/** Number of pixels to pull active handle up by */
	tab_handle_pullup = 2,
	/** Tab frame horizontal thickness */
	tab_frame_w = 2,
	/** Tab frame vertical thickness */
	tab_frame_h = 2,
	/** Tab frame horizontal thickness in text mode */
	tab_frame_w_text = 1,
	/** Tab frame horizontal thickness in text mode */
	tab_frame_h_text = 1
};

/** Selected tab handle box characters */
static ui_box_chars_t sel_tab_box_chars = {
	{
		{ "\u250c", "\u2500", "\u2510" },
		{ "\u2502", " ",      "\u2502" },
		{ "\u2518", " ",      "\u2514" }
	}
};

/** Not selected tab handle box characters */
static ui_box_chars_t unsel_tab_box_chars = {
	{
		{ "\u250c", "\u2500", "\u2510" },
		{ "\u2502", " ",      "\u2502" },
		{ "\u2534", "\u2500", "\u2534" }
	}
};

/** Create new tab.
 *
 * @param tabset Tab set
 * @param caption Caption
 * @param rtab Place to store pointer to new tab
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_tab_create(ui_tab_set_t *tabset, const char *caption,
    ui_tab_t **rtab)
{
	ui_tab_t *tab;
	ui_tab_t *prev;

	tab = calloc(1, sizeof(ui_tab_t));
	if (tab == NULL)
		return ENOMEM;

	tab->caption = str_dup(caption);
	if (tab->caption == NULL) {
		free(tab);
		return ENOMEM;
	}

	prev = ui_tab_last(tabset);
	if (prev != NULL)
		tab->xoff = prev->xoff + ui_tab_handle_width(prev);
	else
		tab->xoff = tabset->res->textmode ?
		    tab_start_hmargin_text : tab_start_hmargin;

	tab->tabset = tabset;
	list_append(&tab->ltabs, &tabset->tabs);

	/* This is the first tab. Select it. */
	if (tabset->selected == NULL)
		tabset->selected = tab;

	*rtab = tab;
	return EOK;
}

/** Destroy tab.
 *
 * @param tab Tab or @c NULL
 */
void ui_tab_destroy(ui_tab_t *tab)
{
	if (tab == NULL)
		return;

	/* Destroy content */
	ui_control_destroy(tab->content);

	list_remove(&tab->ltabs);
	free(tab->caption);
	free(tab);
}

/** Get first tab in tab bar.
 *
 * @param tabset Tab set
 * @return First tab or @c NULL if there is none
 */
ui_tab_t *ui_tab_first(ui_tab_set_t *tabset)
{
	link_t *link;

	link = list_first(&tabset->tabs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_tab_t, ltabs);
}

/** Get next tab in tab bar.
 *
 * @param cur Current tab
 * @return Next tab or @c NULL if @a cur is the last one
 */
ui_tab_t *ui_tab_next(ui_tab_t *cur)
{
	link_t *link;

	link = list_next(&cur->ltabs, &cur->tabset->tabs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_tab_t, ltabs);
}

/** Get last tab in tab bar.
 *
 * @param tabset Tab set
 * @return Last tab or @c NULL if there is none
 */
ui_tab_t *ui_tab_last(ui_tab_set_t *tabset)
{
	link_t *link;

	link = list_last(&tabset->tabs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_tab_t, ltabs);
}

/** Get previous tab in tab bar.
 *
 * @param cur Current tab
 * @return Previous tab or @c NULL if @a cur is the fist one
 */
ui_tab_t *ui_tab_prev(ui_tab_t *cur)
{
	link_t *link;

	link = list_prev(&cur->ltabs, &cur->tabset->tabs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_tab_t, ltabs);
}

/** Determine if tab is selected.
 *
 * @param tab Tab
 * @return @c true iff tab is selected
 */
bool ui_tab_is_selected(ui_tab_t *tab)
{
	return tab->tabset->selected == tab;
}

/** Add control to tab.
 *
 * Only one control can be added to a window. If more than one control
 * is added, the results are undefined.
 *
 * @param tab Tab
 * @param control Control
 */
void ui_tab_add(ui_tab_t *tab, ui_control_t *control)
{
	assert(tab->content == NULL);

	tab->content = control;
	control->elemp = (void *) tab;
}

/** Remove control from tab.
 *
 * @param tab Tab
 * @param control Control
 */
void ui_tab_remove(ui_tab_t *tab, ui_control_t *control)
{
	assert(tab->content == control);
	assert((ui_tab_t *) control->elemp == tab);

	tab->content = NULL;
	control->elemp = NULL;
}

/** Get tab handle width.
 *
 * @param tab Tab
 * @return Handle width in pixels
 */
gfx_coord_t ui_tab_handle_width(ui_tab_t *tab)
{
	ui_resource_t *res;
	gfx_coord_t frame_w;
	gfx_coord_t handle_hpad;
	gfx_coord_t text_w;

	res = tab->tabset->res;
	if (!res->textmode) {
		frame_w = tab_frame_w;
		handle_hpad = tab_handle_hpad;
	} else {
		frame_w = tab_frame_w_text;
		handle_hpad = tab_handle_hpad_text;
	}

	text_w = ui_text_width(tab->tabset->res->font, tab->caption);
	return 2 * frame_w + 2 * handle_hpad + text_w;
}

/** Get tab handle height.
 *
 * @param tab Tab
 * @return Handle height in pixels
 */
gfx_coord_t ui_tab_handle_height(ui_tab_t *tab)
{
	gfx_coord_t frame_h;
	gfx_coord_t handle_top_pad;
	gfx_coord_t handle_bottom_pad;
	gfx_font_metrics_t metrics;
	ui_resource_t *res;

	res = tab->tabset->res;
	gfx_font_get_metrics(tab->tabset->res->font, &metrics);

	if (!res->textmode) {
		frame_h = tab_frame_h;
		handle_top_pad = tab_handle_top_pad;
		handle_bottom_pad = tab_handle_bottom_pad;
	} else {
		frame_h = tab_frame_h_text;
		handle_top_pad = tab_handle_top_pad_text;
		handle_bottom_pad = tab_handle_bottom_pad_text;
	}

	return frame_h + handle_top_pad + metrics.ascent +
	    metrics.descent + 1 + handle_bottom_pad;
}

/** Get tab geometry.
 *
 * @param tab Tab
 * @param geom Structure to fill in with computed geometry
 */
void ui_tab_get_geom(ui_tab_t *tab, ui_tab_geom_t *geom)
{
	gfx_coord_t handle_w;
	gfx_coord_t handle_h;
	gfx_coord_t pullup;
	gfx_coord_t frame_w;
	gfx_coord_t frame_h;
	gfx_coord_t handle_hpad;
	gfx_coord_t handle_top_pad;
	ui_resource_t *res;

	res = tab->tabset->res;

	handle_w = ui_tab_handle_width(tab);
	handle_h = ui_tab_handle_height(tab);
	pullup = res->textmode ? 0 : tab_handle_pullup;

	if (!res->textmode) {
		frame_w = tab_frame_w;
		frame_h = tab_frame_h;
		handle_hpad = tab_handle_hpad;
		handle_top_pad = tab_handle_top_pad;
	} else {
		frame_w = tab_frame_w_text;
		frame_h = tab_frame_h_text;
		handle_hpad = tab_handle_hpad_text;
		handle_top_pad = tab_handle_top_pad_text;
	}

	/* Entire handle area */
	geom->handle_area.p0.x = tab->tabset->rect.p0.x + tab->xoff;
	geom->handle_area.p0.y = tab->tabset->rect.p0.y;
	geom->handle_area.p1.x = geom->handle_area.p0.x + handle_w;
	geom->handle_area.p1.y = geom->handle_area.p0.y + handle_h + pullup;

	geom->handle = geom->handle_area;

	/* If handle is selected */
	if (!ui_tab_is_selected(tab)) {
		/* Push top of handle down a bit */
		geom->handle.p0.y += pullup;
		/* Do not paint background over tab body frame */
		geom->handle_area.p1.y -= pullup;
	}

	/* Caption text position */
	geom->text_pos.x = geom->handle.p0.x + frame_w + handle_hpad;
	geom->text_pos.y = geom->handle.p0.y + frame_h + handle_top_pad;

	/* Tab body */
	geom->body.p0.x = tab->tabset->rect.p0.x;
	geom->body.p0.y = tab->tabset->rect.p0.y + handle_h - frame_h +
	    pullup;
	geom->body.p1 = tab->tabset->rect.p1;
}

/** Get UI resource from tab.
 *
 * @param tab Tab
 * @return UI resource
 */
ui_resource_t *ui_tab_get_res(ui_tab_t *tab)
{
	return tab->tabset->res;
}

/** Paint tab handle frame.
 *
 * @param gc Graphic context
 * @param rect Rectangle
 * @param chamfer Chamfer
 * @param hi_color Highlight color
 * @param sh_color Shadow color
 * @param selected Tab is selected
 * @param irect Place to store interior rectangle
 * @return EOK on success or an error code
 */
errno_t ui_tab_paint_handle_frame(gfx_context_t *gc, gfx_rect_t *rect,
    gfx_coord_t chamfer, gfx_color_t *hi_color, gfx_color_t *sh_color,
    bool selected, gfx_rect_t *irect)
{
	gfx_rect_t r;
	gfx_coord_t i;
	errno_t rc;

	rc = gfx_set_color(gc, hi_color);
	if (rc != EOK)
		goto error;

	/* Left side */
	r.p0.x = rect->p0.x;
	r.p0.y = rect->p0.y + chamfer;
	r.p1.x = rect->p0.x + 1;
	r.p1.y = rect->p1.y - 2;
	rc = gfx_fill_rect(gc, &r);
	if (rc != EOK)
		goto error;

	/* Top-left chamfer */
	for (i = 1; i < chamfer; i++) {
		r.p0.x = rect->p0.x + i;
		r.p0.y = rect->p0.y + chamfer - i;
		r.p1.x = r.p0.x + 1;
		r.p1.y = r.p0.y + 1;
		rc = gfx_fill_rect(gc, &r);
		if (rc != EOK)
			goto error;
	}

	/* Top side */
	r.p0.x = rect->p0.x + chamfer;
	r.p0.y = rect->p0.y;
	r.p1.x = rect->p1.x - chamfer;
	r.p1.y = rect->p0.y + 1;
	rc = gfx_fill_rect(gc, &r);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(gc, sh_color);
	if (rc != EOK)
		goto error;

	/* Top-right chamfer */
	for (i = 1; i < chamfer; i++) {
		r.p0.x = rect->p1.x - 1 - i;
		r.p0.y = rect->p0.y + chamfer - i;
		r.p1.x = r.p0.x + 1;
		r.p1.y = r.p0.y + 1;
		rc = gfx_fill_rect(gc, &r);
		if (rc != EOK)
			goto error;
	}

	/* Right side */
	r.p0.x = rect->p1.x - 1;
	r.p0.y = rect->p0.y + chamfer;
	r.p1.x = rect->p1.x;
	r.p1.y = rect->p1.y - 2;
	rc = gfx_fill_rect(gc, &r);
	if (rc != EOK)
		goto error;

	irect->p0.x = rect->p0.x + 1;
	irect->p0.y = rect->p0.y + 1;
	irect->p1.x = rect->p1.x - 1;
	irect->p1.y = rect->p1.y;
	return EOK;
error:
	return rc;
}

/** Paint tab body frame.
 *
 * @param tab Tab
 * @return EOK on success or an error code
 */
errno_t ui_tab_paint_body_frame(ui_tab_t *tab)
{
	gfx_rect_t bg_rect;
	ui_tab_geom_t geom;
	ui_resource_t *res;
	errno_t rc;

	res = ui_tab_get_res(tab);
	ui_tab_get_geom(tab, &geom);

	if (!res->textmode) {
		rc = ui_paint_outset_frame(res, &geom.body, &bg_rect);
		if (rc != EOK)
			goto error;
	} else {
		rc = ui_paint_text_box(res, &geom.body, ui_box_single,
		    res->wnd_face_color);
		if (rc != EOK)
			goto error;

		bg_rect.p0.x = geom.body.p0.x + 1;
		bg_rect.p0.y = geom.body.p0.y + 1;
		bg_rect.p1.x = geom.body.p1.x - 1;
		bg_rect.p1.y = geom.body.p1.y - 1;
	}

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &bg_rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint tab frame.
 *
 * @param tab Tab
 * @return EOK on success or an error code
 */
errno_t ui_tab_paint_frame(ui_tab_t *tab)
{
	gfx_rect_t r0;
	ui_tab_geom_t geom;
	ui_resource_t *res;
	errno_t rc;

	res = ui_tab_get_res(tab);
	ui_tab_get_geom(tab, &geom);

	/* Paint handle background */

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &geom.handle_area);
	if (rc != EOK)
		goto error;

	/* Paint handle frame */
	if (!res->textmode) {
		rc = ui_tab_paint_handle_frame(res->gc, &geom.handle,
		    tab_handle_chamfer, res->wnd_frame_hi_color, res->wnd_frame_sh_color,
		    ui_tab_is_selected(tab), &r0);
		if (rc != EOK)
			goto error;

		rc = ui_tab_paint_handle_frame(res->gc, &r0, tab_handle_chamfer - 1,
		    res->wnd_highlight_color, res->wnd_shadow_color,
		    ui_tab_is_selected(tab), &r0);
		if (rc != EOK)
			goto error;
	} else {
		rc = ui_paint_text_box_custom(res, &geom.handle,
		    ui_tab_is_selected(tab) ? &sel_tab_box_chars :
		    &unsel_tab_box_chars, res->wnd_face_color);
		if (rc != EOK)
			goto error;
	}

	return EOK;
error:
	return rc;
}

/** Paint tab.
 *
 * @param tab Tab
 * @return EOK on success or an error code
 */
errno_t ui_tab_paint(ui_tab_t *tab)
{
	gfx_text_fmt_t fmt;
	ui_tab_geom_t geom;
	ui_resource_t *res;
	errno_t rc;

	res = ui_tab_get_res(tab);
	ui_tab_get_geom(tab, &geom);

	rc = ui_tab_paint_frame(tab);
	if (rc != EOK)
		goto error;

	/* Paint caption */

	gfx_text_fmt_init(&fmt);
	fmt.font = res->font;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;
	fmt.color = res->wnd_text_color;

	rc = gfx_puttext(&geom.text_pos, &fmt, tab->caption);
	if (rc != EOK)
		goto error;

	if (tab->content != NULL && ui_tab_is_selected(tab)) {
		/* Paint content */
		rc = ui_control_paint(tab->content);
		if (rc != EOK)
			goto error;
	}

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Handle position event in tab.
 *
 * @param tab Tab
 * @param event Position event
 * @return ui_claimed iff the event was claimed
 */
ui_evclaim_t ui_tab_pos_event(ui_tab_t *tab, pos_event_t *event)
{
	ui_tab_geom_t geom;
	gfx_coord2_t epos;

	ui_tab_get_geom(tab, &geom);
	epos.x = event->hpos;
	epos.y = event->vpos;

	/* Event inside tab handle? */
	if (gfx_pix_inside_rect(&epos, &geom.handle)) {
		/* Select tab? */
		if (event->type == POS_PRESS && event->btn_num == 1 &&
		    !ui_tab_is_selected(tab))
			ui_tab_set_select(tab->tabset, tab);

		/* Claim event */
		return ui_claimed;
	}

	/* Deliver event to content control, if any */
	if (ui_tab_is_selected(tab) && tab->content != NULL)
		return ui_control_pos_event(tab->content, event);

	return ui_unclaimed;
}

/** Handle keyboard event in tab.
 *
 * @param tab Tab
 * @param event Keyboard event
 * @return ui_claimed iff the event was claimed
 */
ui_evclaim_t ui_tab_kbd_event(ui_tab_t *tab, kbd_event_t *event)
{
	/* Deliver event to content control, if any */
	if (ui_tab_is_selected(tab) && tab->content != NULL)
		return ui_control_kbd_event(tab->content, event);

	return ui_unclaimed;
}

/** @}
 */
