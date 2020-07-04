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

#include <canvas.h>
#include <congfx/console.h>
#include <draw/surface.h>
#include <display.h>
#include <fibril.h>
#include <guigfx/canvas.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/render.h>
#include <io/console.h>
#include <io/pixelmap.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <window.h>

static void wnd_close_event(void *);
static void wnd_kbd_event(void *, kbd_event_t *);

static display_wnd_cb_t wnd_cb = {
	.close_event = wnd_close_event,
	.kbd_event = wnd_kbd_event
};

static bool quit = false;

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

	rc = clear_scr(gc, w, h);
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
			    PIXEL(255, (i % 30) < 3 ? 255 : 0,
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
			    PIXEL(255, k, k, k));
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
			    k < w * w / 2 ? PIXEL(255, 0, 255, 0) :
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

	rc = clear_scr(gc, w, h);
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
				break;
		}
	}

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

	rc = clear_scr(gc, w, h);
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

	rc = clear_scr(gc, w, h);
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

/** Run demo loop on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_loop(gfx_context_t *gc, gfx_coord_t w, gfx_coord_t h)
{
	errno_t rc;

	while (!quit) {
		rc = demo_rects(gc, w, h);
		if (rc != EOK)
			return rc;

		rc = demo_bitmap(gc, w, h);
		if (rc != EOK)
			return rc;

		rc = demo_bitmap2(gc, w, h);
		if (rc != EOK)
			return rc;

		rc = demo_bitmap_kc(gc, w, h);
		if (rc != EOK)
			return rc;
	}

	return EOK;
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

/** Run demo on canvas. */
static errno_t demo_canvas(const char *display_svc)
{
	canvas_gc_t *cgc = NULL;
	gfx_context_t *gc;
	window_t *window = NULL;
	pixel_t *pixbuf = NULL;
	surface_t *surface = NULL;
	canvas_t *canvas = NULL;
	gfx_coord_t vw, vh;
	errno_t rc;

	printf("Init canvas..\n");

	window = window_open(display_svc, NULL,
	    WINDOW_MAIN | WINDOW_DECORATED, "GFX Demo");
	if (window == NULL) {
		printf("Error creating window.\n");
		return -1;
	}

	vw = 400;
	vh = 300;

	pixbuf = calloc(vw * vh, sizeof(pixel_t));
	if (pixbuf == NULL) {
		printf("Error allocating memory for pixel buffer.\n");
		return ENOMEM;
	}

	surface = surface_create(vw, vh, pixbuf, 0);
	if (surface == NULL) {
		printf("Error creating surface.\n");
		return EIO;
	}

	canvas = create_canvas(window_root(window), NULL, vw, vh,
	    surface);
	if (canvas == NULL) {
		printf("Error creating canvas.\n");
		return EIO;
	}

	window_resize(window, 0, 0, vw + 10, vh + 30, WINDOW_PLACEMENT_ANY);
	window_exec(window);

	printf("Create canvas GC\n");
	rc = canvas_gc_create(canvas, surface, &cgc);
	if (rc != EOK)
		return rc;

	gc = canvas_gc_get_ctx(cgc);

	task_retval(0);

	rc = demo_loop(gc, 400, 300);
	if (rc != EOK)
		return rc;

	rc = canvas_gc_delete(cgc);
	if (rc != EOK)
		return rc;

	return EOK;
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

static void print_syntax(void)
{
	printf("Syntax: gfxdemo [-d <display>] {canvas|console|display}\n");
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
	} else if (str_cmp(argv[i], "canvas") == 0) {
		rc = demo_canvas(display_svc);
		if (rc != EOK)
			return 1;
	} else {
		print_syntax();
		return 1;
	}
}

/** @}
 */
