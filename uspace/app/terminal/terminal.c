/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup terminal
 * @{
 */
/**
 * @file Terminal application
 */

#include <adt/list.h>
#include <adt/prodcons.h>
#include <as.h>
#include <errno.h>
#include <fbfont/font-8x16.h>
#include <fibril.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/con_srv.h>
#include <io/concaps.h>
#include <io/console.h>
#include <io/pixelmap.h>
#include <macros.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <str.h>
#include <task.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>

#include "terminal.h"

#define NAME       "terminal"
#define NAMESPACE  "terminal"

#define LOCFS_MOUNT_POINT  "/loc"

#define APP_GETTERM  "/app/getterm"

#define TERM_CAPS \
	(CONSOLE_CAP_CURSORCTL | CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED | \
	CONSOLE_CAP_RGB)

#define SCROLLBACK_MAX_LINES 1000
#define MIN_WINDOW_COLS 8
#define MIN_WINDOW_ROWS 4

static LIST_INITIALIZE(terms);

#define COLOR_BRIGHT 8

static const pixel_t _basic_colors[16] = {
	[COLOR_BLACK]       = PIXEL(255, 0, 0, 0),
	[COLOR_RED]         = PIXEL(255, 170, 0, 0),
	[COLOR_GREEN]       = PIXEL(255, 0, 170, 0),
	[COLOR_YELLOW]      = PIXEL(255, 170, 85, 0),
	[COLOR_BLUE]        = PIXEL(255, 0, 0, 170),
	[COLOR_MAGENTA]     = PIXEL(255, 170, 0, 170),
	[COLOR_CYAN]        = PIXEL(255, 0, 170, 170),
	[COLOR_WHITE]       = PIXEL(255, 170, 170, 170),

	[COLOR_BLACK   | COLOR_BRIGHT] = PIXEL(255, 85, 85, 85),
	[COLOR_RED     | COLOR_BRIGHT] = PIXEL(255, 255, 85, 85),
	[COLOR_GREEN   | COLOR_BRIGHT] = PIXEL(255, 85, 255, 85),
	[COLOR_YELLOW  | COLOR_BRIGHT] = PIXEL(255, 255, 255, 85),
	[COLOR_BLUE    | COLOR_BRIGHT] = PIXEL(255, 85, 85, 255),
	[COLOR_MAGENTA | COLOR_BRIGHT] = PIXEL(255, 255, 85, 255),
	[COLOR_CYAN    | COLOR_BRIGHT] = PIXEL(255, 85, 255, 255),
	[COLOR_WHITE   | COLOR_BRIGHT] = PIXEL(255, 255, 255, 255),
};

static errno_t term_open(con_srvs_t *, con_srv_t *);
static errno_t term_close(con_srv_t *);
static errno_t term_read(con_srv_t *, void *, size_t, size_t *);
static errno_t term_write(con_srv_t *, void *, size_t, size_t *);
static void term_sync(con_srv_t *);
static void term_clear(con_srv_t *);
static void term_set_pos(con_srv_t *, sysarg_t col, sysarg_t row);
static errno_t term_get_pos(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t term_get_size(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t term_get_color_cap(con_srv_t *, console_caps_t *);
static void term_set_style(con_srv_t *, console_style_t);
static void term_set_color(con_srv_t *, console_color_t, console_color_t,
    console_color_attr_t);
static void term_set_rgb_color(con_srv_t *, pixel_t, pixel_t);
static void term_set_cursor_visibility(con_srv_t *, bool);
static errno_t term_set_caption(con_srv_t *, const char *);
static errno_t term_get_event(con_srv_t *, cons_event_t *);
static errno_t term_map(con_srv_t *, sysarg_t, sysarg_t, charfield_t **);
static void term_unmap(con_srv_t *);
static void term_buf_update(con_srv_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

static con_ops_t con_ops = {
	.open = term_open,
	.close = term_close,
	.read = term_read,
	.write = term_write,
	.sync = term_sync,
	.clear = term_clear,
	.set_pos = term_set_pos,
	.get_pos = term_get_pos,
	.get_size = term_get_size,
	.get_color_cap = term_get_color_cap,
	.set_style = term_set_style,
	.set_color = term_set_color,
	.set_rgb_color = term_set_rgb_color,
	.set_cursor_visibility = term_set_cursor_visibility,
	.set_caption = term_set_caption,
	.get_event = term_get_event,
	.map = term_map,
	.unmap = term_unmap,
	.update = term_buf_update
};

static void terminal_close_event(ui_window_t *, void *);
static void terminal_focus_event(ui_window_t *, void *, unsigned);
static void terminal_resize_event(ui_window_t *, void *);
static void terminal_kbd_event(ui_window_t *, void *, kbd_event_t *);
static void terminal_pos_event(ui_window_t *, void *, pos_event_t *);
static void terminal_unfocus_event(ui_window_t *, void *, unsigned);
static void terminal_maximize_event(ui_window_t *, void *);
static void terminal_unmaximize_event(ui_window_t *, void *);

static ui_window_cb_t terminal_window_cb = {
	.close = terminal_close_event,
	.focus = terminal_focus_event,
	.resize = terminal_resize_event,
	.kbd = terminal_kbd_event,
	.pos = terminal_pos_event,
	.unfocus = terminal_unfocus_event,
	.maximize = terminal_maximize_event,
	.unmaximize = terminal_unmaximize_event,
};

static errno_t terminal_wait_fibril(void *);

static terminal_t *srv_to_terminal(con_srv_t *srv)
{
	return srv->srvs->sarg;
}

static errno_t getterm(task_wait_t *wait, const char *svc, const char *app)
{
	return task_spawnl(NULL, wait, APP_GETTERM, APP_GETTERM, svc,
	    LOCFS_MOUNT_POINT, "--msg", "--wait", "--", app, NULL);
}

static pixel_t termui_color_to_pixel(termui_color_t c)
{
	uint8_t r, g, b;
	termui_color_to_rgb(c, &r, &g, &b);
	return PIXEL(255, r, g, b);
}

static termui_color_t termui_color_from_pixel(pixel_t pixel)
{
	return termui_color_from_rgb(RED(pixel), GREEN(pixel), BLUE(pixel));
}

static termui_cell_t charfield_to_termui_cell(terminal_t *term, const charfield_t *cf)
{
	termui_cell_t cell = { };

	cell.glyph_idx = fb_font_glyph(cf->ch, NULL);

	switch (cf->attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (cf->attrs.val.style) {
		case STYLE_NORMAL:
			cell.bgcolor = term->default_bgcolor;
			cell.fgcolor = term->default_fgcolor;
			break;
		case STYLE_EMPHASIS:
			cell.bgcolor = term->emphasis_bgcolor;
			cell.fgcolor = term->emphasis_fgcolor;
			break;
		case STYLE_INVERTED:
			cell.bgcolor = term->default_bgcolor;
			cell.fgcolor = term->default_fgcolor;
			cell.inverted = 1;
			break;
		case STYLE_SELECTED:
			cell.bgcolor = term->selection_bgcolor;
			cell.fgcolor = term->selection_fgcolor;
			break;
		}
		break;

	case CHAR_ATTR_INDEX:
		char_attr_index_t index = cf->attrs.val.index;

		int bright = (index.attr & CATTR_BRIGHT) ? COLOR_BRIGHT : 0;
		pixel_t bgcolor = _basic_colors[index.bgcolor];
		pixel_t fgcolor = _basic_colors[index.fgcolor | bright];
		cell.bgcolor = termui_color_from_pixel(bgcolor);
		cell.fgcolor = termui_color_from_pixel(fgcolor);

		if (index.attr & CATTR_BLINK)
			cell.blink = 1;

		break;

	case CHAR_ATTR_RGB:
		cell.bgcolor = termui_color_from_pixel(cf->attrs.val.rgb.bgcolor);
		cell.fgcolor = termui_color_from_pixel(cf->attrs.val.rgb.fgcolor);
		break;
	}

	return cell;
}

static void term_update_region(terminal_t *term, sysarg_t x, sysarg_t y,
    sysarg_t w, sysarg_t h)
{
	gfx_rect_t rect;
	gfx_rect_t nupdate;

	rect.p0.x = x;
	rect.p0.y = y;
	rect.p1.x = x + w;
	rect.p1.y = y + h;

	gfx_rect_envelope(&term->update, &rect, &nupdate);
	term->update = nupdate;
}

static void term_draw_cell(terminal_t *term, pixelmap_t *pixelmap, int col, int row, const termui_cell_t *cell)
{
	termui_color_t bg = cell->bgcolor;
	if (bg == TERMUI_COLOR_DEFAULT)
		bg = term->default_bgcolor;

	termui_color_t fg = cell->fgcolor;
	if (fg == TERMUI_COLOR_DEFAULT)
		fg = term->default_fgcolor;

	pixel_t bgcolor = termui_color_to_pixel(bg);
	pixel_t fgcolor = termui_color_to_pixel(fg);

	int bx = col * FONT_WIDTH;
	int by = row * FONT_SCANLINES;

	// TODO: support bold/italic/underline/strike/blink styling

	if (cell->inverted ^ cell->cursor) {
		pixel_t tmp = bgcolor;
		bgcolor = fgcolor;
		fgcolor = tmp;
	}

	uint32_t glyph = cell->glyph_idx;
	assert(glyph < FONT_GLYPHS);

	if (glyph == 0)
		glyph = fb_font_glyph(U' ', NULL);

	for (unsigned int y = 0; y < FONT_SCANLINES; y++) {
		pixel_t *dst = pixelmap_pixel_at(pixelmap, bx, by + y);
		pixel_t *dst_max = pixelmap_pixel_at(pixelmap, bx + FONT_WIDTH - 1, by + y);
		if (!dst || !dst_max)
			continue;
		int count = FONT_WIDTH;
		while (count-- != 0) {
			*dst++ = (fb_font[glyph][y] & (1 << count)) ? fgcolor : bgcolor;
		}
	}

	term_update_region(term, bx, by, FONT_WIDTH, FONT_SCANLINES);
}

static void term_render(terminal_t *term)
{
	(void) gfx_bitmap_render(term->bmp, &term->update, &term->off);

	term->update.p0.x = 0;
	term->update.p0.y = 0;
	term->update.p1.x = 0;
	term->update.p1.y = 0;
}

static void termui_refresh_cb(void *userdata)
{
	terminal_t *term = userdata;

	termui_force_viewport_update(term->termui, 0, termui_get_rows(term->termui));
}

static void termui_scroll_cb(void *userdata, int delta)
{
	(void) delta;

	// Until we have support for hardware accelerated scrolling, just redraw everything.
	termui_refresh_cb(userdata);
}

static pixelmap_t term_get_pixelmap(terminal_t *term)
{
	pixelmap_t pixelmap = { };
	gfx_bitmap_alloc_t alloc;

	errno_t rc = gfx_bitmap_get_alloc(term->bmp, &alloc);
	if (rc != EOK)
		return pixelmap;

	pixelmap.width = term->w;
	pixelmap.height = term->h;
	pixelmap.data = alloc.pixels;
	return pixelmap;
}

static void term_clear_bitmap(terminal_t *term, pixel_t color)
{
	pixelmap_t pixelmap = term_get_pixelmap(term);
	if (pixelmap.data == NULL)
		return;

	sysarg_t pixels = pixelmap.height * pixelmap.width;
	for (sysarg_t i = 0; i < pixels; i++)
		pixelmap.data[i] = color;

	term_update_region(term, 0, 0, pixelmap.width, pixelmap.height);
}

static void termui_update_cb(void *userdata, int col, int row, const termui_cell_t *cell, int len)
{
	terminal_t *term = userdata;

	pixelmap_t pixelmap = term_get_pixelmap(term);
	if (pixelmap.data == NULL)
		return;

	for (int i = 0; i < len; i++)
		term_draw_cell(term, &pixelmap, col + i, row, &cell[i]);
}

static errno_t term_open(con_srvs_t *srvs, con_srv_t *srv)
{
	return EOK;
}

static errno_t term_close(con_srv_t *srv)
{
	return EOK;
}

static errno_t term_read(con_srv_t *srv, void *buf, size_t size, size_t *nread)
{
	terminal_t *term = srv_to_terminal(srv);
	uint8_t *bbuf = buf;
	size_t pos = 0;

	/*
	 * Read input from keyboard and copy it to the buffer.
	 * We need to handle situation when wchar is split by 2 following
	 * reads.
	 */
	while (pos < size) {
		/* Copy to the buffer remaining characters. */
		while ((pos < size) && (term->char_remains_len > 0)) {
			bbuf[pos] = term->char_remains[0];
			pos++;

			/* Unshift the array. */
			for (size_t i = 1; i < term->char_remains_len; i++)
				term->char_remains[i - 1] = term->char_remains[i];

			term->char_remains_len--;
		}

		/* Still not enough? Then get another key from the queue. */
		if (pos < size) {
			link_t *link = prodcons_consume(&term->input_pc);
			terminal_event_t *qevent = list_get_instance(link,
			    terminal_event_t, link);
			cons_event_t *event = &qevent->ev;

			/* Accept key presses of printable chars only. */
			if (event->type == CEV_KEY && event->ev.key.type == KEY_PRESS &&
			    event->ev.key.c != 0) {
				char32_t tmp[2] = {
					event->ev.key.c,
					0
				};

				wstr_to_str(term->char_remains, UTF8_CHAR_BUFFER_SIZE, tmp);
				term->char_remains_len = str_size(term->char_remains);
			}

			free(qevent);
		}
	}

	*nread = size;
	return EOK;
}

static void term_write_char(terminal_t *term, wchar_t ch)
{
	switch (ch) {
	case L'\n':
		termui_put_crlf(term->termui);
		break;
	case L'\r':
		termui_put_cr(term->termui);
		break;
	case L'\t':
		termui_put_tab(term->termui);
		break;
	case L'\b':
		termui_put_backspace(term->termui);
		break;
	default:
		// TODO: For some languages, we might need support for combining
		//       characters. Currently, we assume every unicode code point is
		//       an individual printed character, which is not always the case.
		termui_put_glyph(term->termui, fb_font_glyph(ch, NULL), 1);
		break;
	}
}

static errno_t term_write(con_srv_t *srv, void *data, size_t size, size_t *nwritten)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);

	size_t off = 0;
	while (off < size)
		term_write_char(term, str_decode(data, &off, size));

	fibril_mutex_unlock(&term->mtx);

	term_render(term);
	gfx_update(term->gc);
	*nwritten = size;

	return EOK;
}

static void term_sync(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	term_render(term);
	gfx_update(term->gc);
}

static void term_clear(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	termui_clear_screen(term->termui);
	fibril_mutex_unlock(&term->mtx);

	term_render(term);
	gfx_update(term->gc);
}

static void term_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	termui_set_pos(term->termui, col, row);
	fibril_mutex_unlock(&term->mtx);

	term_render(term);
	gfx_update(term->gc);
}

static errno_t term_get_pos(con_srv_t *srv, sysarg_t *col, sysarg_t *row)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	int irow, icol;
	termui_get_pos(term->termui, &icol, &irow);
	fibril_mutex_unlock(&term->mtx);

	*col = icol;
	*row = irow;

	return EOK;
}

static errno_t term_get_size(con_srv_t *srv, sysarg_t *cols, sysarg_t *rows)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	*cols = termui_get_cols(term->termui);
	*rows = termui_get_rows(term->termui);
	fibril_mutex_unlock(&term->mtx);

	return EOK;
}

static errno_t term_get_color_cap(con_srv_t *srv, console_caps_t *caps)
{
	(void) srv;
	*caps = TERM_CAPS;

	return EOK;
}

static void term_set_style(con_srv_t *srv, console_style_t style)
{
	terminal_t *term = srv_to_terminal(srv);

	termui_cell_t cellstyle = { };

	switch (style) {
	case STYLE_NORMAL:
		cellstyle.bgcolor = term->default_bgcolor;
		cellstyle.fgcolor = term->default_fgcolor;
		break;
	case STYLE_EMPHASIS:
		cellstyle.bgcolor = term->emphasis_bgcolor;
		cellstyle.fgcolor = term->emphasis_fgcolor;
		break;
	case STYLE_INVERTED:
		cellstyle.bgcolor = term->default_bgcolor;
		cellstyle.fgcolor = term->default_fgcolor;
		cellstyle.inverted = 1;
		break;
	case STYLE_SELECTED:
		cellstyle.bgcolor = term->selection_bgcolor;
		cellstyle.fgcolor = term->selection_fgcolor;
		break;
	}

	fibril_mutex_lock(&term->mtx);
	termui_set_style(term->termui, cellstyle);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_color(con_srv_t *srv, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	terminal_t *term = srv_to_terminal(srv);

	int bright = (attr & CATTR_BRIGHT) ? COLOR_BRIGHT : 0;

	termui_cell_t cellstyle = { };
	cellstyle.bgcolor = termui_color_from_pixel(_basic_colors[bgcolor]);
	cellstyle.fgcolor = termui_color_from_pixel(_basic_colors[fgcolor | bright]);

	if (attr & CATTR_BLINK)
		cellstyle.blink = 1;

	fibril_mutex_lock(&term->mtx);
	termui_set_style(term->termui, cellstyle);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_rgb_color(con_srv_t *srv, pixel_t bgcolor,
    pixel_t fgcolor)
{
	terminal_t *term = srv_to_terminal(srv);
	termui_cell_t cellstyle = {
		.bgcolor = termui_color_from_pixel(bgcolor),
		.fgcolor = termui_color_from_pixel(fgcolor),
	};

	fibril_mutex_lock(&term->mtx);
	termui_set_style(term->termui, cellstyle);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_cursor_visibility(con_srv_t *srv, bool visible)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	termui_set_cursor_visibility(term->termui, visible);
	fibril_mutex_unlock(&term->mtx);

	term_render(term);
	gfx_update(term->gc);
}

static errno_t term_set_caption(con_srv_t *srv, const char *caption)
{
	terminal_t *term = srv_to_terminal(srv);
	const char *cap;

	fibril_mutex_lock(&term->mtx);

	if (str_size(caption) > 0)
		cap = caption;
	else
		cap = "Terminal";

	ui_window_set_caption(term->window, cap);
	fibril_mutex_unlock(&term->mtx);

	term_render(term);
	gfx_update(term->gc);
	return EOK;
}

static errno_t term_get_event(con_srv_t *srv, cons_event_t *event)
{
	terminal_t *term = srv_to_terminal(srv);
	link_t *link = prodcons_consume(&term->input_pc);
	terminal_event_t *ev = list_get_instance(link, terminal_event_t, link);

	*event = ev->ev;
	free(ev);
	return EOK;
}

/** Create shared buffer for efficient rendering.
 *
 * @param srv Console server
 * @param cols Number of columns in buffer
 * @param rows Number of rows in buffer
 * @param rbuf Place to store pointer to new sharable buffer
 *
 * @return EOK on sucess or an error code
 */
static errno_t term_map(con_srv_t *srv, sysarg_t cols, sysarg_t rows,
    charfield_t **rbuf)
{
	terminal_t *term = srv_to_terminal(srv);
	void *buf;

	fibril_mutex_lock(&term->mtx);

	if (term->ubuf != NULL) {
		fibril_mutex_unlock(&term->mtx);
		return EBUSY;
	}

	buf = as_area_create(AS_AREA_ANY, cols * rows * sizeof(charfield_t),
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (buf == AS_MAP_FAILED) {
		fibril_mutex_unlock(&term->mtx);
		return ENOMEM;
	}

	term->ucols = cols;
	term->urows = rows;
	term->ubuf = buf;

	/* Scroll back to active screen. */
	termui_history_scroll(term->termui, INT_MAX);

	fibril_mutex_unlock(&term->mtx);

	*rbuf = buf;
	return EOK;
}

/** Delete shared buffer.
 *
 * @param srv Console server
 */
static void term_unmap(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);
	void *buf;

	fibril_mutex_lock(&term->mtx);

	buf = term->ubuf;
	term->ubuf = NULL;

	termui_wipe_screen(term->termui, 0);

	fibril_mutex_unlock(&term->mtx);

	/* Update terminal */
	term_render(term);
	gfx_update(term->gc);

	if (buf != NULL)
		as_area_destroy(buf);
}

/** Update area of terminal from shared buffer.
 *
 * @param srv Console server
 * @param c0 Column coordinate of top-left corner (inclusive)
 * @param r0 Row coordinate of top-left corner (inclusive)
 * @param c1 Column coordinate of bottom-right corner (exclusive)
 * @param r1 Row coordinate of bottom-right corner (exclusive)
 */
static void term_buf_update(con_srv_t *srv, sysarg_t c0, sysarg_t r0,
    sysarg_t c1, sysarg_t r1)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);

	if (term->ubuf == NULL) {
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	/* Make sure we have meaningful coordinates, within bounds */
	c1 = min(c1, term->ucols);
	c1 = min(c1, (sysarg_t) termui_get_cols(term->termui));
	r1 = min(r1, term->urows);
	r1 = min(r1, (sysarg_t) termui_get_rows(term->termui));

	if (c0 >= c1 || r0 >= r1) {
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	/* Update front buffer from user buffer */

	for (sysarg_t row = r0; row < r1; row++) {
		termui_cell_t *cells = termui_get_active_row(term->termui, row);

		for (sysarg_t col = c0; col < c1; col++) {
			cells[col] = charfield_to_termui_cell(term, &term->ubuf[row * term->ucols + col]);
		}

		termui_update_cb(term, c0, row, &cells[c0], c1 - c0);
	}

	fibril_mutex_unlock(&term->mtx);

	/* Update terminal */
	term_render(term);
	gfx_update(term->gc);
}

static errno_t terminal_window_resize(terminal_t *term)
{
	gfx_rect_t rect;
	ui_window_get_app_rect(term->window, &rect);

	int width = rect.p1.x - rect.p0.x;
	int height = rect.p1.y - rect.p0.y;

	if (!term->gc)
		term->gc = ui_window_get_gc(term->window);
	else
		assert(term->gc == ui_window_get_gc(term->window));

	if (!term->ui_res)
		term->ui_res = ui_window_get_res(term->window);
	else
		assert(term->ui_res == ui_window_get_res(term->window));

	gfx_bitmap_t *new_bmp;
	gfx_bitmap_params_t params;
	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = width;
	params.rect.p1.y = height;

	errno_t rc = gfx_bitmap_create(term->gc, &params, NULL, &new_bmp);
	if (rc != EOK) {
		fprintf(stderr, "Error allocating new screen bitmap: %s\n", str_error(rc));
		return rc;
	}

	if (term->bmp) {
		rc = gfx_bitmap_destroy(term->bmp);
		if (rc != EOK)
			fprintf(stderr, "Error deallocating old screen bitmap: %s\n", str_error(rc));
	}

	term->bmp = new_bmp;
	term->w = width;
	term->h = height;

	term_clear_bitmap(term, termui_color_to_pixel(term->default_bgcolor));

	return EOK;
}

void terminal_destroy(terminal_t *term)
{
	list_remove(&term->link);

	termui_destroy(term->termui);

	if (term->ubuf)
		as_area_destroy(term->ubuf);

	ui_destroy(term->ui);
	free(term);
}

static void terminal_queue_cons_event(terminal_t *term, cons_event_t *ev)
{
	/* Got key press/release event */
	terminal_event_t *event =
	    (terminal_event_t *) malloc(sizeof(terminal_event_t));
	if (event == NULL)
		return;

	event->ev = *ev;
	link_initialize(&event->link);

	prodcons_produce(&term->input_pc, &event->link);
}

/** Handle window close event. */
static void terminal_close_event(ui_window_t *window, void *arg)
{
	terminal_t *term = (terminal_t *) arg;

	ui_quit(term->ui);
}

/** Handle window focus event. */
static void terminal_focus_event(ui_window_t *window, void *arg,
    unsigned nfocus)
{
	terminal_t *term = (terminal_t *) arg;

	(void)nfocus;
	term->is_focused = true;
	term_render(term);
	gfx_update(term->gc);
}

static void terminal_resize_handler(ui_window_t *window, void *arg)
{
	terminal_t *term = (terminal_t *) arg;

	fibril_mutex_lock(&term->mtx);

	errno_t rc = terminal_window_resize(term);
	if (rc == EOK) {
		(void) termui_resize(term->termui, term->w / FONT_WIDTH, term->h / FONT_SCANLINES, SCROLLBACK_MAX_LINES);
		termui_refresh_cb(term);
		term_render(term);
		gfx_update(term->gc);

		cons_event_t event = { .type = CEV_RESIZE };
		terminal_queue_cons_event(term, &event);
	}

	fibril_mutex_unlock(&term->mtx);
}

static void terminal_resize_event(ui_window_t *window, void *arg)
{
	ui_window_def_resize(window);
	terminal_resize_handler(window, arg);
}

static void terminal_maximize_event(ui_window_t *window, void *arg)
{
	ui_window_def_maximize(window);
	terminal_resize_handler(window, arg);
}

static void terminal_unmaximize_event(ui_window_t *window, void *arg)
{
	ui_window_def_unmaximize(window);
	terminal_resize_handler(window, arg);
}

/** Handle window keyboard event */
static void terminal_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *kbd_event)
{
	terminal_t *term = (terminal_t *) arg;
	cons_event_t event;

	event.type = CEV_KEY;
	event.ev.key = *kbd_event;

	const int PAGE_ROWS = (termui_get_rows(term->termui) * 2) / 3;

	fibril_mutex_lock(&term->mtx);

	if (!term->ubuf && kbd_event->type == KEY_PRESS &&
	    (kbd_event->key == KC_PAGE_UP || kbd_event->key == KC_PAGE_DOWN)) {

		termui_history_scroll(term->termui,
		    (kbd_event->key == KC_PAGE_UP) ? -PAGE_ROWS : PAGE_ROWS);

		term_render(term);
		gfx_update(term->gc);
	} else {
		terminal_queue_cons_event(term, &event);
	}

	fibril_mutex_unlock(&term->mtx);
}

/** Handle window position event */
static void terminal_pos_event(ui_window_t *window, void *arg, pos_event_t *event)
{
	cons_event_t cevent;
	terminal_t *term = (terminal_t *) arg;

	switch (event->type) {
	case POS_UPDATE:
		return;

	case POS_PRESS:
	case POS_RELEASE:
	case POS_DCLICK:
	}

	/* Ignore mouse events when we're in scrollback mode. */
	if (termui_scrollback_is_active(term->termui))
		return;

	sysarg_t sx = term->off.x;
	sysarg_t sy = term->off.y;

	if (event->hpos < sx || event->vpos < sy)
		return;

	cevent.type = CEV_POS;
	cevent.ev.pos.type = event->type;
	cevent.ev.pos.pos_id = event->pos_id;
	cevent.ev.pos.btn_num = event->btn_num;

	cevent.ev.pos.hpos = (event->hpos - sx) / FONT_WIDTH;
	cevent.ev.pos.vpos = (event->vpos - sy) / FONT_SCANLINES;

	/* Filter out events outside the terminal area. */
	int cols = termui_get_cols(term->termui);
	int rows = termui_get_rows(term->termui);

	if (cevent.ev.pos.hpos < (sysarg_t) cols && cevent.ev.pos.vpos < (sysarg_t) rows)
		terminal_queue_cons_event(term, &cevent);
}

/** Handle window unfocus event. */
static void terminal_unfocus_event(ui_window_t *window, void *arg,
    unsigned nfocus)
{
	terminal_t *term = (terminal_t *) arg;

	if (nfocus == 0) {
		term->is_focused = false;
		term_render(term);
		gfx_update(term->gc);
	}
}

static void term_connection(ipc_call_t *icall, void *arg)
{
	terminal_t *term = NULL;

	list_foreach(terms, link, terminal_t, cur) {
		if (cur->dsid == (service_id_t) ipc_get_arg2(icall)) {
			term = cur;
			break;
		}
	}

	if (term == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	if (!atomic_flag_test_and_set(&term->refcnt))
		termui_set_cursor_visibility(term->termui, true);

	con_conn(icall, &term->srvs);
}

static errno_t term_init_window(terminal_t *term, const char *display_spec,
    gfx_coord_t width, gfx_coord_t height,
    gfx_coord_t min_width, gfx_coord_t min_height,
    terminal_flags_t flags)
{
	gfx_rect_t min_rect = { { 0, 0 }, { min_width, min_height } };
	gfx_rect_t wmin_rect;
	gfx_rect_t wrect;

	errno_t rc = ui_create(display_spec, &term->ui);
	if (rc != EOK) {
		printf("Error creating UI on %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_t wparams;
	ui_wnd_params_init(&wparams);
	wparams.caption = "Terminal";
	wparams.style |= ui_wds_maximize_btn | ui_wds_resizable;

	if ((flags & tf_topleft) != 0)
		wparams.placement = ui_wnd_place_top_left;

	if (ui_is_fullscreen(term->ui)) {
		wparams.placement = ui_wnd_place_full_screen;
		wparams.style &= ~ui_wds_decorated;
	}

	/* Compute wrect such that application area corresponds to rect. */
	ui_wdecor_rect_from_app(term->ui, wparams.style, &min_rect, &wrect);
	gfx_rect_rtranslate(&wrect.p0, &wrect, &wmin_rect);
	wparams.min_size = wmin_rect.p1;

	gfx_rect_t rect = { { 0, 0 }, { width, height } };
	ui_wdecor_rect_from_app(term->ui, wparams.style, &rect, &rect);
	term->off.x = -rect.p0.x;
	term->off.y = -rect.p0.y;
	printf("off=%d,%d\n", term->off.x, term->off.y);
	gfx_rect_translate(&term->off, &rect, &wparams.rect);
	printf("wparams.rect=%d,%d,%d,%d\n",
	    wparams.rect.p0.x,
	    wparams.rect.p1.x,
	    wparams.rect.p0.y,
	    wparams.rect.p1.y);

	rc = ui_window_create(term->ui, &wparams, &term->window);
	if (rc != EOK)
		return rc;

	ui_window_set_cb(term->window, &terminal_window_cb, (void *) term);
	return terminal_window_resize(term);
}

errno_t terminal_create(const char *display_spec, sysarg_t width,
    sysarg_t height, terminal_flags_t flags, const char *command,
    terminal_t **rterm)
{
	errno_t rc;

	terminal_t *term = calloc(1, sizeof(terminal_t));
	if (term == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	link_initialize(&term->link);
	fibril_mutex_initialize(&term->mtx);
	atomic_flag_clear(&term->refcnt);

	prodcons_initialize(&term->input_pc);
	term->char_remains_len = 0;

	term->default_bgcolor = termui_color_from_pixel(_basic_colors[COLOR_WHITE | COLOR_BRIGHT]);
	term->default_fgcolor = termui_color_from_pixel(_basic_colors[COLOR_BLACK]);

	term->emphasis_bgcolor = termui_color_from_pixel(_basic_colors[COLOR_WHITE | COLOR_BRIGHT]);
	term->emphasis_fgcolor = termui_color_from_pixel(_basic_colors[COLOR_RED | COLOR_BRIGHT]);

	term->selection_bgcolor = termui_color_from_pixel(_basic_colors[COLOR_RED | COLOR_BRIGHT]);
	term->selection_fgcolor = termui_color_from_pixel(_basic_colors[COLOR_WHITE | COLOR_BRIGHT]);

	rc = term_init_window(term, display_spec, width, height,
	    MIN_WINDOW_COLS * FONT_WIDTH, MIN_WINDOW_ROWS * FONT_SCANLINES, flags);
	if (rc != EOK) {
		printf("Error creating window (%s).\n", str_error(rc));
		goto error;
	}

	term->termui = termui_create(term->w / FONT_WIDTH,
	    term->h / FONT_SCANLINES, SCROLLBACK_MAX_LINES);
	if (!term->termui) {
		printf("Error creating terminal UI.\n");
		rc = ENOMEM;
		goto error;
	}

	termui_set_refresh_cb(term->termui, termui_refresh_cb, term);
	termui_set_scroll_cb(term->termui, termui_scroll_cb, term);
	termui_set_update_cb(term->termui, termui_update_cb, term);

	async_set_fallback_port_handler(term_connection, NULL);
	con_srvs_init(&term->srvs);
	term->srvs.ops = &con_ops;
	term->srvs.sarg = term;

	rc = loc_server_register(NAME, &term->srv);
	if (rc != EOK) {
		printf("Error registering server.\n");
		rc = EIO;
		goto error;
	}

	char vc[LOC_NAME_MAXLEN + 1];
	snprintf(vc, LOC_NAME_MAXLEN, "%s/%" PRIu64, NAMESPACE,
	    task_get_id());

	rc = loc_service_register(term->srv, vc, &term->dsid);
	if (rc != EOK) {
		printf("Error registering service.\n");
		rc = EIO;
		goto error;
	}

	list_append(&term->link, &terms);
	rc = getterm(&term->wait, vc, command);
	if (rc != EOK)
		goto error;

	term->wfid = fibril_create(terminal_wait_fibril, term);
	if (term->wfid == 0)
		goto error;

	fibril_add_ready(term->wfid);

	term->is_focused = true;

	termui_refresh_cb(term);

	*rterm = term;
	return EOK;
error:
	if (term->dsid != 0)
		loc_service_unregister(term->srv, term->dsid);
	if (term->srv != NULL)
		loc_server_unregister(term->srv);
	if (term->window != NULL)
		ui_window_destroy(term->window);
	if (term->ui != NULL)
		ui_destroy(term->ui);
	if (term->termui != NULL)
		termui_destroy(term->termui);
	free(term);
	return rc;
}

static errno_t terminal_wait_fibril(void *arg)
{
	terminal_t *term = (terminal_t *)arg;
	task_exit_t texit;
	int retval;

	/*
	 * XXX There is no way to break the sleep if the task does not
	 * exit.
	 */
	(void) task_wait(&term->wait, &texit, &retval);
	ui_quit(term->ui);
	return EOK;
}

/** @}
 */
