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
#include <io/chargrid.h>
#include <fibril.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/con_srv.h>
#include <io/concaps.h>
#include <io/console.h>
#include <io/pixelmap.h>
#include <task.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
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
	(CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED | CONSOLE_CAP_RGB)

static LIST_INITIALIZE(terms);

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
static void terminal_kbd_event(ui_window_t *, void *, kbd_event_t *);
static void terminal_pos_event(ui_window_t *, void *, pos_event_t *);
static void terminal_unfocus_event(ui_window_t *, void *, unsigned);

static ui_window_cb_t terminal_window_cb = {
	.close = terminal_close_event,
	.focus = terminal_focus_event,
	.kbd = terminal_kbd_event,
	.pos = terminal_pos_event,
	.unfocus = terminal_unfocus_event
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

static pixel_t color_table[16] = {
	[COLOR_BLACK]       = PIXEL(255, 0, 0, 0),
	[COLOR_BLUE]        = PIXEL(255, 0, 0, 170),
	[COLOR_GREEN]       = PIXEL(255, 0, 170, 0),
	[COLOR_CYAN]        = PIXEL(255, 0, 170, 170),
	[COLOR_RED]         = PIXEL(255, 170, 0, 0),
	[COLOR_MAGENTA]     = PIXEL(255, 170, 0, 170),
	[COLOR_YELLOW]      = PIXEL(255, 170, 85, 0),
	[COLOR_WHITE]       = PIXEL(255, 170, 170, 170),

	[COLOR_BLACK + 8]   = PIXEL(255, 85, 85, 85),
	[COLOR_BLUE + 8]    = PIXEL(255, 85, 85, 255),
	[COLOR_GREEN + 8]   = PIXEL(255, 85, 255, 85),
	[COLOR_CYAN + 8]    = PIXEL(255, 85, 255, 255),
	[COLOR_RED + 8]     = PIXEL(255, 255, 85, 85),
	[COLOR_MAGENTA + 8] = PIXEL(255, 255, 85, 255),
	[COLOR_YELLOW + 8]  = PIXEL(255, 255, 255, 85),
	[COLOR_WHITE + 8]   = PIXEL(255, 255, 255, 255),
};

static inline void attrs_rgb(char_attrs_t attrs, pixel_t *bgcolor, pixel_t *fgcolor)
{
	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			*bgcolor = color_table[COLOR_WHITE + 8];
			*fgcolor = color_table[COLOR_BLACK];
			break;
		case STYLE_EMPHASIS:
			*bgcolor = color_table[COLOR_WHITE + 8];
			*fgcolor = color_table[COLOR_RED + 8];
			break;
		case STYLE_INVERTED:
			*bgcolor = color_table[COLOR_BLACK];
			*fgcolor = color_table[COLOR_WHITE + 8];
			break;
		case STYLE_SELECTED:
			*bgcolor = color_table[COLOR_RED + 8];
			*fgcolor = color_table[COLOR_WHITE + 8];
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		*bgcolor = color_table[(attrs.val.index.bgcolor & 7)];
		*fgcolor = color_table[(attrs.val.index.fgcolor & 7) |
		    ((attrs.val.index.attr & CATTR_BRIGHT) ? 8 : 0)];
		break;
	case CHAR_ATTR_RGB:
		*bgcolor = 0xff000000 | attrs.val.rgb.bgcolor;
		*fgcolor = 0xff000000 | attrs.val.rgb.fgcolor;
		break;
	}
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

static void term_update_char(terminal_t *term, pixelmap_t *pixelmap,
    sysarg_t col, sysarg_t row)
{
	charfield_t *field =
	    chargrid_charfield_at(term->backbuf, col, row);

	bool inverted = chargrid_cursor_at(term->backbuf, col, row);

	sysarg_t bx = col * FONT_WIDTH;
	sysarg_t by = row * FONT_SCANLINES;

	pixel_t bgcolor = 0;
	pixel_t fgcolor = 0;

	if (inverted)
		attrs_rgb(field->attrs, &fgcolor, &bgcolor);
	else
		attrs_rgb(field->attrs, &bgcolor, &fgcolor);

	// FIXME: Glyph type should be actually uint32_t
	//        for full UTF-32 coverage.

	uint16_t glyph = fb_font_glyph(field->ch, NULL);

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

static bool term_update_scroll(terminal_t *term, pixelmap_t *pixelmap)
{
	sysarg_t top_row = chargrid_get_top_row(term->frontbuf);

	if (term->top_row == top_row) {
		return false;
	}

	term->top_row = top_row;

	for (sysarg_t row = 0; row < term->rows; row++) {
		for (sysarg_t col = 0; col < term->cols; col++) {
			charfield_t *front_field =
			    chargrid_charfield_at(term->frontbuf, col, row);
			charfield_t *back_field =
			    chargrid_charfield_at(term->backbuf, col, row);
			bool update = false;

			if (front_field->ch != back_field->ch) {
				back_field->ch = front_field->ch;
				update = true;
			}

			if (!attrs_same(front_field->attrs, back_field->attrs)) {
				back_field->attrs = front_field->attrs;
				update = true;
			}

			front_field->flags &= ~CHAR_FLAG_DIRTY;

			if (update) {
				term_update_char(term, pixelmap, col, row);
			}
		}
	}

	return true;
}

static bool term_update_cursor(terminal_t *term, pixelmap_t *pixelmap)
{
	bool update = false;

	sysarg_t front_col;
	sysarg_t front_row;
	chargrid_get_cursor(term->frontbuf, &front_col, &front_row);

	sysarg_t back_col;
	sysarg_t back_row;
	chargrid_get_cursor(term->backbuf, &back_col, &back_row);

	bool front_visibility =
	    chargrid_get_cursor_visibility(term->frontbuf) &&
	    term->is_focused;
	bool back_visibility =
	    chargrid_get_cursor_visibility(term->backbuf);

	if (front_visibility != back_visibility) {
		chargrid_set_cursor_visibility(term->backbuf,
		    front_visibility);
		term_update_char(term, pixelmap, back_col, back_row);
		update = true;
	}

	if ((front_col != back_col) || (front_row != back_row)) {
		chargrid_set_cursor(term->backbuf, front_col, front_row);
		term_update_char(term, pixelmap, back_col, back_row);
		term_update_char(term, pixelmap, front_col, front_row);
		update = true;
	}

	return update;
}

static void term_update(terminal_t *term)
{
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t pos;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(term->bmp, &alloc);
	if (rc != EOK) {
		return;
	}

	fibril_mutex_lock(&term->mtx);
	pixelmap.width = term->w;
	pixelmap.height = term->h;
	pixelmap.data = alloc.pixels;

	bool update = false;

	if (term_update_scroll(term, &pixelmap)) {
		update = true;
	} else {
		for (sysarg_t y = 0; y < term->rows; y++) {
			for (sysarg_t x = 0; x < term->cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(term->frontbuf, x, y);
				charfield_t *back_field =
				    chargrid_charfield_at(term->backbuf, x, y);
				bool cupdate = false;

				if ((front_field->flags & CHAR_FLAG_DIRTY) ==
				    CHAR_FLAG_DIRTY) {
					if (front_field->ch != back_field->ch) {
						back_field->ch = front_field->ch;
						cupdate = true;
					}

					if (!attrs_same(front_field->attrs,
					    back_field->attrs)) {
						back_field->attrs = front_field->attrs;
						cupdate = true;
					}

					front_field->flags &= ~CHAR_FLAG_DIRTY;
				}

				if (cupdate) {
					term_update_char(term, &pixelmap, x, y);
					update = true;
				}
			}
		}
	}

	if (term_update_cursor(term, &pixelmap))
		update = true;

	if (update) {
		pos.x = 4;
		pos.y = 26;
		(void) gfx_bitmap_render(term->bmp, &term->update, &pos);

		term->update.p0.x = 0;
		term->update.p0.y = 0;
		term->update.p1.x = 0;
		term->update.p1.y = 0;
	}

	fibril_mutex_unlock(&term->mtx);
}

static void term_repaint(terminal_t *term)
{
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(term->bmp, &alloc);
	if (rc != EOK) {
		printf("Error getting bitmap allocation info.\n");
		return;
	}

	fibril_mutex_lock(&term->mtx);

	pixelmap.width = term->w;
	pixelmap.height = term->h;
	pixelmap.data = alloc.pixels;

	if (!term_update_scroll(term, &pixelmap)) {
		for (sysarg_t y = 0; y < term->rows; y++) {
			for (sysarg_t x = 0; x < term->cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(term->frontbuf, x, y);
				charfield_t *back_field =
				    chargrid_charfield_at(term->backbuf, x, y);

				back_field->ch = front_field->ch;
				back_field->attrs = front_field->attrs;
				front_field->flags &= ~CHAR_FLAG_DIRTY;

				term_update_char(term, &pixelmap, x, y);
			}
		}
	}

	term_update_cursor(term, &pixelmap);

	fibril_mutex_unlock(&term->mtx);
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
			cons_event_t *event = list_get_instance(link, cons_event_t, link);

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

			free(event);
		}
	}

	*nread = size;
	return EOK;
}

static void term_write_char(terminal_t *term, wchar_t ch)
{
	sysarg_t updated = 0;

	fibril_mutex_lock(&term->mtx);

	switch (ch) {
	case '\n':
		updated = chargrid_newline(term->frontbuf);
		break;
	case '\r':
		break;
	case '\t':
		updated = chargrid_tabstop(term->frontbuf, 8);
		break;
	case '\b':
		updated = chargrid_backspace(term->frontbuf);
		break;
	default:
		updated = chargrid_putuchar(term->frontbuf, ch, true);
	}

	fibril_mutex_unlock(&term->mtx);

	if (updated > 1)
		term_update(term);
}

static errno_t term_write(con_srv_t *srv, void *data, size_t size, size_t *nwritten)
{
	terminal_t *term = srv_to_terminal(srv);

	size_t off = 0;
	while (off < size)
		term_write_char(term, str_decode(data, &off, size));

	gfx_update(term->gc);
	*nwritten = size;
	return EOK;
}

static void term_sync(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	term_update(term);
	gfx_update(term->gc);
}

static void term_clear(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_clear(term->frontbuf);
	fibril_mutex_unlock(&term->mtx);

	term_update(term);
	gfx_update(term->gc);
}

static void term_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_set_cursor(term->frontbuf, col, row);
	fibril_mutex_unlock(&term->mtx);

	term_update(term);
	gfx_update(term->gc);
}

static errno_t term_get_pos(con_srv_t *srv, sysarg_t *col, sysarg_t *row)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_get_cursor(term->frontbuf, col, row);
	fibril_mutex_unlock(&term->mtx);

	return EOK;
}

static errno_t term_get_size(con_srv_t *srv, sysarg_t *cols, sysarg_t *rows)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	*cols = term->cols;
	*rows = term->rows;
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

	fibril_mutex_lock(&term->mtx);
	chargrid_set_style(term->frontbuf, style);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_color(con_srv_t *srv, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_set_color(term->frontbuf, bgcolor, fgcolor, attr);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_rgb_color(con_srv_t *srv, pixel_t bgcolor,
    pixel_t fgcolor)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_set_rgb_color(term->frontbuf, bgcolor, fgcolor);
	fibril_mutex_unlock(&term->mtx);
}

static void term_set_cursor_visibility(con_srv_t *srv, bool visible)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_set_cursor_visibility(term->frontbuf, visible);
	fibril_mutex_unlock(&term->mtx);

	term_update(term);
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

	term_update(term);
	gfx_update(term->gc);
	return EOK;
}

static errno_t term_get_event(con_srv_t *srv, cons_event_t *event)
{
	terminal_t *term = srv_to_terminal(srv);
	link_t *link = prodcons_consume(&term->input_pc);
	cons_event_t *ev = list_get_instance(link, cons_event_t, link);

	*event = *ev;
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

	if (buf != NULL)
		as_area_destroy(buf);

	fibril_mutex_unlock(&term->mtx);
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
	charfield_t *ch;
	sysarg_t col, row;

	fibril_mutex_lock(&term->mtx);

	if (term->ubuf == NULL) {
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	/* Make sure we have meaningful coordinates, within bounds */

	if (c1 > term->ucols)
		c1 = term->ucols;
	if (c1 > term->cols)
		c1 = term->cols;
	if (c0 >= c1) {
		fibril_mutex_unlock(&term->mtx);
		return;
	}
	if (r1 > term->urows)
		r1 = term->urows;
	if (r1 > term->rows)
		r1 = term->rows;
	if (r0 >= r1) {
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	/* Update front buffer from user buffer */

	for (row = r0; row < r1; row++) {
		for (col = c0; col < c1; col++) {
			ch = chargrid_charfield_at(term->frontbuf, col, row);
			*ch = term->ubuf[row * term->ucols + col];
		}
	}

	fibril_mutex_unlock(&term->mtx);

	/* Update terminal */
	term_update(term);
	gfx_update(term->gc);
}

static void deinit_terminal(terminal_t *term)
{
	list_remove(&term->link);

	if (term->frontbuf)
		chargrid_destroy(term->frontbuf);

	if (term->backbuf)
		chargrid_destroy(term->backbuf);
}

void terminal_destroy(terminal_t *term)
{
	deinit_terminal(term);
	free(term);
}

static void terminal_queue_cons_event(terminal_t *term, cons_event_t *ev)
{
	/* Got key press/release event */
	cons_event_t *event =
	    (cons_event_t *) malloc(sizeof(cons_event_t));
	if (event == NULL)
		return;

	*event = *ev;
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
	term_update(term);
	gfx_update(term->gc);
}

/** Handle window keyboard event */
static void terminal_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *kbd_event)
{
	terminal_t *term = (terminal_t *) arg;
	cons_event_t event;

	event.type = CEV_KEY;
	event.ev.key = *kbd_event;

	terminal_queue_cons_event(term, &event);
}

/** Handle window position event */
static void terminal_pos_event(ui_window_t *window, void *arg, pos_event_t *event)
{
	cons_event_t cevent;
	terminal_t *term = (terminal_t *) arg;

	sysarg_t sx = -term->off.x;
	sysarg_t sy = -term->off.y;

	if (event->type == POS_PRESS || event->type == POS_RELEASE ||
	    event->type == POS_DCLICK) {
		cevent.type = CEV_POS;
		cevent.ev.pos.type = event->type;
		cevent.ev.pos.pos_id = event->pos_id;
		cevent.ev.pos.btn_num = event->btn_num;

		cevent.ev.pos.hpos = (event->hpos - sx) / FONT_WIDTH;
		cevent.ev.pos.vpos = (event->vpos - sy) / FONT_SCANLINES;
		terminal_queue_cons_event(term, &cevent);
	}
}

/** Handle window unfocus event. */
static void terminal_unfocus_event(ui_window_t *window, void *arg,
    unsigned nfocus)
{
	terminal_t *term = (terminal_t *) arg;

	if (nfocus == 0) {
		term->is_focused = false;
		term_update(term);
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
		chargrid_set_cursor_visibility(term->frontbuf, true);

	con_conn(icall, &term->srvs);
}

errno_t terminal_create(const char *display_spec, sysarg_t width,
    sysarg_t height, terminal_flags_t flags, const char *command,
    terminal_t **rterm)
{
	terminal_t *term;
	gfx_bitmap_params_t params;
	ui_wnd_params_t wparams;
	gfx_rect_t rect;
	gfx_coord2_t off;
	gfx_rect_t wrect;
	errno_t rc;

	term = calloc(1, sizeof(terminal_t));
	if (term == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	link_initialize(&term->link);
	fibril_mutex_initialize(&term->mtx);
	atomic_flag_clear(&term->refcnt);

	prodcons_initialize(&term->input_pc);
	term->char_remains_len = 0;

	term->w = width;
	term->h = height;

	term->cols = width / FONT_WIDTH;
	term->rows = height / FONT_SCANLINES;

	term->frontbuf = NULL;
	term->backbuf = NULL;

	term->frontbuf = chargrid_create(term->cols, term->rows,
	    CHARGRID_FLAG_NONE);
	if (!term->frontbuf) {
		printf("Error creating front buffer.\n");
		rc = ENOMEM;
		goto error;
	}

	term->backbuf = chargrid_create(term->cols, term->rows,
	    CHARGRID_FLAG_NONE);
	if (!term->backbuf) {
		printf("Error creating back buffer.\n");
		rc = ENOMEM;
		goto error;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = width;
	rect.p1.y = height;

	ui_wnd_params_init(&wparams);
	wparams.caption = "Terminal";
	if ((flags & tf_topleft) != 0)
		wparams.placement = ui_wnd_place_top_left;

	rc = ui_create(display_spec, &term->ui);
	if (rc != EOK) {
		printf("Error creating UI on %s.\n", display_spec);
		goto error;
	}

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(term->ui, wparams.style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &wparams.rect);

	term->off = off;

	rc = ui_window_create(term->ui, &wparams, &term->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	term->gc = ui_window_get_gc(term->window);
	term->ui_res = ui_window_get_res(term->window);

	ui_window_set_cb(term->window, &terminal_window_cb, (void *) term);

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = width;
	params.rect.p1.y = height;

	rc = gfx_bitmap_create(term->gc, &params, NULL, &term->bmp);
	if (rc != EOK) {
		printf("Error allocating screen bitmap.\n");
		goto error;
	}

	chargrid_clear(term->frontbuf);
	chargrid_clear(term->backbuf);
	term->top_row = 0;

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

	term->update.p0.x = 0;
	term->update.p0.y = 0;
	term->update.p1.x = 0;
	term->update.p1.y = 0;

	term_repaint(term);

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
	if (term->frontbuf != NULL)
		chargrid_destroy(term->frontbuf);
	if (term->backbuf != NULL)
		chargrid_destroy(term->backbuf);
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
