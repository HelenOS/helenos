/*
 * Copyright (c) 2020 Jiri Svoboda
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

#include <canvas.h>
#include <draw/surface.h>
#include <fibril.h>
#include <guigfx/canvas.h>
#include <gfx/color.h>
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/render.h>
#include <gfx/typeface.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <window.h>
#include "fontedit.h"

enum {
	glyph_scale = 8,
	glyph_orig_x = 100,
	glyph_orig_y = 100
};

static errno_t font_edit_paint(font_edit_t *);

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

/** Handle font editor position event.
 *
 * @param widget Canvas widget
 * @param data Position event
 */
static void font_edit_pos_event(widget_t *widget, void *data)
{
	pos_event_t *event = (pos_event_t *) data;
	font_edit_t *fedit;
	int x, y;

	fedit = (font_edit_t *) widget_get_data(widget);

	if (event->type == POS_PRESS) {
		x = gfx_coord_div_rneg((int)event->hpos - glyph_orig_x,
		    glyph_scale);
		y = gfx_coord_div_rneg((int)event->vpos - glyph_orig_y,
		    glyph_scale);

		printf("x=%d y=%d\n", x, y);
		gfx_glyph_bmp_setpix(fedit->gbmp, x, y, fedit->pen_color);
		font_edit_paint(fedit);
	}
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
 * @param widget Canvas widget
 * @param data Position event
 */
static void font_edit_kbd_event(widget_t *widget, void *data)
{
	kbd_event_t *event = (kbd_event_t *) data;
	font_edit_t *fedit;

	fedit = (font_edit_t *) widget_get_data(widget);

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

/** Paint font preview.
 *
 * @param fedit Font editor
 */
static errno_t font_edit_paint_preview(font_edit_t *fedit)
{
	gfx_glyph_metrics_t gmetrics;
	size_t stradv;
	const char *cp;
	gfx_glyph_t *glyph;
	gfx_coord2_t pos;
	errno_t rc;

	cp = "ABCD";
	pos.x = 20;
	pos.y = 20;

	while (*cp != '\0') {
		rc = gfx_font_search_glyph(fedit->font, cp, &glyph, &stradv);
		if (rc != EOK) {
			++cp;
			continue;
		}

		gfx_glyph_get_metrics(glyph, &gmetrics);

		rc = gfx_glyph_render(glyph, &pos);
		if (rc != EOK)
			return rc;

		cp += stradv;
		pos.x += gmetrics.advance;
	}

	return EOK;
}

/** Paint glyph bitmap.
 *
 * @param fedit Font editor
 */
static errno_t font_edit_paint_gbmp(font_edit_t *fedit)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	gfx_rect_t grect;
	gfx_glyph_metrics_t gmetrics;
	errno_t rc;
	int x, y;
	int pix;

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
 * @param display_svc Display service
 * @param fname Font file to open or @c NULL to create new font
 * @param rfedit Place to store pointer to new font editor
 * @return EOK on success or an error code
 */
static errno_t font_edit_create(const char *display_svc, const char *fname,
    font_edit_t **rfedit)
{
	canvas_gc_t *cgc = NULL;
	window_t *window = NULL;
	pixel_t *pixbuf = NULL;
	surface_t *surface = NULL;
	canvas_t *canvas = NULL;
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
	gfx_context_t *gc;
	errno_t rc;

	fedit = calloc(1, sizeof(font_edit_t));
	if (fedit == NULL) {
		rc = ENOMEM;
		goto error;
	}

	printf("Init canvas..\n");

	window = window_open(display_svc, NULL,
	    WINDOW_MAIN | WINDOW_DECORATED, "Font Editor");
	if (window == NULL) {
		printf("Error creating window.\n");
		rc = ENOMEM;
		goto error;
	}

	vw = 400;
	vh = 300;

	pixbuf = calloc(vw * vh, sizeof(pixel_t));
	if (pixbuf == NULL) {
		printf("Error allocating memory for pixel buffer.\n");
		rc = ENOMEM;
		goto error;
	}

	surface = surface_create(vw, vh, pixbuf, 0);
	if (surface == NULL) {
		printf("Error creating surface.\n");
		rc = ENOMEM;
		goto error;
	}

	/* Memory block pixbuf is now owned by surface */
	pixbuf = NULL;

	canvas = create_canvas(window_root(window), fedit, vw, vh,
	    surface);
	if (canvas == NULL) {
		printf("Error creating canvas.\n");
		rc = ENOMEM;
		goto error;
	}

	window_resize(window, 0, 0, vw + 10, vh + 30, WINDOW_PLACEMENT_ANY);
	window_exec(window);

	printf("Create canvas GC\n");
	rc = canvas_gc_create(canvas, surface, &cgc);
	if (rc != EOK) {
		printf("Error creating canvas GC.\n");
		goto error;
	}

	gc = canvas_gc_get_ctx(cgc);

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

	sig_connect(&canvas->position_event, &canvas->widget,
	    font_edit_pos_event);
	sig_connect(&canvas->keyboard_event, &canvas->widget,
	    font_edit_kbd_event);

	if (fname == NULL)
		fname = "new.tpf";

	fedit->cgc = cgc;
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
	if (surface != NULL)
		surface_destroy(surface);
	if (pixbuf != NULL)
		free(pixbuf);
	if (window != NULL)
		window_close(window);
	if (fedit != NULL)
		free(fedit);
	return rc;
}

static void print_syntax(void)
{
	printf("Syntax: fontedit [-d <display>] [<file.tpf>]\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;
	const char *display_svc = DISPLAY_DEFAULT;
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

			display_svc = argv[i++];
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

	rc = font_edit_create(display_svc, fname, &fedit);
	if (rc != EOK)
		return 1;

	(void) font_edit_paint(fedit);

	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
