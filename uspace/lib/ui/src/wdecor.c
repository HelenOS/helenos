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
 * @file Window decoration
 */

#include <assert.h>
#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/paint.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include "../private/resource.h"
#include "../private/wdecor.h"

static void ui_wdecor_btn_min_clicked(ui_pbutton_t *, void *);
static errno_t ui_wdecor_btn_min_paint(ui_pbutton_t *, void *,
    gfx_coord2_t *);

static void ui_wdecor_btn_max_clicked(ui_pbutton_t *, void *);
static errno_t ui_wdecor_btn_max_paint(ui_pbutton_t *, void *,
    gfx_coord2_t *);

static void ui_wdecor_btn_close_clicked(ui_pbutton_t *, void *);
static errno_t ui_wdecor_btn_close_paint(ui_pbutton_t *, void *,
    gfx_coord2_t *);

static ui_pbutton_cb_t ui_wdecor_btn_min_cb = {
	.clicked = ui_wdecor_btn_min_clicked
};

static ui_pbutton_decor_ops_t ui_wdecor_btn_min_decor_ops = {
	.paint = ui_wdecor_btn_min_paint
};

static ui_pbutton_cb_t ui_wdecor_btn_max_cb = {
	.clicked = ui_wdecor_btn_max_clicked
};

static ui_pbutton_decor_ops_t ui_wdecor_btn_max_decor_ops = {
	.paint = ui_wdecor_btn_max_paint
};

static ui_pbutton_cb_t ui_wdecor_btn_close_cb = {
	.clicked = ui_wdecor_btn_close_clicked
};

static ui_pbutton_decor_ops_t ui_wdecor_btn_close_decor_ops = {
	.paint = ui_wdecor_btn_close_paint
};

enum {
	/** Width of corner drag area */
	wdecor_corner_w = 24,
	/** Height of corner drag area */
	wdecor_corner_h = 24,
	/** Window resizing edge witdth */
	wdecor_edge_w = 4,
	/** Window resizing edge height */
	wdecor_edge_h = 4,
	/** Window resizing edge witdth */
	wdecor_edge_w_text = 1,
	/** Window resizing edge height */
	wdecor_edge_h_text = 1,
	/** Title bar height */
	wdecor_tbar_h = 22,
	/** Window width */
	wdecor_frame_w = 4,
	/** Window frame width in text mode */
	wdecor_frame_w_text = 1,
	/** Window caption horizontal margin */
	wdecor_cap_hmargin = 4,
	/** Window caption horizontal margin in text mode */
	wdecor_cap_hmargin_text = 1,
	/** System menu handle width */
	wdecor_sysmenu_hdl_w = 20,
	/** System menu handle height */
	wdecor_sysmenu_hdl_h = 20,
	/** System menu handle width in text mode */
	wdecor_sysmenu_hdl_w_text = 3,
	/** System menu handle height in text mode */
	wdecor_sysmenu_hdl_h_text = 1,
	/** Close button cross leg length */
	wdecor_close_cross_n = 5,
	/** Close button cross pen width */
	wdecor_close_cross_w = 2,
	/** Close button cross pen height */
	wdecor_close_cross_h = 1,
	/** Minimize icon width */
	wdecor_min_w = 10,
	/** Minimize icon height */
	wdecor_min_h = 10,
	/** Maximize icon width */
	wdecor_max_w = 10,
	/** Maximize icon height */
	wdecor_max_h = 10,
	/** Unmaximize icon window width */
	wdecor_unmax_w = 8,
	/** Unmaximize icon window height */
	wdecor_unmax_h = 8,
	/** Unmaximize icon window horizontal distance */
	wdecor_unmax_dw = 4,
	/** Unmaximize icon window vertical distance */
	wdecor_unmax_dh = 4
};

/** Create new window decoration.
 *
 * @param resource UI resource
 * @param caption Window caption
 * @param style Style
 * @param rwdecor Place to store pointer to new window decoration
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_wdecor_create(ui_resource_t *resource, const char *caption,
    ui_wdecor_style_t style, ui_wdecor_t **rwdecor)
{
	ui_wdecor_t *wdecor;
	errno_t rc;

	wdecor = calloc(1, sizeof(ui_wdecor_t));
	if (wdecor == NULL)
		return ENOMEM;

	wdecor->caption = str_dup(caption);
	if (wdecor->caption == NULL) {
		free(wdecor);
		return ENOMEM;
	}

	if ((style & ui_wds_minimize_btn) != 0) {
		rc = ui_pbutton_create(resource, "_", &wdecor->btn_min);
		if (rc != EOK) {
			ui_wdecor_destroy(wdecor);
			return rc;
		}

		ui_pbutton_set_cb(wdecor->btn_min, &ui_wdecor_btn_min_cb,
		    (void *)wdecor);

		ui_pbutton_set_decor_ops(wdecor->btn_min,
		    &ui_wdecor_btn_min_decor_ops, (void *)wdecor);
	}

	if ((style & ui_wds_maximize_btn) != 0) {
		rc = ui_pbutton_create(resource, "^", &wdecor->btn_max);
		if (rc != EOK) {
			ui_wdecor_destroy(wdecor);
			return rc;
		}

		ui_pbutton_set_cb(wdecor->btn_max, &ui_wdecor_btn_max_cb,
		    (void *)wdecor);

		ui_pbutton_set_decor_ops(wdecor->btn_max,
		    &ui_wdecor_btn_max_decor_ops, (void *)wdecor);
	}

	if ((style & ui_wds_close_btn) != 0) {
		rc = ui_pbutton_create(resource, "X", &wdecor->btn_close);
		if (rc != EOK) {
			ui_wdecor_destroy(wdecor);
			return rc;
		}

		ui_pbutton_set_cb(wdecor->btn_close, &ui_wdecor_btn_close_cb,
		    (void *)wdecor);

		ui_pbutton_set_decor_ops(wdecor->btn_close,
		    &ui_wdecor_btn_close_decor_ops, (void *)wdecor);
	}

	wdecor->res = resource;
	wdecor->active = true;
	wdecor->style = style;
	*rwdecor = wdecor;
	return EOK;
}

/** Destroy window decoration.
 *
 * @param wdecor Window decoration or @c NULL
 */
void ui_wdecor_destroy(ui_wdecor_t *wdecor)
{
	if (wdecor == NULL)
		return;

	ui_pbutton_destroy(wdecor->btn_min);
	ui_pbutton_destroy(wdecor->btn_max);
	ui_pbutton_destroy(wdecor->btn_close);
	free(wdecor->caption);
	free(wdecor);
}

/** Set window decoration callbacks.
 *
 * @param wdecor Window decoration
 * @param cb Window decoration callbacks
 * @param arg Callback argument
 */
void ui_wdecor_set_cb(ui_wdecor_t *wdecor, ui_wdecor_cb_t *cb, void *arg)
{
	wdecor->cb = cb;
	wdecor->arg = arg;
}

/** Set window decoration rectangle.
 *
 * @param wdecor Window decoration
 * @param rect New window decoration rectangle
 */
void ui_wdecor_set_rect(ui_wdecor_t *wdecor, gfx_rect_t *rect)
{
	ui_wdecor_geom_t geom;

	wdecor->rect = *rect;

	ui_wdecor_get_geom(wdecor, &geom);

	if (wdecor->btn_min != NULL)
		ui_pbutton_set_rect(wdecor->btn_min, &geom.btn_min_rect);
	if (wdecor->btn_max != NULL)
		ui_pbutton_set_rect(wdecor->btn_max, &geom.btn_max_rect);
	if (wdecor->btn_close != NULL)
		ui_pbutton_set_rect(wdecor->btn_close, &geom.btn_close_rect);
}

/** Set active flag.
 *
 * Active window is the one receiving keyboard events.
 *
 * @param wdecor Window decoration
 * @param active @c true iff window is active
 */
void ui_wdecor_set_active(ui_wdecor_t *wdecor, bool active)
{
	wdecor->active = active;
}

/** Set maximized flag.
 *
 * Active window is the one receiving keyboard events.
 *
 * @param wdecor Window decoration
 * @param maximized @c true iff window is maximized
 */
void ui_wdecor_set_maximized(ui_wdecor_t *wdecor, bool maximized)
{
	wdecor->maximized = maximized;
}

/** Change caption.
 *
 * @param wdecor Window decoration
 * @param caption New caption
 *
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_set_caption(ui_wdecor_t *wdecor, const char *caption)
{
	char *cdup;

	cdup = str_dup(caption);
	if (cdup == NULL)
		return ENOMEM;

	free(wdecor->caption);
	wdecor->caption = cdup;

	ui_wdecor_paint(wdecor);
	return EOK;
}

/** Paint system menu handle in graphics mode.
 *
 * @param wdecor Window decoration
 * @param rect System menu handle rectangle
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_sysmenu_hdl_paint_gfx(ui_wdecor_t *wdecor, gfx_rect_t *rect)
{
	errno_t rc;
	gfx_rect_t mrect;
	gfx_rect_t inside;
	gfx_coord2_t center;

	rc = gfx_set_color(wdecor->res->gc, wdecor->sysmenu_hdl_active ?
	    wdecor->res->btn_shadow_color : wdecor->res->btn_face_color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(wdecor->res->gc, rect);
	if (rc != EOK)
		return rc;

	center.x = (rect->p0.x + rect->p1.x) / 2;
	center.y = (rect->p0.y + rect->p1.y) / 2;
	mrect.p0.x = center.x - 7;
	mrect.p0.y = center.y - 1;
	mrect.p1.x = center.x + 7;
	mrect.p1.y = center.y + 1 + 1;

	/* XXX Not really a bevel, just a frame */
	rc = ui_paint_bevel(wdecor->res->gc, &mrect,
	    wdecor->res->btn_text_color, wdecor->res->btn_text_color, 1,
	    &inside);
	if (rc != EOK)
		return rc;

	rc = gfx_set_color(wdecor->res->gc, wdecor->res->btn_highlight_color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(wdecor->res->gc, &inside);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint system menu handle in text mode.
 *
 * @param wdecor Window decoration
 * @param rect System menu handle rectangle
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_sysmenu_hdl_paint_text(ui_wdecor_t *wdecor, gfx_rect_t *rect)
{
	errno_t rc;
	gfx_text_fmt_t fmt;

	rc = gfx_set_color(wdecor->res->gc, wdecor->sysmenu_hdl_active ?
	    wdecor->res->btn_shadow_color : wdecor->res->btn_face_color);

	gfx_text_fmt_init(&fmt);
	fmt.font = wdecor->res->font;
	fmt.color = wdecor->sysmenu_hdl_active ?
	    wdecor->res->wnd_sel_text_color :
	    wdecor->res->tbar_act_text_color;
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	rc = gfx_puttext(&rect->p0, &fmt, "[\u2261]");
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint system menu handle.
 *
 * @param wdecor Window decoration
 * @param rect System menu handle rectangle
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_sysmenu_hdl_paint(ui_wdecor_t *wdecor, gfx_rect_t *rect)
{
	errno_t rc;

	if (wdecor->res->textmode)
		rc = ui_wdecor_sysmenu_hdl_paint_text(wdecor, rect);
	else
		rc = ui_wdecor_sysmenu_hdl_paint_gfx(wdecor, rect);

	return rc;
}

/** Set system menu handle active flag.
 *
 * @param wdecor Window decoration
 * @param active @c true iff handle should be active
 */
void ui_wdecor_sysmenu_hdl_set_active(ui_wdecor_t *wdecor, bool active)
{
	ui_wdecor_geom_t geom;

	wdecor->sysmenu_hdl_active = active;

	ui_wdecor_get_geom(wdecor, &geom);
	(void) ui_wdecor_sysmenu_hdl_paint(wdecor, &geom.sysmenu_hdl_rect);
	(void) gfx_update(wdecor->res->gc);
}

/** Paint window decoration.
 *
 * @param wdecor Window decoration
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_paint(ui_wdecor_t *wdecor)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_rect_t trect;
	gfx_rect_t text_rect;
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	ui_wdecor_geom_t geom;

	rect = wdecor->rect;
	ui_wdecor_get_geom(wdecor, &geom);

	if ((wdecor->style & ui_wds_frame) != 0) {

		if (wdecor->res->textmode != false) {
			rc = ui_paint_text_box(wdecor->res, &rect,
			    ui_box_double, wdecor->res->wnd_face_color);
			if (rc != EOK)
				return rc;
		} else {
			rc = ui_paint_outset_frame(wdecor->res, &rect,
			    &rect);
			if (rc != EOK)
				return rc;

			rc = ui_paint_bevel(wdecor->res->gc, &rect,
			    wdecor->res->wnd_face_color,
			    wdecor->res->wnd_face_color, 2, &rect);
			if (rc != EOK)
				return rc;
		}
	}

	if ((wdecor->style & ui_wds_titlebar) != 0) {
		trect = geom.title_bar_rect;

		if (wdecor->res->textmode == false) {
			rc = ui_paint_bevel(wdecor->res->gc, &trect,
			    wdecor->res->wnd_shadow_color,
			    wdecor->res->wnd_highlight_color, 1, &trect);
			if (rc != EOK)
				return rc;

			rc = gfx_set_color(wdecor->res->gc, wdecor->active ?
			    wdecor->res->tbar_act_bg_color :
			    wdecor->res->tbar_inact_bg_color);
			if (rc != EOK)
				return rc;

			rc = gfx_fill_rect(wdecor->res->gc, &trect);
			if (rc != EOK)
				return rc;
		}

		gfx_text_fmt_init(&fmt);
		fmt.font = wdecor->res->font;
		fmt.color = wdecor->active ?
		    wdecor->res->tbar_act_text_color :
		    wdecor->res->tbar_inact_text_color;
		fmt.halign = gfx_halign_center;
		fmt.valign = gfx_valign_center;
		fmt.abbreviate = true;
		fmt.width = geom.caption_rect.p1.x -
		    geom.caption_rect.p0.x;

		pos.x = (geom.caption_rect.p0.x + geom.caption_rect.p1.x) / 2;
		pos.y = (geom.caption_rect.p0.y + geom.caption_rect.p1.y) / 2;

		if (wdecor->res->textmode) {
			/* Make space around caption text */
			gfx_text_rect(&pos, &fmt, wdecor->caption, &text_rect);

			/* Only make space if caption is non-empty */
			if (text_rect.p0.x < text_rect.p1.x) {
				text_rect.p0.x -= 1;
				text_rect.p1.x += 1;
			}

			rc = gfx_set_color(wdecor->res->gc, wdecor->active ?
			    wdecor->res->tbar_act_bg_color :
			    wdecor->res->tbar_inact_bg_color);
			if (rc != EOK)
				return rc;

			rc = gfx_fill_rect(wdecor->res->gc, &text_rect);
			if (rc != EOK)
				return rc;
		}

		rc = gfx_puttext(&pos, &fmt, wdecor->caption);
		if (rc != EOK)
			return rc;

		if ((wdecor->style & ui_wds_sysmenu_hdl) != 0) {
			rc = ui_wdecor_sysmenu_hdl_paint(wdecor,
			    &geom.sysmenu_hdl_rect);
			if (rc != EOK)
				return rc;
		}

		if (wdecor->btn_min != NULL) {
			rc = ui_pbutton_paint(wdecor->btn_min);
			if (rc != EOK)
				return rc;
		}

		if (wdecor->btn_max != NULL) {
			rc = ui_pbutton_paint(wdecor->btn_max);
			if (rc != EOK)
				return rc;
		}

		if (wdecor->btn_close != NULL) {
			rc = ui_pbutton_paint(wdecor->btn_close);
			if (rc != EOK)
				return rc;
		}
	}

	rc = gfx_update(wdecor->res->gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Send decoration sysmenu open event.
 *
 * @param wdecor Window decoration
 * @param idev_id Input device ID
 */
void ui_wdecor_sysmenu_open(ui_wdecor_t *wdecor, sysarg_t idev_id)
{
	if (wdecor->cb != NULL && wdecor->cb->sysmenu_open != NULL)
		wdecor->cb->sysmenu_open(wdecor, wdecor->arg, idev_id);
}

/** Send decoration sysmenu left event.
 *
 * @param wdecor Window decoration
 * @param idev_id Input device ID
 */
void ui_wdecor_sysmenu_left(ui_wdecor_t *wdecor, sysarg_t idev_id)
{
	if (wdecor->cb != NULL && wdecor->cb->sysmenu_left != NULL)
		wdecor->cb->sysmenu_left(wdecor, wdecor->arg, idev_id);
}

/** Send decoration sysmenu right event.
 *
 * @param wdecor Window decoration
 * @param idev_id Input device ID
 */
void ui_wdecor_sysmenu_right(ui_wdecor_t *wdecor, sysarg_t idev_id)
{
	if (wdecor->cb != NULL && wdecor->cb->sysmenu_right != NULL)
		wdecor->cb->sysmenu_right(wdecor, wdecor->arg, idev_id);
}

/** Send decoration sysmenu accelerator event.
 *
 * @param wdecor Window decoration
 * @param c Accelerator character
 * @param idev_id Input device ID
 */
void ui_wdecor_sysmenu_accel(ui_wdecor_t *wdecor, char32_t c, sysarg_t idev_id)
{
	if (wdecor->cb != NULL && wdecor->cb->sysmenu_right != NULL)
		wdecor->cb->sysmenu_accel(wdecor, wdecor->arg, c, idev_id);
}

/** Send decoration minimize event.
 *
 * @param wdecor Window decoration
 */
void ui_wdecor_minimize(ui_wdecor_t *wdecor)
{
	if (wdecor->cb != NULL && wdecor->cb->minimize != NULL)
		wdecor->cb->minimize(wdecor, wdecor->arg);
}

/** Send decoration maximize event.
 *
 * @param wdecor Window decoration
 */
void ui_wdecor_maximize(ui_wdecor_t *wdecor)
{
	if (wdecor->cb != NULL && wdecor->cb->maximize != NULL)
		wdecor->cb->maximize(wdecor, wdecor->arg);
}

/** Send decoration unmaximize event.
 *
 * @param wdecor Window decoration
 */
void ui_wdecor_unmaximize(ui_wdecor_t *wdecor)
{
	if (wdecor->cb != NULL && wdecor->cb->unmaximize != NULL)
		wdecor->cb->unmaximize(wdecor, wdecor->arg);
}

/** Send decoration close event.
 *
 * @param wdecor Window decoration
 */
void ui_wdecor_close(ui_wdecor_t *wdecor)
{
	if (wdecor->cb != NULL && wdecor->cb->close != NULL)
		wdecor->cb->close(wdecor, wdecor->arg);
}

/** Send decoration move event.
 *
 * @param wdecor Window decoration
 * @param pos Position where the title bar was pressed
 * @param pos_id Positioning device ID
 */
void ui_wdecor_move(ui_wdecor_t *wdecor, gfx_coord2_t *pos, sysarg_t pos_id)
{
	if (wdecor->cb != NULL && wdecor->cb->move != NULL)
		wdecor->cb->move(wdecor, wdecor->arg, pos, pos_id);
}

/** Send decoration resize event.
 *
 * @param wdecor Window decoration
 * @param rsztype Resize type
 * @param pos Position where the button was pressed
 * @param pos_id Positioning device ID
 */
void ui_wdecor_resize(ui_wdecor_t *wdecor, ui_wdecor_rsztype_t rsztype,
    gfx_coord2_t *pos, sysarg_t pos_id)
{
	if (wdecor->cb != NULL && wdecor->cb->resize != NULL)
		wdecor->cb->resize(wdecor, wdecor->arg, rsztype, pos, pos_id);
}

/** Send cursor change event.
 *
 * @param wdecor Window decoration
 * @param cursor Cursor
 */
void ui_wdecor_set_cursor(ui_wdecor_t *wdecor, ui_stock_cursor_t cursor)
{
	if (wdecor->cb != NULL && wdecor->cb->set_cursor != NULL)
		wdecor->cb->set_cursor(wdecor, wdecor->arg, cursor);
}

/** Get window decoration geometry.
 *
 * @param wdecor Window decoration
 * @param geom Structure to fill in with computed geometry
 */
void ui_wdecor_get_geom(ui_wdecor_t *wdecor, ui_wdecor_geom_t *geom)
{
	gfx_coord_t frame_w;
	gfx_coord_t btn_x;
	gfx_coord_t btn_y;
	gfx_coord_t cap_hmargin;
	gfx_coord_t cap_x;
	gfx_coord_t hdl_x_off;
	gfx_coord_t hdl_y_off;
	gfx_coord_t hdl_w;
	gfx_coord_t hdl_h;

	/* Does window have a frame? */
	if ((wdecor->style & ui_wds_frame) != 0) {
		frame_w = wdecor->res->textmode ?
		    wdecor_frame_w_text : wdecor_frame_w;

		geom->interior_rect.p0.x = wdecor->rect.p0.x + frame_w;
		geom->interior_rect.p0.y = wdecor->rect.p0.y + frame_w;
		geom->interior_rect.p1.x = wdecor->rect.p1.x - frame_w;
		geom->interior_rect.p1.y = wdecor->rect.p1.y - frame_w;
	} else {
		geom->interior_rect = wdecor->rect;
	}

	/* Does window have a title bar? */
	if ((wdecor->style & ui_wds_titlebar) != 0) {
		if (wdecor->res->textmode) {
			geom->title_bar_rect.p0 = wdecor->rect.p0;
			geom->title_bar_rect.p1.x = wdecor->rect.p1.x;
			geom->title_bar_rect.p1.y = wdecor->rect.p0.y + 1;

			btn_x = geom->title_bar_rect.p1.x - 1;
			btn_y = geom->title_bar_rect.p0.y;
		} else {
			geom->title_bar_rect.p0 = geom->interior_rect.p0;
			geom->title_bar_rect.p1.x = geom->interior_rect.p1.x;
			geom->title_bar_rect.p1.y = geom->interior_rect.p0.y +
			    wdecor_tbar_h;

			btn_x = geom->title_bar_rect.p1.x - 1;
			btn_y = geom->title_bar_rect.p0.y + 1;
		}

		geom->app_area_rect.p0.x = geom->interior_rect.p0.x;
		geom->app_area_rect.p0.y = geom->title_bar_rect.p1.y;
		geom->app_area_rect.p1 = geom->interior_rect.p1;

	} else {
		geom->title_bar_rect.p0.x = 0;
		geom->title_bar_rect.p0.y = 0;
		geom->title_bar_rect.p1.x = 0;
		geom->title_bar_rect.p1.y = 0;

		geom->app_area_rect = geom->interior_rect;
		btn_x = 0;
		btn_y = 0;
	}

	/* Does window have a sysmenu handle? */
	if ((wdecor->style & ui_wds_sysmenu_hdl) != 0) {
		if (wdecor->res->textmode) {
			hdl_x_off = 2;
			hdl_y_off = 0;
			hdl_w = wdecor_sysmenu_hdl_w_text;
			hdl_h = wdecor_sysmenu_hdl_h_text;
		} else {
			hdl_x_off = 1;
			hdl_y_off = 1;
			hdl_w = wdecor_sysmenu_hdl_w;
			hdl_h = wdecor_sysmenu_hdl_h;
		}

		geom->sysmenu_hdl_rect.p0.x = geom->title_bar_rect.p0.x +
		    hdl_x_off;
		geom->sysmenu_hdl_rect.p0.y = geom->title_bar_rect.p0.y +
		    hdl_y_off;
		geom->sysmenu_hdl_rect.p1.x = geom->sysmenu_hdl_rect.p0.x +
		    hdl_w;
		geom->sysmenu_hdl_rect.p1.y = geom->sysmenu_hdl_rect.p0.y +
		    hdl_h;
		cap_x = hdl_w;
	} else {
		geom->sysmenu_hdl_rect.p0.x = 0;
		geom->sysmenu_hdl_rect.p0.y = 0;
		geom->sysmenu_hdl_rect.p1.x = 0;
		geom->sysmenu_hdl_rect.p1.y = 0;
		cap_x = 0;
	}

	/* Does window have a close button? */
	if ((wdecor->style & ui_wds_close_btn) != 0) {
		if (wdecor->res->textmode == false) {
			geom->btn_close_rect.p0.x = btn_x - 20;
			geom->btn_close_rect.p0.y = btn_y;
			geom->btn_close_rect.p1.x = btn_x;
			geom->btn_close_rect.p1.y = btn_y + 20;

			btn_x -= 20;
		} else {
			geom->btn_close_rect.p0.x = btn_x - 3;
			geom->btn_close_rect.p0.y = btn_y;
			geom->btn_close_rect.p1.x = btn_x;
			geom->btn_close_rect.p1.y = btn_y + 1;

			btn_x -= 3;
		}
	} else {
		geom->btn_close_rect.p0.x = 0;
		geom->btn_close_rect.p0.y = 0;
		geom->btn_close_rect.p1.x = 0;
		geom->btn_close_rect.p1.y = 0;
	}

	/* Does window have a (un)maximize button? */
	if ((wdecor->style & ui_wds_maximize_btn) != 0) {
		if (wdecor->res->textmode == false) {
			geom->btn_max_rect.p0.x = btn_x - 20;
			geom->btn_max_rect.p0.y = btn_y;
			geom->btn_max_rect.p1.x = btn_x;
			geom->btn_max_rect.p1.y = btn_y + 20;

			btn_x -= 20;
		} else {
			geom->btn_max_rect.p0.x = btn_x - 3;
			geom->btn_max_rect.p0.y = btn_y;
			geom->btn_max_rect.p1.x = btn_x;
			geom->btn_max_rect.p1.y = btn_y + 1;

			btn_x -= 3;
		}
	} else {
		geom->btn_max_rect.p0.x = 0;
		geom->btn_max_rect.p0.y = 0;
		geom->btn_max_rect.p1.x = 0;
		geom->btn_max_rect.p1.y = 0;
	}

	/* Does window have a minimize button? */
	if ((wdecor->style & ui_wds_minimize_btn) != 0) {
		if (wdecor->res->textmode == false) {
			geom->btn_min_rect.p0.x = btn_x - 20;
			geom->btn_min_rect.p0.y = btn_y;
			geom->btn_min_rect.p1.x = btn_x;
			geom->btn_min_rect.p1.y = btn_y + 20;

			btn_x -= 20;
		} else {
			geom->btn_min_rect.p0.x = btn_x - 3;
			geom->btn_min_rect.p0.y = btn_y;
			geom->btn_min_rect.p1.x = btn_x;
			geom->btn_min_rect.p1.y = btn_y + 1;

			btn_x -= 3;
		}
	} else {
		geom->btn_min_rect.p0.x = 0;
		geom->btn_min_rect.p0.y = 0;
		geom->btn_min_rect.p1.x = 0;
		geom->btn_min_rect.p1.y = 0;
	}

	if ((wdecor->style & ui_wds_titlebar) != 0) {
		if (wdecor->res->textmode == false)
			cap_hmargin = wdecor_cap_hmargin;
		else
			cap_hmargin = wdecor_cap_hmargin_text;

		geom->caption_rect.p0.x = geom->title_bar_rect.p0.x +
		    cap_hmargin + cap_x;
		geom->caption_rect.p0.y = geom->title_bar_rect.p0.y;
		geom->caption_rect.p1.x = btn_x - cap_hmargin;
		geom->caption_rect.p1.y = geom->title_bar_rect.p1.y;
	} else {
		geom->caption_rect.p0.x = 0;
		geom->caption_rect.p0.y = 0;
		geom->caption_rect.p1.x = 0;
		geom->caption_rect.p1.y = 0;
	}
}

/** Get outer rectangle from application area rectangle.
 *
 * Note that this needs to work just based on a UI, without having an actual
 * window decoration, since we need it in order to create the window
 * and its decoration.
 *
 * @param ui UI
 * @param style Decoration style
 * @param app Application area rectangle
 * @param rect Place to store (outer) window decoration rectangle
 */
void ui_wdecor_rect_from_app(ui_t *ui, ui_wdecor_style_t style,
    gfx_rect_t *app, gfx_rect_t *rect)
{
	bool textmode;
	gfx_coord_t edge_w, edge_h;
	*rect = *app;

	textmode = ui_is_textmode(ui);
	if (textmode) {
		edge_w = wdecor_edge_w_text;
		edge_h = wdecor_edge_h_text;
	} else {
		edge_w = wdecor_edge_w;
		edge_h = wdecor_edge_h;
	}

	if ((style & ui_wds_frame) != 0) {
		rect->p0.x -= edge_w;
		rect->p0.y -= edge_h;
		rect->p1.x += edge_w;
		rect->p1.y += edge_h;
	}

	if ((style & ui_wds_titlebar) != 0 && !textmode)
		rect->p0.y -= wdecor_tbar_h;
}

/** Application area rectangle from window rectangle.
 *
 * Note that this needs to work just based on a UI, without having an actual
 * window decoration, since we need it in process of resizing the window,
 * before it is actually resized.
 *
 * @param style Decoration style
 * @param rect Window decoration rectangle
 * @param app Place to store application area rectangle
 */
void ui_wdecor_app_from_rect(ui_wdecor_style_t style, gfx_rect_t *rect,
    gfx_rect_t *app)
{
	*app = *rect;

	if ((style & ui_wds_frame) != 0) {
		app->p0.x += wdecor_edge_w;
		app->p0.y += wdecor_edge_h;
		app->p1.x -= wdecor_edge_w;
		app->p1.y -= wdecor_edge_h;
	}

	if ((style & ui_wds_titlebar) != 0)
		app->p0.y += 22;
}

/** Get resize type for pointer at the specified position.
 *
 * @param wdecor Window decoration
 * @param pos Pointer position
 * @return Resize type
 */
ui_wdecor_rsztype_t ui_wdecor_get_rsztype(ui_wdecor_t *wdecor,
    gfx_coord2_t *pos)
{
	bool eleft, eright;
	bool etop, ebottom;
	bool edge;
	bool cleft, cright;
	bool ctop, cbottom;

	/* Window not resizable? */
	if ((wdecor->style & ui_wds_resizable) == 0)
		return ui_wr_none;

	/* Window is maximized? */
	if (wdecor->maximized)
		return ui_wr_none;

	/* Position not inside window? */
	if (!gfx_pix_inside_rect(pos, &wdecor->rect))
		return ui_wr_none;

	/* Position is within edge width from the outside */
	eleft = (pos->x < wdecor->rect.p0.x + wdecor_edge_w);
	eright = (pos->x >= wdecor->rect.p1.x - wdecor_edge_w);
	etop = (pos->y < wdecor->rect.p0.y + wdecor_edge_h);
	ebottom = (pos->y >= wdecor->rect.p1.y - wdecor_edge_h);

	/* Position is on one of the four edges */
	edge = eleft || eright || etop || ebottom;

	/* Position is within resize-corner distance from the outside */
	cleft = (pos->x < wdecor->rect.p0.x + wdecor_corner_w);
	cright = (pos->x >= wdecor->rect.p1.x - wdecor_corner_w);
	ctop = (pos->y < wdecor->rect.p0.y + wdecor_corner_h);
	cbottom = (pos->y >= wdecor->rect.p1.y - wdecor_corner_h);

	/* Top-left corner */
	if (edge && cleft && ctop)
		return ui_wr_top_left;

	/* Top-right corner */
	if (edge && cright && ctop)
		return ui_wr_top_right;

	/* Bottom-left corner */
	if (edge && cleft && cbottom)
		return ui_wr_bottom_left;

	/* Bottom-right corner */
	if (edge && cright && cbottom)
		return ui_wr_bottom_right;

	/* Left edge */
	if (eleft)
		return ui_wr_left;

	/* Right edge */
	if (eright)
		return ui_wr_right;

	/* Top edge */
	if (etop)
		return ui_wr_top;

	/* Bottom edge */
	if (ebottom)
		return ui_wr_bottom;

	return ui_wr_none;
}

/** Get stock cursor to use for the specified window resize type.
 *
 * The resize type must be valid, otherwise behavior is undefined.
 *
 * @param rsztype Resize type
 * @return Cursor to use for this resize type
 */
ui_stock_cursor_t ui_wdecor_cursor_from_rsztype(ui_wdecor_rsztype_t rsztype)
{
	switch (rsztype) {
	case ui_wr_none:
		return ui_curs_arrow;

	case ui_wr_top:
	case ui_wr_bottom:
		return ui_curs_size_ud;

	case ui_wr_left:
	case ui_wr_right:
		return ui_curs_size_lr;

	case ui_wr_top_left:
	case ui_wr_bottom_right:
		return ui_curs_size_uldr;

	case ui_wr_top_right:
	case ui_wr_bottom_left:
		return ui_curs_size_urdl;

	default:
		assert(false);
		return ui_curs_arrow;
	}
}

/** Handle window decoration keyboard event.
 *
 * @param wdecor Window decoration
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff event was claimed
 */
ui_evclaim_t ui_wdecor_kbd_event(ui_wdecor_t *wdecor, kbd_event_t *event)
{
	if (event->type == KEY_PRESS && (event->mods & (KM_CTRL | KM_ALT |
	    KM_SHIFT)) == 0) {
		if (event->key == KC_F10) {
			ui_wdecor_sysmenu_hdl_set_active(wdecor, true);
			ui_wdecor_sysmenu_open(wdecor, event->kbd_id);
			return ui_claimed;
		}
	}

	/* System menu handle events (if active) */
	if (event->type == KEY_PRESS && (event->mods & (KM_CTRL | KM_ALT |
	    KM_SHIFT)) == 0 && wdecor->sysmenu_hdl_active) {
		switch (event->key) {
		case KC_ESCAPE:
			ui_wdecor_sysmenu_hdl_set_active(wdecor, false);
			return ui_claimed;
		case KC_LEFT:
			ui_wdecor_sysmenu_left(wdecor, event->kbd_id);
			return ui_claimed;
		case KC_RIGHT:
			ui_wdecor_sysmenu_right(wdecor, event->kbd_id);
			return ui_claimed;
		case KC_DOWN:
			ui_wdecor_sysmenu_open(wdecor, event->kbd_id);
			return ui_claimed;
		default:
			break;
		}

		if (event->c != '\0') {
			/* Could be an accelerator key */
			ui_wdecor_sysmenu_accel(wdecor, event->c,
			    event->kbd_id);
		}
	}

	return ui_unclaimed;
}

/** Handle window frame position event.
 *
 * @param wdecor Window decoration
 * @param pos_event Position event
 */
void ui_wdecor_frame_pos_event(ui_wdecor_t *wdecor, pos_event_t *event)
{
	gfx_coord2_t pos;
	sysarg_t pos_id;
	ui_wdecor_rsztype_t rsztype;
	ui_stock_cursor_t cursor;

	pos.x = event->hpos;
	pos.y = event->vpos;
	pos_id = event->pos_id;

	/* Set appropriate resizing cursor, or set arrow cursor */

	rsztype = ui_wdecor_get_rsztype(wdecor, &pos);
	cursor = ui_wdecor_cursor_from_rsztype(rsztype);

	ui_wdecor_set_cursor(wdecor, cursor);

	/* Press on window border? */
	if (rsztype != ui_wr_none && event->type == POS_PRESS)
		ui_wdecor_resize(wdecor, rsztype, &pos, pos_id);
}

/** Handle window decoration position event.
 *
 * @param wdecor Window decoration
 * @param pos_event Position event
 * @return @c ui_claimed iff event was claimed
 */
ui_evclaim_t ui_wdecor_pos_event(ui_wdecor_t *wdecor, pos_event_t *event)
{
	gfx_coord2_t pos;
	sysarg_t pos_id;
	ui_wdecor_geom_t geom;
	ui_evclaim_t claim;

	pos.x = event->hpos;
	pos.y = event->vpos;
	pos_id = event->pos_id;

	ui_wdecor_get_geom(wdecor, &geom);

	if ((wdecor->style & ui_wds_titlebar) != 0 &&
	    (wdecor->style & ui_wds_sysmenu_hdl) != 0) {
		if (event->type == POS_PRESS &&
		    gfx_pix_inside_rect(&pos, &geom.sysmenu_hdl_rect)) {
			ui_wdecor_sysmenu_hdl_set_active(wdecor, true);
			ui_wdecor_sysmenu_open(wdecor, event->pos_id);
			return ui_claimed;
		}
	}

	if (wdecor->btn_min != NULL) {
		claim = ui_pbutton_pos_event(wdecor->btn_min, event);
		if (claim == ui_claimed)
			return ui_claimed;
	}

	if (wdecor->btn_max != NULL) {
		claim = ui_pbutton_pos_event(wdecor->btn_max, event);
		if (claim == ui_claimed)
			return ui_claimed;
	}

	if (wdecor->btn_close != NULL) {
		claim = ui_pbutton_pos_event(wdecor->btn_close, event);
		if (claim == ui_claimed)
			return ui_claimed;
	}

	ui_wdecor_frame_pos_event(wdecor, event);

	if ((wdecor->style & ui_wds_titlebar) != 0 && !wdecor->maximized) {
		if (event->type == POS_PRESS &&
		    gfx_pix_inside_rect(&pos, &geom.title_bar_rect)) {
			ui_wdecor_move(wdecor, &pos, pos_id);
			return ui_claimed;
		}
	}

	return ui_unclaimed;
}

/** Window decoration minimize button was clicked.
 *
 * @param pbutton Minimize button
 * @param arg Argument (ui_wdecor_t)
 */
static void ui_wdecor_btn_min_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *) arg;

	(void) pbutton;

	ui_wdecor_minimize(wdecor);
}

/** Window decoration (un)maximize button was clicked.
 *
 * @param pbutton (Un)maximize button
 * @param arg Argument (ui_wdecor_t)
 */
static void ui_wdecor_btn_max_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *) arg;

	(void) pbutton;

	if (wdecor->maximized)
		ui_wdecor_unmaximize(wdecor);
	else
		ui_wdecor_maximize(wdecor);
}

/** Paint minimize button decoration.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_wdecor_t *)
 * @param pos Center position
 */
static errno_t ui_wdecor_btn_min_paint(ui_pbutton_t *pbutton,
    void *arg, gfx_coord2_t *pos)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *)arg;
	errno_t rc;

	rc = ui_paint_minicon(wdecor->res, pos, wdecor_min_w,
	    wdecor_min_h);

	return rc;
}

/** Paint (un)maximize button decoration.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_wdecor_t *)
 * @param pos Center position
 */
static errno_t ui_wdecor_btn_max_paint(ui_pbutton_t *pbutton,
    void *arg, gfx_coord2_t *pos)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *)arg;
	errno_t rc;

	if (wdecor->maximized) {
		rc = ui_paint_unmaxicon(wdecor->res, pos, wdecor_unmax_w,
		    wdecor_unmax_h, wdecor_unmax_dw, wdecor_unmax_dh);
	} else {
		rc = ui_paint_maxicon(wdecor->res, pos, wdecor_max_w,
		    wdecor_max_h);
	}

	return rc;
}

/** Window decoration close button was clicked.
 *
 * @param pbutton Close button
 * @param arg Argument (ui_wdecor_t)
 */
static void ui_wdecor_btn_close_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *) arg;

	(void) pbutton;
	ui_wdecor_close(wdecor);
}

/** Paint close button decoration.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_wdecor_t *)
 * @param pos Center position
 */
static errno_t ui_wdecor_btn_close_paint(ui_pbutton_t *pbutton,
    void *arg, gfx_coord2_t *pos)
{
	ui_wdecor_t *wdecor = (ui_wdecor_t *)arg;
	gfx_coord2_t p;
	errno_t rc;

	rc = gfx_set_color(wdecor->res->gc, wdecor->res->btn_text_color);
	if (rc != EOK)
		return rc;

	p.x = pos->x - 1;
	p.y = pos->y - 1;
	return ui_paint_cross(wdecor->res->gc, &p, wdecor_close_cross_n,
	    wdecor_close_cross_w, wdecor_close_cross_h);
}

/** @}
 */
