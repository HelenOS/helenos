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
#include <gfx/render.h>
#include <gfx/typeface.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <window.h>
#include "fontedit.h"

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

	return EOK;
}

/** Create font editor.
 *
 * @param display_svc Display service
 * @param rfedit Place to store pointer to new font editor
 * @return EOK on success or an error code
 */
static errno_t font_edit_create(const char *display_svc, font_edit_t **rfedit)
{
	canvas_gc_t *cgc = NULL;
	window_t *window = NULL;
	pixel_t *pixbuf = NULL;
	surface_t *surface = NULL;
	canvas_t *canvas = NULL;
	font_edit_t *fedit = NULL;
	gfx_typeface_t *tface = NULL;
	gfx_font_t *font = NULL;
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
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

	canvas = create_canvas(window_root(window), NULL, vw, vh,
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

	fedit->cgc = cgc;
	fedit->gc = gc;
	fedit->width = vw;
	fedit->height = vh;
	fedit->typeface = tface;

	*rfedit = fedit;
	return EOK;
error:
	/*
	 * Once the window is created it would be probably more correct
	 * to leave destruction of these resources to a window destroy
	 * handler (which we have no way of registering)
	 */
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
	printf("Syntax: fontedit [-d <display>]\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;
	const char *display_svc = DISPLAY_DEFAULT;
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

	rc = font_edit_create(display_svc, &fedit);
	if (rc != EOK)
		return 1;

	(void) font_edit_paint(fedit);

	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
