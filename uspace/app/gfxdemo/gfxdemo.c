/*
 * Copyright (c) 2019 Jiri Svoboda
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
#include <stdlib.h>
#include <str.h>
#include <window.h>

/** Run rectangle demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_rects(gfx_context_t *gc, int w, int h)
{
	gfx_color_t *color = NULL;
	gfx_rect_t rect;
	int i, j;
	errno_t rc;

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
	}

	return EOK;
}

/** Run bitmap demo on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_bitmap(gfx_context_t *gc, int w, int h)
{
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	int i, j;
	gfx_coord2_t offs;
	gfx_rect_t srect;
	errno_t rc;
	pixelmap_t pixelmap;

	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = w;
	params.rect.p1.y = h;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		return rc;

	/* Fill bitmap with something. In absence of anything else, use pixelmap */
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

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 5; i++) {
			srect.p0.x = rand() % (w - 40);
			srect.p0.y = rand() % (h - 40);
			srect.p1.x = srect.p0.x + rand() % (w - srect.p0.x);
			srect.p1.y = srect.p0.y + rand() % (h - srect.p0.y);
			offs.x = rand() % (w - srect.p1.x);
			offs.y = rand() % (h - srect.p1.y);
			offs.x = 0;
			offs.y = 0;

			gfx_bitmap_render(bitmap, &srect, &offs);
			fibril_usleep(500 * 1000);
		}
	}

	gfx_bitmap_destroy(bitmap);

	return EOK;
}

/** Run demo loop on a graphic context.
 *
 * @param gc Graphic context
 * @param w Width
 * @param h Height
 */
static errno_t demo_loop(gfx_context_t *gc, int w, int h)
{
	errno_t rc;

	while (true) {
		rc = demo_rects(gc, w, h);
		if (rc != EOK)
			return rc;

		rc = demo_bitmap(gc, w, h);
		if (rc != EOK)
			return rc;
	}
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
static errno_t demo_canvas(void)
{
	canvas_gc_t *cgc = NULL;
	gfx_context_t *gc;
	window_t *window = NULL;
	pixel_t *pixbuf = NULL;
	surface_t *surface = NULL;
	canvas_t *canvas = NULL;
	int vw, vh;
	errno_t rc;

	printf("Init canvas..\n");

	window = window_open("comp:0/winreg", NULL,
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

	rc = demo_loop(gc, 400, 300);
	if (rc != EOK)
		return rc;

	rc = canvas_gc_delete(cgc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Run demo on display server. */
static errno_t demo_display(void)
{
	display_t *display = NULL;
	gfx_context_t *gc;
	display_window_t *window = NULL;
	errno_t rc;

	printf("Init display..\n");

	rc = display_open(NULL, &display);
	if (rc != EOK) {
		printf("Error opening display.\n");
		return rc;
	}

	rc = display_window_create(display, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	rc = display_window_get_gc(window, &gc);
	if (rc != EOK) {
		printf("Error getting graphics context.\n");
	}

	rc = demo_loop(gc, 400, 300);
	if (rc != EOK)
		return rc;

	rc = gfx_context_delete(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

static void print_syntax(void)
{
	printf("syntax: gfxdemo {canvas|console|display}\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;

	if (argc < 2) {
		print_syntax();
		return 1;
	}

	if (str_cmp(argv[1], "console") == 0) {
		rc = demo_console();
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "canvas") == 0) {
		rc = demo_canvas();
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "display") == 0) {
		rc = demo_display();
		if (rc != EOK)
			return 1;
	} else {
		print_syntax();
		return 1;
	}
}

/** @}
 */
