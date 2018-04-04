/*
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

/** @addtogroup gui
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <stdlib.h>
#include <io/chargrid.h>
#include <surface.h>
#include <gfx/font-8x16.h>
#include <io/con_srv.h>
#include <io/concaps.h>
#include <io/console.h>
#include <task.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <atomic.h>
#include <stdarg.h>
#include <str.h>
#include "window.h"
#include "terminal.h"

#define NAME       "vterm"
#define NAMESPACE  "vterm"

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
static errno_t term_get_event(con_srv_t *, cons_event_t *);

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
	.get_event = term_get_event
};

static terminal_t *srv_to_terminal(con_srv_t *srv)
{
	return srv->srvs->sarg;
}

static void getterm(const char *svc, const char *app)
{
	task_spawnl(NULL, NULL, APP_GETTERM, APP_GETTERM, svc,
	    LOCFS_MOUNT_POINT, "--msg", "--wait", "--", app, NULL);
}

static pixel_t color_table[16] = {
	[COLOR_BLACK]       = PIXEL(255, 0, 0, 0),
	[COLOR_BLUE]        = PIXEL(255, 0, 0, 240),
	[COLOR_GREEN]       = PIXEL(255, 0, 240, 0),
	[COLOR_CYAN]        = PIXEL(255, 0, 240, 240),
	[COLOR_RED]         = PIXEL(255, 240, 0, 0),
	[COLOR_MAGENTA]     = PIXEL(255, 240, 0, 240),
	[COLOR_YELLOW]      = PIXEL(255, 240, 240, 0),
	[COLOR_WHITE]       = PIXEL(255, 240, 240, 240),

	[COLOR_BLACK + 8]   = PIXEL(255, 0, 0, 0),
	[COLOR_BLUE + 8]    = PIXEL(255, 0, 0, 255),
	[COLOR_GREEN + 8]   = PIXEL(255, 0, 255, 0),
	[COLOR_CYAN + 8]    = PIXEL(255, 0, 255, 255),
	[COLOR_RED + 8]     = PIXEL(255, 255, 0, 0),
	[COLOR_MAGENTA + 8] = PIXEL(255, 255, 0, 255),
	[COLOR_YELLOW + 8]  = PIXEL(255, 255, 255, 0),
	[COLOR_WHITE + 8]   = PIXEL(255, 255, 255, 255),
};

static inline void attrs_rgb(char_attrs_t attrs, pixel_t *bgcolor, pixel_t *fgcolor)
{
	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			*bgcolor = color_table[COLOR_WHITE];
			*fgcolor = color_table[COLOR_BLACK];
			break;
		case STYLE_EMPHASIS:
			*bgcolor = color_table[COLOR_WHITE];
			*fgcolor = color_table[COLOR_RED];
			break;
		case STYLE_INVERTED:
			*bgcolor = color_table[COLOR_BLACK];
			*fgcolor = color_table[COLOR_WHITE];
			break;
		case STYLE_SELECTED:
			*bgcolor = color_table[COLOR_RED];
			*fgcolor = color_table[COLOR_WHITE];
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		*bgcolor = color_table[(attrs.val.index.bgcolor & 7) |
		    ((attrs.val.index.attr & CATTR_BRIGHT) ? 8 : 0)];
		*fgcolor = color_table[(attrs.val.index.fgcolor & 7) |
		    ((attrs.val.index.attr & CATTR_BRIGHT) ? 8 : 0)];
		break;
	case CHAR_ATTR_RGB:
		*bgcolor = 0xff000000 | attrs.val.rgb.bgcolor;
		*fgcolor = 0xff000000 | attrs.val.rgb.fgcolor;
		break;
	}
}

static void term_update_char(terminal_t *term, surface_t *surface,
    sysarg_t sx, sysarg_t sy, sysarg_t col, sysarg_t row)
{
	charfield_t *field =
	    chargrid_charfield_at(term->backbuf, col, row);

	bool inverted = chargrid_cursor_at(term->backbuf, col, row);

	sysarg_t bx = sx + (col * FONT_WIDTH);
	sysarg_t by = sy + (row * FONT_SCANLINES);

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
		pixel_t *dst = pixelmap_pixel_at(
		    surface_pixmap_access(surface), bx, by + y);
		pixel_t *dst_max = pixelmap_pixel_at(
		    surface_pixmap_access(surface), bx + FONT_WIDTH - 1, by + y);
		if (!dst || !dst_max)
			continue;
		int count = FONT_WIDTH;
		while (count-- != 0) {
			*dst++ = (fb_font[glyph][y] & (1 << count)) ? fgcolor : bgcolor;
		}
	}
	surface_add_damaged_region(surface, bx, by, FONT_WIDTH, FONT_SCANLINES);
}

static bool term_update_scroll(terminal_t *term, surface_t *surface,
    sysarg_t sx, sysarg_t sy)
{
	sysarg_t top_row = chargrid_get_top_row(term->frontbuf);

	if (term->top_row == top_row)
		return false;

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

			if (update)
				term_update_char(term, surface, sx, sy, col, row);
		}
	}

	return true;
}

static bool term_update_cursor(terminal_t *term, surface_t *surface,
    sysarg_t sx, sysarg_t sy)
{
	bool damage = false;

	sysarg_t front_col;
	sysarg_t front_row;
	chargrid_get_cursor(term->frontbuf, &front_col, &front_row);

	sysarg_t back_col;
	sysarg_t back_row;
	chargrid_get_cursor(term->backbuf, &back_col, &back_row);

	bool front_visibility =
	    chargrid_get_cursor_visibility(term->frontbuf) &&
	    term->widget.window->is_focused;
	bool back_visibility =
	    chargrid_get_cursor_visibility(term->backbuf);

	if (front_visibility != back_visibility) {
		chargrid_set_cursor_visibility(term->backbuf,
		    front_visibility);
		term_update_char(term, surface, sx, sy, back_col, back_row);
		damage = true;
	}

	if ((front_col != back_col) || (front_row != back_row)) {
		chargrid_set_cursor(term->backbuf, front_col, front_row);
		term_update_char(term, surface, sx, sy, back_col, back_row);
		term_update_char(term, surface, sx, sy, front_col, front_row);
		damage = true;
	}

	return damage;
}

static void term_update(terminal_t *term)
{
	fibril_mutex_lock(&term->mtx);

	surface_t *surface = window_claim(term->widget.window);
	if (!surface) {
		window_yield(term->widget.window);
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	bool damage = false;
	sysarg_t sx = term->widget.hpos;
	sysarg_t sy = term->widget.vpos;

	if (term_update_scroll(term, surface, sx, sy)) {
		damage = true;
	} else {
		for (sysarg_t y = 0; y < term->rows; y++) {
			for (sysarg_t x = 0; x < term->cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(term->frontbuf, x, y);
				charfield_t *back_field =
				    chargrid_charfield_at(term->backbuf, x, y);
				bool update = false;

				if ((front_field->flags & CHAR_FLAG_DIRTY) ==
				    CHAR_FLAG_DIRTY) {
					if (front_field->ch != back_field->ch) {
						back_field->ch = front_field->ch;
						update = true;
					}

					if (!attrs_same(front_field->attrs,
					    back_field->attrs)) {
						back_field->attrs = front_field->attrs;
						update = true;
					}

					front_field->flags &= ~CHAR_FLAG_DIRTY;
				}

				if (update) {
					term_update_char(term, surface, sx, sy, x, y);
					damage = true;
				}
			}
		}
	}

	if (term_update_cursor(term, surface, sx, sy))
		damage = true;

	window_yield(term->widget.window);

	if (damage)
		window_damage(term->widget.window);

	fibril_mutex_unlock(&term->mtx);
}

static void term_damage(terminal_t *term)
{
	fibril_mutex_lock(&term->mtx);

	surface_t *surface = window_claim(term->widget.window);
	if (!surface) {
		window_yield(term->widget.window);
		fibril_mutex_unlock(&term->mtx);
		return;
	}

	sysarg_t sx = term->widget.hpos;
	sysarg_t sy = term->widget.vpos;

	if (!term_update_scroll(term, surface, sx, sy)) {
		for (sysarg_t y = 0; y < term->rows; y++) {
			for (sysarg_t x = 0; x < term->cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(term->frontbuf, x, y);
				charfield_t *back_field =
				    chargrid_charfield_at(term->backbuf, x, y);

				back_field->ch = front_field->ch;
				back_field->attrs = front_field->attrs;
				front_field->flags &= ~CHAR_FLAG_DIRTY;

				term_update_char(term, surface, sx, sy, x, y);
			}
		}
	}

	term_update_cursor(term, surface, sx, sy);

	window_yield(term->widget.window);
	window_damage(term->widget.window);

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
				wchar_t tmp[2] = {
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
		updated = chargrid_putchar(term->frontbuf, ch, true);
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

	*nwritten = size;
	return EOK;
}

static void term_sync(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	term_update(term);
}

static void term_clear(con_srv_t *srv)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_clear(term->frontbuf);
	fibril_mutex_unlock(&term->mtx);

	term_update(term);
}

static void term_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	terminal_t *term = srv_to_terminal(srv);

	fibril_mutex_lock(&term->mtx);
	chargrid_set_cursor(term->frontbuf, col, row);
	fibril_mutex_unlock(&term->mtx);

	term_update(term);
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

void deinit_terminal(terminal_t *term)
{
	list_remove(&term->link);
	widget_deinit(&term->widget);

	if (term->frontbuf)
		chargrid_destroy(term->frontbuf);

	if (term->backbuf)
		chargrid_destroy(term->backbuf);
}

static void terminal_destroy(widget_t *widget)
{
	terminal_t *term = (terminal_t *) widget;

	deinit_terminal(term);
	free(term);
}

static void terminal_reconfigure(widget_t *widget)
{
	/* No-op */
}

static void terminal_rearrange(widget_t *widget, sysarg_t hpos, sysarg_t vpos,
    sysarg_t width, sysarg_t height)
{
	terminal_t *term = (terminal_t *) widget;

	widget_modify(widget, hpos, vpos, width, height);
	widget->width_ideal = width;
	widget->height_ideal = height;

	term_damage(term);
}

static void terminal_repaint(widget_t *widget)
{
	terminal_t *term = (terminal_t *) widget;

	term_damage(term);
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

/* Got key press/release event */
static void terminal_handle_keyboard_event(widget_t *widget,
    kbd_event_t kbd_event)
{
	terminal_t *term = (terminal_t *) widget;
	cons_event_t event;

	event.type = CEV_KEY;
	event.ev.key = kbd_event;

	terminal_queue_cons_event(term, &event);
}

static void terminal_handle_position_event(widget_t *widget, pos_event_t pos_event)
{
	cons_event_t event;
	terminal_t *term = (terminal_t *) widget;
	sysarg_t sx = term->widget.hpos;
	sysarg_t sy = term->widget.vpos;

	if (pos_event.type == POS_PRESS) {
		event.type = CEV_POS;
		event.ev.pos.type = pos_event.type;
		event.ev.pos.pos_id = pos_event.pos_id;
		event.ev.pos.btn_num = pos_event.btn_num;

		event.ev.pos.hpos = (pos_event.hpos - sx) / FONT_WIDTH;
		event.ev.pos.vpos = (pos_event.vpos - sy) / FONT_SCANLINES;
		terminal_queue_cons_event(term, &event);
	}
}

static void term_connection(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	terminal_t *term = NULL;

	list_foreach(terms, link, terminal_t, cur) {
		if (cur->dsid == (service_id_t) IPC_GET_ARG2(*icall)) {
			term = cur;
			break;
		}
	}

	if (term == NULL) {
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	if (atomic_postinc(&term->refcnt) == 0)
		chargrid_set_cursor_visibility(term->frontbuf, true);

	con_conn(icall_handle, icall, &term->srvs);
}

bool init_terminal(terminal_t *term, widget_t *parent, const void *data,
    sysarg_t width, sysarg_t height)
{
	widget_init(&term->widget, parent, data);

	link_initialize(&term->link);
	fibril_mutex_initialize(&term->mtx);
	atomic_set(&term->refcnt, 0);

	prodcons_initialize(&term->input_pc);
	term->char_remains_len = 0;

	term->widget.width = width;
	term->widget.height = height;
	term->widget.width_ideal = width;
	term->widget.height_ideal = height;

	term->widget.destroy = terminal_destroy;
	term->widget.reconfigure = terminal_reconfigure;
	term->widget.rearrange = terminal_rearrange;
	term->widget.repaint = terminal_repaint;
	term->widget.handle_keyboard_event = terminal_handle_keyboard_event;
	term->widget.handle_position_event = terminal_handle_position_event;

	term->cols = width / FONT_WIDTH;
	term->rows = height / FONT_SCANLINES;

	term->frontbuf = NULL;
	term->backbuf = NULL;

	term->frontbuf = chargrid_create(term->cols, term->rows,
	    CHARGRID_FLAG_NONE);
	if (!term->frontbuf) {
		widget_deinit(&term->widget);
		return false;
	}

	term->backbuf = chargrid_create(term->cols, term->rows,
	    CHARGRID_FLAG_NONE);
	if (!term->backbuf) {
		widget_deinit(&term->widget);
		return false;
	}

	chargrid_clear(term->frontbuf);
	chargrid_clear(term->backbuf);
	term->top_row = 0;

	async_set_fallback_port_handler(term_connection, NULL);
	con_srvs_init(&term->srvs);
	term->srvs.ops = &con_ops;
	term->srvs.sarg = term;

	errno_t rc = loc_server_register(NAME);
	if (rc != EOK) {
		widget_deinit(&term->widget);
		return false;
	}

	char vc[LOC_NAME_MAXLEN + 1];
	snprintf(vc, LOC_NAME_MAXLEN, "%s/%" PRIu64, NAMESPACE,
	    task_get_id());

	rc = loc_service_register(vc, &term->dsid);
	if (rc != EOK) {
		widget_deinit(&term->widget);
		return false;
	}

	list_append(&term->link, &terms);
	getterm(vc, "/app/bdsh");

	return true;
}

terminal_t *create_terminal(widget_t *parent, const void *data, sysarg_t width,
    sysarg_t height)
{
	terminal_t *term = (terminal_t *) malloc(sizeof(terminal_t));
	if (!term)
		return NULL;

	bool ret = init_terminal(term, parent, data, width, height);
	if (!ret) {
		free(term);
		return NULL;
	}

	return term;
}

/** @}
 */
