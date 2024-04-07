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

/** @addtogroup fontedit
 * @{
 */
/** @file Font editor
 */

#include <fibril.h>
#include <gfx/color.h>
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <gfx/typeface.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "fontedit.h"

enum {
	glyph_scale = 8,
	glyph_orig_x = 100,
	glyph_orig_y = 200
};

static errno_t font_edit_paint(font_edit_t *);

static void font_edit_close_event(ui_window_t *, void *);
static void font_edit_kbd_event(ui_window_t *, void *, kbd_event_t *);
static void font_edit_pos_event(ui_window_t *, void *, pos_event_t *);

static ui_window_cb_t font_edit_window_cb = {
	.close = font_edit_close_event,
	.kbd = font_edit_kbd_event,
	.pos = font_edit_pos_event
};

/** Clear screen.
 *
 * @param gc Graphic context
 * @param w Screen width
 * @param h Screen height
 */
static errno_t clear_scr(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	errno_t rc;

	rc = gfx_color_new_rgb_i16(0, 0, 0, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(gc, color);
	if (rc != EOK)
		goto error;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = w;
	rect.p1.y = h;

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);
	return EOK;
error:
	if (color != NULL)
		gfx_color_delete(color);
	return rc;
}

/** Adjust advance of the current glyph.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_advance(font_edit_t *fedit, gfx_coord_t change)
{
	gfx_glyph_metrics_t gmetrics;

	gfx_glyph_get_metrics(fedit->glyph, &gmetrics);
	gmetrics.advance += change;
	(void) gfx_glyph_set_metrics(fedit->glyph, &gmetrics);

	font_edit_paint(fedit);
}

/** Adjust font ascent.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_ascent(font_edit_t *fedit, gfx_coord_t change)
{
	gfx_font_metrics_t fmetrics;

	gfx_font_get_metrics(fedit->font, &fmetrics);
	fmetrics.ascent += change;
	(void) gfx_font_set_metrics(fedit->font, &fmetrics);

	printf("New ascent: %d\n", fmetrics.ascent);
	font_edit_paint(fedit);
}

/** Adjust font descent.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_descent(font_edit_t *fedit, gfx_coord_t change)
{
	gfx_font_metrics_t fmetrics;

	gfx_font_get_metrics(fedit->font, &fmetrics);
	fmetrics.descent += change;
	(void) gfx_font_set_metrics(fedit->font, &fmetrics);

	printf("New descent: %d\n", fmetrics.descent);
	font_edit_paint(fedit);
}

/** Adjust font leading.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_leading(font_edit_t *fedit, gfx_coord_t change)
{
	gfx_font_metrics_t fmetrics;

	gfx_font_get_metrics(fedit->font, &fmetrics);
	fmetrics.leading += change;
	(void) gfx_font_set_metrics(fedit->font, &fmetrics);

	printf("New leading: %d\n", fmetrics.leading);
	font_edit_paint(fedit);
}

/** Adjust font underline Y0.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_underline_y0(font_edit_t *fedit,
    gfx_coord_t change)
{
	gfx_font_metrics_t fmetrics;

	gfx_font_get_metrics(fedit->font, &fmetrics);
	fmetrics.underline_y0 += change;
	(void) gfx_font_set_metrics(fedit->font, &fmetrics);

	printf("New underline Y0: %d\n", fmetrics.underline_y0);
	font_edit_paint(fedit);
}

/** Adjust font underline Y1.
 *
 * @param fedit Font editor
 */
static void font_edit_adjust_underline_y1(font_edit_t *fedit,
    gfx_coord_t change)
{
	gfx_font_metrics_t fmetrics;

	gfx_font_get_metrics(fedit->font, &fmetrics);
	fmetrics.underline_y1 += change;
	(void) gfx_font_set_metrics(fedit->font, &fmetrics);

	printf("New underline Y1: %d\n", fmetrics.underline_y1);
	font_edit_paint(fedit);
}

/** Handle font editor close event.
 *
 * @param window Window
 * @param arg Argument (font_edit_t *)
 */
static void font_edit_close_event(ui_window_t *window, void *arg)
{
	font_edit_t *fedit = (font_edit_t *) arg;

	ui_quit(fedit->ui);
}

/** Handle font editor position event.
 *
 * @param window Window
 * @param arg Argument (font_edit_t *)
 * @param event Position event
 */
static void font_edit_pos_event(ui_window_t *window, void *arg,
    pos_event_t *event)
{
	font_edit_t *fedit = (font_edit_t *) arg;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	int x, y;

	ui_window_get_app_rect(window, &rect);

	pos.x = event->hpos;
	pos.y = event->vpos;

	if (event->type != POS_PRESS)
		return;

	if (!gfx_pix_inside_rect(&pos, &rect))
		return;

	x = gfx_coord_div_rneg(pos.x - glyph_orig_x -
	    rect.p0.x, glyph_scale);
	y = gfx_coord_div_rneg(pos.y - glyph_orig_y -
	    rect.p0.y, glyph_scale);

	printf("x=%d y=%d\n", x, y);
	gfx_glyph_bmp_setpix(fedit->gbmp, x, y, fedit->pen_color);
	font_edit_paint(fedit);
}

/** Duplicate previously selected glyph to the current glyph.
 *
 * @param fedit Font editor
 */
static void font_edit_copy_paste(font_edit_t *fedit)
{
	gfx_glyph_bmp_t *src_bmp;
	gfx_glyph_metrics_t gmetrics;
	gfx_rect_t rect;
	gfx_coord_t x, y;
	int pix;
	errno_t rc;

	/* If source and destination are the same, there is nothing to do. */
	if (fedit->glyph == fedit->src_glyph)
		return;

	rc = gfx_glyph_bmp_open(fedit->src_glyph, &src_bmp);
	if (rc != EOK) {
		printf("Error opening source glyph.\n");
		return;
	}

	gfx_glyph_bmp_get_rect(src_bmp, &rect);

	rc = gfx_glyph_bmp_clear(fedit->gbmp);
	if (rc != EOK) {
		printf("Error clearing glyph bitmap.\n");
		return;
	}

	for (y = rect.p0.y; y < rect.p1.y; y++) {
		for (x = rect.p0.x; x < rect.p1.x; x++) {
			pix = gfx_glyph_bmp_getpix(src_bmp, x, y);
			rc = gfx_glyph_bmp_setpix(fedit->gbmp, x, y, pix);
			if (rc != EOK) {
				printf("Error setting pixel.\n");
				return;
			}
		}
	}

	/* Copy metrics over */
	gfx_glyph_get_metrics(fedit->src_glyph, &gmetrics);
	(void) gfx_glyph_set_metrics(fedit->glyph, &gmetrics);

	font_edit_paint(fedit);

}

/** Handle font editor control-key press.
 *
 * @param widget Canvas widget
 * @param data Position event
 */
static void font_edit_ctrl_key(font_edit_t *fedit, kbd_event_t *event)
{
	errno_t rc;

	switch (event->key) {
	case KC_S:
		printf("Save!\n");
		(void) gfx_glyph_bmp_save(fedit->gbmp);
		rc = gfx_typeface_save(fedit->typeface, fedit->fname);
		if (rc != EOK)
			printf("Error saving typeface.\n");
		font_edit_paint(fedit);
		break;
	case KC_1:
		printf("Set pixels\n");
		fedit->pen_color = 1;
		break;
	case KC_2:
		printf("Clear pixels\n");
		fedit->pen_color = 0;
		break;
	case KC_3:
		font_edit_adjust_advance(fedit, -1);
		break;
	case KC_4:
		font_edit_adjust_advance(fedit, +1);
		break;
	case KC_5:
		font_edit_adjust_ascent(fedit, -1);
		break;
	case KC_6:
		font_edit_adjust_ascent(fedit, +1);
		break;
	case KC_7:
		font_edit_adjust_descent(fedit, -1);
		break;
	case KC_8:
		font_edit_adjust_descent(fedit, +1);
		break;
	case KC_9:
		font_edit_adjust_leading(fedit, -1);
		break;
	case KC_0:
		font_edit_adjust_leading(fedit, +1);
		break;
	case KC_U:
		font_edit_adjust_underline_y0(fedit, -1);
		break;
	case KC_I:
		font_edit_adjust_underline_y0(fedit, +1);
		break;
	case KC_O:
		font_edit_adjust_underline_y1(fedit, -1);
		break;
	case KC_P:
		font_edit_adjust_underline_y1(fedit, +1);
		break;
	case KC_X:
		(void) gfx_glyph_bmp_clear(fedit->gbmp);
		font_edit_paint(fedit);
		break;
	case KC_C:
		/* Select source glyph for copying */
		fedit->src_glyph = fedit->glyph;
		break;
	case KC_V:
		/* Duplicate another glyph */
		font_edit_copy_paste(fedit);
		break;
	default:
		break;
	}
}

/** Handle font editor unmodified key press.
 *
 * @param widget Canvas widget
 * @param data Position event
 */
static void font_edit_unmod_key(font_edit_t *fedit, kbd_event_t *event)
{
	char str[5];
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_glyph_bmp_t *bmp;
	size_t stradv;
	errno_t rc;

	if (event->c == '\0')
		return;

	printf("Character '%lc'\n", event->c);
	snprintf(str, sizeof(str), "%lc", event->c);

	rc = gfx_font_search_glyph(fedit->font, str, &glyph, &stradv);
	if (rc == EOK) {
		/* Found an existing glyph */
		rc = gfx_glyph_bmp_open(glyph, &bmp);
		if (rc != EOK) {
			printf("Error opening glyph bitmap.\n");
			return;
		}

		gfx_glyph_bmp_close(fedit->gbmp);
		fedit->glyph = glyph;
		fedit->gbmp = bmp;
		font_edit_paint(fedit);
		return;
	}

	/* Create new glyph */

	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(fedit->font, &gmetrics, &glyph);
	if (rc != EOK) {
		printf("Error creating glyph.\n");
		goto error;
	}

	rc = gfx_glyph_set_pattern(glyph, str);
	if (rc != EOK) {
		printf("Error setting glyph pattern.\n");
		goto error;
	}

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	if (rc != EOK) {
		printf("Error opening glyph bitmap.\n");
		goto error;
	}

	gfx_glyph_bmp_close(fedit->gbmp);
	fedit->glyph = glyph;
	fedit->gbmp = bmp;
	font_edit_paint(fedit);
	return;
error:
	if (glyph != NULL)
		gfx_glyph_destroy(glyph);
	return;
}

/** Handle font editor keyboard event.
 *
 * @param window Window
 * @param arg Argument (font_edit_t *)
 * @param event Keyboard event
 */
static void font_edit_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	font_edit_t *fedit = (font_edit_t *) arg;

	if (event->type != KEY_PRESS)
		return;

	if ((event->mods & KM_CTRL) != 0 &&
	    (event->mods & (KM_ALT | KM_SHIFT)) == 0) {
		font_edit_ctrl_key(fedit, event);
	} else if ((event->mods & (KM_CTRL | KM_ALT)) == 0) {
		font_edit_unmod_key(fedit, event);
	}
}

/** Convert glyph pixel coordinates to displayed rectangle.
 *
 * Since we upscale the glyph a pixel in the glyph corresponds to a rectangle
 * on the screen.
 *
 * @param fedit Font editor
 * @param x X coordinate in glyph
 * @param y Y coordinate in glyph
 * @param drect Place to store displayed rectangle coordinates
 */
static void font_edit_gpix_to_disp(font_edit_t *fedit, int x, int y,
    gfx_rect_t *drect)
{
	(void) fedit;

	drect->p0.x = glyph_orig_x + x * glyph_scale;
	drect->p0.y = glyph_orig_y + y * glyph_scale;
	drect->p1.x = glyph_orig_x + (x + 1) * glyph_scale;
	drect->p1.y = glyph_orig_y + (y + 1) * glyph_scale;
}

/** Paint font preview string.
 *
 * @param fedit Font editor
 * @param x Starting X coordinate
 * @param y Starting Y coordinate
 * @param color Color
 * @param str String
 */
static errno_t font_edit_paint_preview_str(font_edit_t *fedit,
    gfx_coord_t x, gfx_coord_t y, gfx_color_t *color, const char *str)
{
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;

	gfx_text_fmt_init(&fmt);
	fmt.font = fedit->font;
	fmt.color = color;

	pos.x = x;
	pos.y = y;

	return gfx_puttext(&pos, &fmt, str);
}

/** Paint font preview.
 *
 * @param fedit Font editor
 */
static errno_t font_edit_paint_preview(font_edit_t *fedit)
{
	gfx_color_t *color;
	errno_t rc;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		return rc;

	rc = gfx_set_color(fedit->gc, color);
	if (rc != EOK)
		goto error;

	rc = font_edit_paint_preview_str(fedit, 20, 20, color,
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	if (rc != EOK)
		goto error;

	rc = font_edit_paint_preview_str(fedit, 20, 40, color,
	    "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
	if (rc != EOK)
		goto error;

	rc = font_edit_paint_preview_str(fedit, 20, 60, color,
	    "abcdefghijklmnopqrstuvwxyz");
	if (rc != EOK)
		goto error;

	rc = font_edit_paint_preview_str(fedit, 20, 80, color,
	    "the quick brown fox jumps over the lazy dog");
	if (rc != EOK)
		goto error;

	rc = font_edit_paint_preview_str(fedit, 20, 100, color,
	    "0123456789,./<>?;'\\:\"|[]{}`~!@#$%^&*()-_=+");
	if (rc != EOK)
		goto error;

	return EOK;
error:
	gfx_color_delete(color);
	return rc;
}

/** Paint glyph bitmap.
 *
 * @param fedit Font editor
 */
static errno_t font_edit_paint_gbmp(font_edit_t *fedit)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	gfx_rect_t rect2;
	gfx_rect_t grect;
	gfx_font_metrics_t fmetrics;
	gfx_glyph_metrics_t gmetrics;
	errno_t rc;
	int x, y;
	int pix;

	/* Display font baseline, ascent, descent, leading */

	gfx_font_get_metrics(fedit->font, &fmetrics);

	rc = gfx_color_new_rgb_i16(0, 0x4000, 0x4000, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(fedit->gc, color);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, 0, &rect);
	rect.p1.x += 100;

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, -fmetrics.ascent, &rect);
	rect.p1.x += 100;

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, fmetrics.descent, &rect);
	rect.p1.x += 100;

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, fmetrics.descent +
	    fmetrics.leading, &rect);
	rect.p1.x += 100;

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	/* Display underline */

	rc = gfx_color_new_rgb_i16(0x4000, 0x4000, 0, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(fedit->gc, color);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, fmetrics.underline_y0, &rect);
	font_edit_gpix_to_disp(fedit, 10, fmetrics.underline_y1, &rect2);
	rect.p1 = rect2.p0;

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	/* Display glyph */

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(fedit->gc, color);
	if (rc != EOK)
		goto error;

	gfx_glyph_bmp_get_rect(fedit->gbmp, &grect);
	printf("grect=%d,%d,%d,%d\n", grect.p0.x, grect.p0.y,
	    grect.p1.x, grect.p1.y);

	for (y = grect.p0.y; y < grect.p1.y; y++) {
		for (x = grect.p0.x; x < grect.p1.x; x++) {
			pix = gfx_glyph_bmp_getpix(fedit->gbmp, x, y);

			if (pix != 0) {
				font_edit_gpix_to_disp(fedit, x, y, &rect);

				rc = gfx_fill_rect(fedit->gc, &rect);
				if (rc != EOK)
					goto error;
			}
		}
	}

	gfx_color_delete(color);

	/* Display glyph origin and advance */

	rc = gfx_color_new_rgb_i16(0, 0xffff, 0, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(fedit->gc, color);
	if (rc != EOK)
		goto error;

	font_edit_gpix_to_disp(fedit, 0, 0, &rect);

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_glyph_get_metrics(fedit->glyph, &gmetrics);

	font_edit_gpix_to_disp(fedit, gmetrics.advance, 0, &rect);

	rc = gfx_fill_rect(fedit->gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	return EOK;
error:
	if (color != NULL)
		gfx_color_delete(color);
	return rc;
}

/** Paint font editor.
 *
 * @param fedit Font editor
 */
static errno_t font_edit_paint(font_edit_t *fedit)
{
	int w, h;
	errno_t rc;

	w = fedit->width;
	h = fedit->height;

	rc = clear_scr(fedit->gc, w, h);
	if (rc != EOK)
		return rc;

	rc = font_edit_paint_gbmp(fedit);
	if (rc != EOK)
		return rc;

	rc = font_edit_paint_preview(fedit);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Create font editor.
 *
 * @param display_spec Display specifier
 * @param fname Font file to open or @c NULL to create new font
 * @param rfedit Place to store pointer to new font editor
 * @return EOK on success or an error code
 */
static errno_t font_edit_create(const char *display_spec, const char *fname,
    font_edit_t **rfedit)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	ui_window_t *window = NULL;
	gfx_context_t *gc = NULL;
	font_edit_t *fedit = NULL;
	gfx_typeface_t *tface = NULL;
	gfx_font_t *font = NULL;
	gfx_font_info_t *finfo;
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_glyph_bmp_t *bmp;
	gfx_coord_t vw, vh;
	errno_t rc;

	fedit = calloc(1, sizeof(font_edit_t));
	if (fedit == NULL) {
		rc = ENOMEM;
		goto error;
	}

	printf("Init UI..\n");

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error initializing UI (%s)\n", display_spec);
		goto error;
	}

	vw = 400;
	vh = 300;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = vw;
	rect.p1.y = vh;

	ui_wnd_params_init(&params);
	params.caption = "Font Editor";

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(ui, params.style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &params.rect);

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &font_edit_window_cb, (void *) fedit);

	rc = ui_window_get_app_gc(window, &gc);
	if (rc != EOK) {
		printf("Error creating graphic context.\n");
		goto error;
	}

	if (fname == NULL) {
		rc = gfx_typeface_create(gc, &tface);
		if (rc != EOK) {
			printf("Error creating typeface.\n");
			goto error;
		}

		gfx_font_props_init(&props);
		gfx_font_metrics_init(&metrics);

		rc = gfx_font_create(tface, &props, &metrics, &font);
		if (rc != EOK) {
			printf("Error creating font.\n");
			goto error;
		}

		gfx_glyph_metrics_init(&gmetrics);

		rc = gfx_glyph_create(font, &gmetrics, &glyph);
		if (rc != EOK) {
			printf("Error creating glyph.\n");
			goto error;
		}

		rc = gfx_glyph_set_pattern(glyph, "A");
		if (rc != EOK) {
			printf("Error setting glyph pattern.\n");
			goto error;
		}
	} else {
		rc = gfx_typeface_open(gc, fname, &tface);
		if (rc != EOK) {
			printf("Error opening typeface '%s.\n",
			    fname);
			goto error;
		}

		finfo = gfx_typeface_first_font(tface);
		rc = gfx_font_open(finfo, &font);
		if (rc != EOK) {
			printf("Error opening font.\n");
			goto error;
		}

		glyph = gfx_font_first_glyph(font);
	}

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	if (rc != EOK) {
		printf("Error opening glyph bitmap.\n");
		goto error;
	}

	if (fname == NULL)
		fname = "new.tpf";

	fedit->ui = ui;
	fedit->window = window;
	fedit->gc = gc;
	fedit->width = vw;
	fedit->height = vh;
	fedit->pen_color = 1;
	fedit->fname = fname;
	fedit->typeface = tface;
	fedit->font = font;
	fedit->glyph = glyph;
	fedit->gbmp = bmp;

	*rfedit = fedit;
	return EOK;
error:
	/*
	 * Once the window is created it would be probably more correct
	 * to leave destruction of these resources to a window destroy
	 * handler (which we have no way of registering)
	 */
	if (bmp != NULL)
		gfx_glyph_bmp_close(bmp);
	if (glyph != NULL)
		gfx_glyph_destroy(glyph);
	if (font != NULL)
		gfx_font_close(font);
	if (tface != NULL)
		gfx_typeface_destroy(tface);
	if (window != NULL)
		ui_window_destroy(window);
	if (ui != NULL)
		ui_destroy(ui);
	if (fedit != NULL)
		free(fedit);
	return rc;
}

/** Destroy font editor.
 *
 * @param fedit Font editor
 */
static void font_edit_destroy(font_edit_t *fedit)
{
	gfx_glyph_bmp_close(fedit->gbmp);
	gfx_glyph_destroy(fedit->glyph);
	gfx_font_close(fedit->font);
	gfx_typeface_destroy(fedit->typeface);
	ui_window_destroy(fedit->window);
	ui_destroy(fedit->ui);
	free(fedit);
}

static void print_syntax(void)
{
	printf("Syntax: fontedit [-d <display-spec>] [<file.tpf>]\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;
	const char *display_spec = UI_DISPLAY_DEFAULT;
	const char *fname = NULL;
	font_edit_t *fedit;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	/* File name argument? */
	if (i < argc) {
		fname = argv[i];
		++i;
	}

	/* Extra arguments? */
	if (i < argc) {
		printf("Unexpected argument '%s'.\n", argv[i]);
		print_syntax();
		return 1;
	}

	rc = font_edit_create(display_spec, fname, &fedit);
	if (rc != EOK)
		return 1;

	(void) font_edit_paint(fedit);

	ui_run(fedit->ui);
	font_edit_destroy(fedit);

	(void) font_edit_kbd_event;
	(void) font_edit_pos_event;
	return 0;
}

/** @}
 */
