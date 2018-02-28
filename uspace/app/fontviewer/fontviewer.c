/*
 * Copyright (c) 2014 Martin Sucha
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

/** @addtogroup fontviewer
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <str_error.h>
#include <window.h>
#include <canvas.h>
#include <surface.h>
#include <codec/tga.h>
#include <task.h>
#include <drawctx.h>
#include <font/embedded.h>
#include <font/pcf.h>
#include <stdarg.h>
#include <io/verify.h>

#define NAME  "fontviewer"

#define WINDOW_WIDTH   640
#define WINDOW_HEIGHT  480

static window_t *main_window;
static surface_t *surface = NULL;
static canvas_t *canvas = NULL;
static surface_coord_t width, height;
uint16_t points = 16;
bool show_metrics = true;
char *font_path = NULL;

static errno_t draw(void);

static void on_keyboard_event(widget_t *widget, void *data)
{
	kbd_event_t *event = (kbd_event_t *) data;

	if (event->type == KEY_PRESS) {
		if (event->c == 'q')
			exit(0);

		if (event->key == KC_UP || event->key == KC_DOWN) {
			uint16_t increment = (event->mods & KM_SHIFT) ? 10 : 1;

			if (event->key == KC_UP)
				points += increment;

			if (event->key == KC_DOWN) {
				if (points <= increment) {
					points = 1;
				}
				else {
					points-= increment;
				}
			}

			if (points < 1)
				points = 1;
		}

		if (event->c == 'm')
			show_metrics = !show_metrics;
	}

	errno_t rc = draw();
	if (rc != EOK) {
		printf("Failed drawing: %s.\n", str_error(rc));
		exit(1);
	}
	update_canvas(canvas, surface);
}

static errno_t create_font(font_t **font, uint16_t points)
{
	if (font_path == NULL) {
		return embedded_font_create(font, points);
	}

	return pcf_font_create(font, font_path, points);
}

static source_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
	source_t source;
	source_init(&source);
	source_set_color(&source, PIXEL(255, r, g, b));
	return source;
}

static void horizontal_rectangle(drawctx_t *drawctx, surface_coord_t x1,
    surface_coord_t y1, surface_coord_t x2, surface_coord_t y2,
    source_t *source)
{
	if (y2 < y1)
		return;

	drawctx_set_source(drawctx, source);
	drawctx_transfer(drawctx, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

static void horizontal_line(drawctx_t *drawctx, surface_coord_t y,
    surface_coord_t x1, surface_coord_t x2, source_t *source)
{
	horizontal_rectangle(drawctx, x1, y, x2, y, source);
}

static int text(drawctx_t *, font_t *, source_t *, surface_coord_t x,
    surface_coord_t , const char *, ...) _HELENOS_PRINTF_ATTRIBUTE(6, 7);
static int text(drawctx_t *drawctx, font_t *font, source_t *source,
    surface_coord_t x, surface_coord_t y, const char *fmt, ...)
{
	char *str = NULL;
	va_list args;
	va_start(args, fmt);
	int ret = vasprintf(&str, fmt, args);
	va_end(args);

	if (ret >= 0) {
		drawctx_set_source(drawctx, source);
		drawctx_set_font(drawctx, font);
		drawctx_print(drawctx, str, x, y);

		free(str);
	}

	return ret;
}


static errno_t draw(void)
{
	source_t background = rgb(255, 255, 255);
	source_t foreground = rgb(0, 0, 0);
	source_t glyphs = rgb(0, 0, 255);
	source_t ascender_bg = rgb(255, 230, 128);
	source_t ascender_fg = rgb(255, 153, 85);
	source_t descender_bg = rgb(204, 255, 170);
	source_t descender_fg = rgb(85, 212, 0);
	source_t leading_bg = rgb(170, 238, 255);
	source_t leading_fg = rgb(0, 170, 212);

	font_t *font;
	errno_t rc = create_font(&font, points);
	if (rc != EOK) {
		printf("Failed creating font\n");
		return rc;
	}

	font_t *info_font;
	rc = embedded_font_create(&info_font, 16);
	if (rc != EOK) {
		printf("Failed creating info font\n");
		return rc;
	}

	font_metrics_t font_metrics;
	rc = font_get_metrics(font, &font_metrics);
	if (rc != EOK)
		return rc;

	surface_coord_t top = 50;
	metric_t ascender_top = top;
	metric_t descender_top = ascender_top + font_metrics.ascender;
	metric_t leading_top = descender_top + font_metrics.descender;
	metric_t line_bottom = leading_top + font_metrics.leading;

	drawctx_t drawctx;
	drawctx_init(&drawctx, surface);

	drawctx_set_source(&drawctx, &background);
	drawctx_transfer(&drawctx, 0, 0,
	    width, height);

	if (show_metrics) {
		horizontal_rectangle(&drawctx, 0, ascender_top, width,
		    descender_top - 1, &ascender_bg);
		horizontal_line(&drawctx, ascender_top, 0, width,
		    &ascender_fg);

		horizontal_rectangle(&drawctx, 0, descender_top, width,
		    leading_top - 1, &descender_bg);
		horizontal_line(&drawctx, descender_top, 0, width,
		    &descender_fg);

		horizontal_rectangle(&drawctx, 0, leading_top,
		    width, line_bottom - 1, &leading_bg);
		horizontal_line(&drawctx, leading_top, 0, width,
		    &leading_fg);
	}

	drawctx_set_source(&drawctx, &glyphs);
	drawctx_set_font(&drawctx, font);
	drawctx_print(&drawctx, "Čaj'_", 0, top);

	if (show_metrics) {
		surface_coord_t infos_top = line_bottom + 10;
		text(&drawctx, info_font, &ascender_fg, 0, infos_top,
		    "Ascender: %d", font_metrics.ascender);
		text(&drawctx, info_font, &descender_fg, 0, infos_top + 16,
		    "Descender: %d", font_metrics.descender);
		text(&drawctx, info_font, &foreground, 0, infos_top + 32,
		    "Line height: %d",
		    font_metrics.ascender + font_metrics.descender);
		text(&drawctx, info_font, &leading_fg, 0, infos_top + 48,
		    "Leading: %d", font_metrics.leading);

	}

	font_release(font);
	return EOK;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Compositor server not specified.\n");
		return 1;
	}

	if (argc < 3) {
		font_path = NULL;
	}
	else {
		font_path = argv[2];
	}

	main_window = window_open(argv[1], NULL, WINDOW_MAIN, "fontviewer");
	if (!main_window) {
		printf("Cannot open main window.\n");
		return 2;
	}

	surface = surface_create(WINDOW_WIDTH, WINDOW_HEIGHT, NULL,
	    SURFACE_FLAG_NONE);
	if (surface == NULL) {
		printf("Cannot create surface.\n");
		return 2;
	}

	width = WINDOW_WIDTH;
	height = WINDOW_HEIGHT;

	errno_t rc = draw();
	if (rc != EOK) {
		printf("Failed drawing: %s.\n", str_error(rc));
		return 2;
	}

	canvas = create_canvas(window_root(main_window), NULL,
	    WINDOW_WIDTH, WINDOW_HEIGHT, surface);
	if (canvas == NULL) {
		printf("Cannot create canvas.\n");
		return 2;
	}
	sig_connect(&canvas->keyboard_event, NULL, on_keyboard_event);

	window_resize(main_window, 200, 200, WINDOW_WIDTH, WINDOW_HEIGHT,
	    WINDOW_PLACEMENT_ABSOLUTE);
	window_exec(main_window);

	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
