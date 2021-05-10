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

/** @addtogroup gfxdemo
 * @{
 */
/** @file Graphic demo
 */

#include <congfx/console.h>
#include <display.h>
#include <fibril.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <gfx/font.h>
#include <gfx/text.h>
#include <gfx/typeface.h>
#include <io/console.h>
#include <io/pixelmap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <ui/wdecor.h>

static void wnd_close_event(void *);
static void wnd_kbd_event(void *, kbd_event_t *);

static display_wnd_cb_t wnd_cb = {
	.close_event = wnd_close_event,
	.kbd_event = wnd_kbd_event
};

static void uiwnd_close_event(ui_window_t *, void *);
static void uiwnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t ui_window_cb = {
	.close = uiwnd_close_event,
	.kbd = uiwnd_kbd_event
};

static bool quit = false;
static gfx_typeface_t *tface;
static gfx_font_t *font;
static gfx_coord_t vpad;

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

/** Initialize demo font.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 * @return EOK on success or an error code
 */
static errno_t demo_font_init(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_font_info_t *finfo;
	errno_t rc;

	if (quit)
		return EOK;

	/* XXX Crude way of detecting text mode */
	if (w < 256) {
		/* Create dummy font for text mode */
		rc = gfx_typeface_create(gc, &tface);
		if (rc != EOK) {
			printf("Error creating typeface\n");
			goto error;
		}

		rc = gfx_font_create_textmode(tface, &font);
		if (rc != EOK) {
			printf("Error creating font\n");
			goto error;
		}

		vpad = 0;
	} else {
		/* Load font */
		rc = gfx_typeface_open(gc, "/data/font/helena.tpf", &tface);
		if (rc != EOK) {
			printf("Error opening typeface\n");
			goto error;
		}

		finfo = gfx_typeface_first_font(tface);
		if (finfo == NULL) {
			printf("Typeface contains no font.\n");
			rc = ENOENT;
			goto error;
		}

		rc = gfx_font_open(finfo, &font);
		if (rc != EOK) {
			printf("Error opening font.\n");
			goto error;
		}

		vpad = 5;
	}

	return EOK;
error:
	if (tface != NULL)
		gfx_typeface_destroy(tface);
	return rc;
}

/** Finalize demo font. */
static void demo_font_fini(void)
{
	if (font == NULL)
		return;

	gfx_font_close(font);
	font = NULL;

	gfx_typeface_destroy(tface);
	tface = NULL;
}

/** Start a new demo screen.
 *
 * Clear the screen, display a status line and set up clipping.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 * @param text Demo screen description
 * @return EOK on success or an error code
 */
static errno_t demo_begin(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h,
    const char *text)
{
	gfx_text_fmt_t fmt;
	gfx_font_metrics_t metrics;
	gfx_coord2_t pos;
	gfx_color_t *color;
	gfx_rect_t rect;
	gfx_coord_t height;
	errno_t rc;

	rc = gfx_set_clip_rect(gc, NULL);
	if (rc != EOK)
		return rc;

	rc = clear_scr(gc, w, h);
	if (rc != EOK)
		return rc;

	if (font != NULL) {
		rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
		if (rc != EOK)
			goto error;

		gfx_text_fmt_init(&fmt);
		fmt.color = color;
		fmt.halign = gfx_halign_center;
		fmt.valign = gfx_valign_bottom;

		pos.x = w / 2;
		pos.y = h - 1;
		rc = gfx_puttext(font, &pos, &fmt, text);
		if (rc != EOK) {
			printf("Error rendering text.\n");
			gfx_color_delete(color);
			goto error;
		}

		gfx_color_delete(color);

		gfx_font_get_metrics(font, &metrics);
		height = metrics.ascent + metrics.descent + 1;
	} else {
		height = 0;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = w;
	rect.p1.y = h - height - vpad;
	rc = gfx_set_clip_rect(gc, &rect);
	if (rc != EOK)
		return rc;

	return EOK;
error:
	return rc;
}

/** Run rectangle demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_rects(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	int i, j;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Rectangle rendering");
	if (rc != EOK)
		return rc;

	for (j = 0; j < 10; j++) {
		rc = gfx_color_new_rgb_i16(rand() % 0x10000, rand() % 0x10000,
		    rand() % 0x10000, &color);
		if (rc != EOK)
			return rc;

		rc = gfx_set_color(gc, color);
		if (rc != EOK)
			return rc;

		for (i = 0; i < 10; i++) {
			rect.p0.x = rand() % (w - 1);
			rect.p0.y = rand() % (h - 1);
			rect.p1.x = rect.p0.x + rand() % (w - 1 - rect.p0.x);
			rect.p1.y = rect.p0.y + rand() % (h - 1 - rect.p0.y);

			rc = gfx_fill_rect(gc, &rect);
			if (rc != EOK)
				return rc;
		}

		gfx_color_delete(color);

		fibril_usleep(500 * 1000);

		if (quit)
			break;
	}

	return EOK;
}

/** Fill bitmap with tartan pattern.
 *
 * @param bitmap Bitmap
 * @param w Bitmap width
 * @param h Bitmap height
 * @return EOK on success or an error code
 */
static errno_t bitmap_tartan(gfx_bitmap_t *bitmap, gfx_coord_t w, gfx_coord_t h)
{
	int i, j;
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		return rc;

	/* In absence of anything else, use pixelmap */
	pixelmap.width = w;
	pixelmap.height = h;
	pixelmap.data = alloc.pixels;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			pixelmap_put_pixel(&pixelmap, i, j,
			    PIXEL(0, (i % 30) < 3 ? 255 : 0,
			    (j % 30) < 3 ? 255 : 0, i / 2));
		}
	}

	return EOK;
}

/** Fill bitmap with moire pattern.
 *
 * @param bitmap Bitmap
 * @param w Bitmap width
 * @param h Bitmap height
 * @return EOK on success or an error code
 */
static errno_t bitmap_moire(gfx_bitmap_t *bitmap, gfx_coord_t w, gfx_coord_t h)
{
	int i, j;
	int k;
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		return rc;

	/* In absence of anything else, use pixelmap */
	pixelmap.width = w;
	pixelmap.height = h;
	pixelmap.data = alloc.pixels;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			k = i * i + j * j;
			pixelmap_put_pixel(&pixelmap, i, j,
			    PIXEL(0, k, k, k));
		}
	}

	return EOK;
}

/** Render circle to a bitmap.
 *
 * @param bitmap Bitmap
 * @param w Bitmap width
 * @param h Bitmap height
 * @return EOK on success or an error code
 */
static errno_t bitmap_circle(gfx_bitmap_t *bitmap, gfx_coord_t w, gfx_coord_t h)
{
	int i, j;
	int k;
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		return rc;

	/* In absence of anything else, use pixelmap */
	pixelmap.width = w;
	pixelmap.height = h;
	pixelmap.data = alloc.pixels;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			k = i * i + j * j;
			pixelmap_put_pixel(&pixelmap, i, j,
			    k < w * w / 2 ? PIXEL(0, 0, 255, 0) :
			    PIXEL(0, 255, 0, 255));
		}
	}

	return EOK;
}

/** Run bitmap demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_bitmap(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	int i, j;
	gfx_coord2_t offs;
	gfx_rect_t srect;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Bitmap rendering without offset");
	if (rc != EOK)
		return rc;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = w;
	params.rect.p1.y = h;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = bitmap_tartan(bitmap, w, h);
	if (rc != EOK)
		goto error;

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 5; i++) {
			srect.p0.x = rand() % (w - 40);
			srect.p0.y = rand() % (h - 20);
			srect.p1.x = srect.p0.x + rand() % (w - srect.p0.x);
			srect.p1.y = srect.p0.y + rand() % (h - srect.p0.y);
			offs.x = 0;
			offs.y = 0;

			rc = gfx_bitmap_render(bitmap, &srect, &offs);
			if (rc != EOK)
				goto error;
			fibril_usleep(250 * 1000);

			if (quit)
				goto out;
		}
	}

out:
	gfx_bitmap_destroy(bitmap);

	return EOK;
error:
	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Run second bitmap demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_bitmap2(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	int i, j;
	gfx_coord2_t offs;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Bitmap rendering with offset");
	if (rc != EOK)
		return rc;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 40;
	params.rect.p1.y = 20;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = bitmap_moire(bitmap, 40, 20);
	if (rc != EOK)
		goto error;

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 10; i++) {
			offs.x = rand() % (w - 40);
			offs.y = rand() % (h - 20);

			rc = gfx_bitmap_render(bitmap, NULL, &offs);
			if (rc != EOK)
				goto error;
		}

		fibril_usleep(500 * 1000);

		if (quit)
			break;
	}

	gfx_bitmap_destroy(bitmap);

	return EOK;
error:
	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Run bitmap color key demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_bitmap_kc(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	int i, j;
	gfx_coord2_t offs;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Bitmap rendering with color key");
	if (rc != EOK)
		return rc;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 40;
	params.rect.p1.y = 40;
	params.flags = bmpf_color_key;
	params.key_color = PIXEL(0, 255, 0, 255);

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = bitmap_circle(bitmap, 40, 40);
	if (rc != EOK)
		goto error;

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 10; i++) {
			offs.x = j * 20 + i * 20;
			offs.y = i * 20;

			rc = gfx_bitmap_render(bitmap, NULL, &offs);
			if (rc != EOK)
				goto error;
		}

		fibril_usleep(500 * 1000);

		if (quit)
			break;
	}

	gfx_bitmap_destroy(bitmap);

	return EOK;
error:
	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Run text demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_text(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	int i;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Text rendering");
	if (rc != EOK)
		goto error;

	/* Vertical bars */

	for (i = 0; i < 20; i++) {
		rc = gfx_color_new_rgb_i16(0, 0x8000 * i / 20,
		    0x8000 * i / 20, &color);
		if (rc != EOK)
			goto error;

		rc = gfx_set_color(gc, color);
		if (rc != EOK)
			goto error;

		rect.p0.x = w * i / 20;
		rect.p0.y = 0;
		rect.p1.x = w * (i + 1) / 20;
		rect.p1.y = h;

		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			goto error;

		gfx_color_delete(color);
	}

	rc = gfx_color_new_rgb_i16(0, 0, 0x8000, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(gc, color);
	if (rc != EOK)
		goto error;

	rect.p0.x = w / 20;
	rect.p0.y = 1 * h / 15;
	rect.p1.x = w - w / 20;
	rect.p1.y = 4 * h / 15;

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	if (rc != EOK)
		goto error;

	gfx_text_fmt_init(&fmt);
	fmt.color = color;

	pos.x = rect.p0.x;
	pos.y = rect.p0.y;
	rc = gfx_puttext(font, &pos, &fmt, "Top left");
	if (rc != EOK) {
		printf("Error rendering text.\n");
		goto error;
	}

	pos.x = (rect.p0.x + rect.p1.x - 1) / 2;
	pos.y = rect.p0.y;
	fmt.halign = gfx_halign_center;
	rc = gfx_puttext(font, &pos, &fmt, "Top center");
	if (rc != EOK)
		goto error;

	pos.x = rect.p1.x - 1;
	pos.y = rect.p0.y;
	fmt.halign = gfx_halign_right;
	rc = gfx_puttext(font, &pos, &fmt, "Top right");
	if (rc != EOK)
		goto error;

	fmt.valign = gfx_valign_center;

	pos.x = rect.p0.x;
	pos.y = (rect.p0.y + rect.p1.y - 1) / 2;
	fmt.halign = gfx_halign_left;
	rc = gfx_puttext(font, &pos, &fmt, "Center left");
	if (rc != EOK)
		goto error;

	pos.x = (rect.p0.x + rect.p1.x - 1) / 2;
	pos.y = (rect.p0.y + rect.p1.y - 1) / 2;
	fmt.halign = gfx_halign_center;
	rc = gfx_puttext(font, &pos, &fmt, "Center");
	if (rc != EOK)
		goto error;

	pos.x = rect.p1.x - 1;
	pos.y = (rect.p0.y + rect.p1.y - 1) / 2;
	fmt.halign = gfx_halign_right;
	rc = gfx_puttext(font, &pos, &fmt, "Center right");
	if (rc != EOK)
		goto error;

	fmt.valign = gfx_valign_bottom;

	pos.x = rect.p0.x;
	pos.y = rect.p1.y - 1;
	fmt.halign = gfx_halign_left;
	rc = gfx_puttext(font, &pos, &fmt, "Bottom left");
	if (rc != EOK)
		goto error;

	pos.x = (rect.p0.x + rect.p1.x - 1) / 2;
	pos.y = rect.p1.y - 1;
	fmt.halign = gfx_halign_center;
	rc = gfx_puttext(font, &pos, &fmt, "Bottom center");
	if (rc != EOK)
		goto error;

	pos.x = rect.p1.x - 1;
	pos.y = rect.p1.y - 1;
	fmt.halign = gfx_halign_right;
	rc = gfx_puttext(font, &pos, &fmt, "Bottom right");
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	gfx_text_fmt_init(&fmt);

	for (i = 0; i < 8; i++) {
		rc = gfx_color_new_rgb_i16((i & 4) ? 0xffff : 0,
		    (i & 2) ? 0xffff : 0, (i & 1) ? 0xffff : 0, &color);
		if (rc != EOK)
			goto error;

		fmt.color = color;

		pos.x = w / 20;
		pos.y = (6 + i) * h / 15;
		rc = gfx_puttext(font, &pos, &fmt, "The quick brown fox jumps over the lazy dog.");
		if (rc != EOK)
			goto error;

		gfx_color_delete(color);
	}

	for (i = 0; i < 10; i++) {
		fibril_usleep(500 * 1000);
		if (quit)
			break;
	}

	return EOK;
error:
	return rc;
}

/** Run clipping demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_clip(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	gfx_bitmap_t *bitmap;
	gfx_color_t *color;
	gfx_bitmap_params_t params;
	int i, j;
	gfx_coord2_t offs;
	gfx_rect_t rect;
	errno_t rc;

	if (quit)
		return EOK;

	rc = demo_begin(gc, w, h, "Clipping demonstration");
	if (rc != EOK)
		return rc;

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 40;
	params.rect.p1.y = 20;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = bitmap_moire(bitmap, 40, 20);
	if (rc != EOK)
		goto error;

	for (j = 0; j < 10; j++) {
		rect.p0.x = w / 8;
		rect.p0.y = h / 8;
		rect.p1.x = w * 7 / 8;
		rect.p1.y = h * 3 / 8;

		rc = gfx_set_clip_rect(gc, &rect);
		if (rc != EOK)
			return rc;

		rc = gfx_color_new_rgb_i16(rand() % 0x10000, rand() % 0x10000,
		    rand() % 0x10000, &color);
		if (rc != EOK)
			return rc;

		rc = gfx_set_color(gc, color);
		if (rc != EOK)
			return rc;

		for (i = 0; i < 10; i++) {
			rect.p0.x = rand() % (w - 1);
			rect.p0.y = rand() % (h - 1);
			rect.p1.x = rect.p0.x + rand() % (w - 1 - rect.p0.x);
			rect.p1.y = rect.p0.y + rand() % (h - 1 - rect.p0.y);

			rc = gfx_fill_rect(gc, &rect);
			if (rc != EOK)
				return rc;
		}

		gfx_color_delete(color);

		rect.p0.x = w / 8;
		rect.p0.y = h * 5 / 8;
		rect.p1.x = w * 7 / 8;
		rect.p1.y = h * 7 / 8;

		rc = gfx_set_clip_rect(gc, &rect);
		if (rc != EOK)
			return rc;

		for (i = 0; i < 10; i++) {
			offs.x = rand() % (w - 40);
			offs.y = rand() % (h - 20);

			rc = gfx_bitmap_render(bitmap, NULL, &offs);
			if (rc != EOK)
				goto error;
		}

		fibril_usleep(500 * 1000);

		if (quit)
			break;
	}

	(void) gfx_set_clip_rect(gc, NULL);
	gfx_bitmap_destroy(bitmap);
	return EOK;
error:
	(void) gfx_set_clip_rect(gc, NULL);
	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Run demo loop on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_loop(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	errno_t rc;

	(void) demo_font_init(gc, w, h);

	while (!quit) {
		rc = demo_rects(gc, w, h);
		if (rc != EOK)
			goto error;

		rc = demo_bitmap(gc, w, h);
		if (rc != EOK)
			goto error;

		rc = demo_bitmap2(gc, w, h);
		if (rc != EOK)
			goto error;

		rc = demo_bitmap_kc(gc, w, h);
		if (rc != EOK)
			goto error;

		rc = demo_text(gc, w, h);
		if (rc != EOK)
			goto error;

		rc = demo_clip(gc, w, h);
		if (rc != EOK)
			goto error;
	}

	demo_font_fini();
	return EOK;
error:
	demo_font_fini();
	return rc;
}

/** Run demo on console. */
static errno_t demo_console(void)
{
	console_ctrl_t *con = NULL;
	console_gc_t *cgc = NULL;
	gfx_context_t *gc;
	errno_t rc;

	printf("Init console..\n");
	con = console_init(stdin, stdout);
	if (con == NULL)
		return EIO;

	printf("Create console GC\n");
	rc = console_gc_create(con, stdout, &cgc);
	if (rc != EOK)
		return rc;

	gc = console_gc_get_ctx(cgc);

	rc = demo_loop(gc, 80, 25);
	if (rc != EOK)
		return rc;

	rc = console_gc_delete(cgc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Run demo on UI. */
static errno_t demo_ui(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	gfx_context_t *gc;
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	errno_t rc;

	printf("Init UI..\n");

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error initializing UI (%s)\n", display_spec);
		goto error;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = 400;
	rect.p1.y = 300;

	ui_wnd_params_init(&params);
	params.caption = "GFX Demo";

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(params.style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &params.rect);

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &ui_window_cb, NULL);

	rc = ui_window_get_app_gc(window, &gc);
	if (rc != EOK) {
		printf("Error creating graphic context.\n");
		goto error;
	}

	task_retval(0);

	rc = demo_loop(gc, rect.p1.x, rect.p1.y);
	if (rc != EOK)
		goto error;

	ui_window_destroy(window);
	ui_destroy(ui);

	return EOK;
error:
	if (window != NULL)
		ui_window_destroy(window);
	if (ui != NULL)
		ui_destroy(ui);
	return rc;
}

/** Run demo on display server. */
static errno_t demo_display(const char *display_svc)
{
	display_t *display = NULL;
	gfx_context_t *gc;
	display_wnd_params_t params;
	display_window_t *window = NULL;
	errno_t rc;

	printf("Init display..\n");

	rc = display_open(display_svc, &display);
	if (rc != EOK) {
		printf("Error opening display.\n");
		return rc;
	}

	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 400;
	params.rect.p1.y = 300;

	rc = display_window_create(display, &params, &wnd_cb, NULL, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	rc = display_window_get_gc(window, &gc);
	if (rc != EOK) {
		printf("Error getting graphics context.\n");
		return rc;
	}

	task_retval(0);

	rc = demo_loop(gc, 400, 300);
	if (rc != EOK)
		return rc;

	rc = gfx_context_delete(gc);
	if (rc != EOK)
		return rc;

	display_window_destroy(window);
	display_close(display);

	return EOK;
}

static void wnd_close_event(void *arg)
{
	printf("Close event\n");
	quit = true;
}

static void wnd_kbd_event(void *arg, kbd_event_t *event)
{
	printf("Keyboard event type=%d key=%d\n", event->type, event->key);
	if (event->type == KEY_PRESS)
		quit = true;
}

static void uiwnd_close_event(ui_window_t *window, void *arg)
{
	printf("Close event\n");
	quit = true;
}

static void uiwnd_kbd_event(ui_window_t *window, void *arg, kbd_event_t *event)
{
	printf("Keyboard event type=%d key=%d\n", event->type, event->key);
	if (event->type == KEY_PRESS)
		quit = true;
}

static void print_syntax(void)
{
	printf("Syntax: gfxdemo [-d <display>] {console|display|ui}\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;
	const char *display_svc = DISPLAY_DEFAULT;
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

	if (i >= argc || str_cmp(argv[i], "display") == 0) {
		rc = demo_display(display_svc);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[i], "console") == 0) {
		rc = demo_console();
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[i], "ui") == 0) {
		rc = demo_ui(display_svc);
		if (rc != EOK)
			return 1;
	} else {
		print_syntax();
		return 1;
	}
}

/** @}
 */
