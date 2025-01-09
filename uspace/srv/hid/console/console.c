/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup console
 * @{
 */
/** @file
 */

#include <async.h>
#include <stdio.h>
#include <adt/prodcons.h>
#include <io/console.h>
#include <io/input.h>
#include <ipc/vfs.h>
#include <errno.h>
#include <str_error.h>
#include <loc.h>
#include <io/con_srv.h>
#include <io/kbd_event.h>
#include <io/keycode.h>
#include <io/chargrid.h>
#include <io/output.h>
#include <align.h>
#include <as.h>
#include <task.h>
#include <fibril_synch.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <str.h>
#include "console.h"

#define NAME       "console"
#define NAMESPACE  "term"

#define UTF8_CHAR_BUFFER_SIZE  (STR_BOUNDS(1) + 1)

typedef struct {
	atomic_flag refcnt;      /**< Connection reference count */
	prodcons_t input_pc;  /**< Incoming console events */

	/**
	 * Not yet sent bytes of last char event.
	 */
	char char_remains[UTF8_CHAR_BUFFER_SIZE];
	size_t char_remains_len;  /**< Number of not yet sent bytes. */

	fibril_mutex_t mtx;  /**< Lock protecting mutable fields */

	size_t index;           /**< Console index */
	service_id_t dsid;      /**< Service handle */

	sysarg_t cols;         /**< Number of columns */
	sysarg_t rows;         /**< Number of rows */
	console_caps_t ccaps;  /**< Console capabilities */

	sysarg_t ucols;		/**< Number of columns in user buffer */
	sysarg_t urows;		/**< Number of rows in user buffer */
	charfield_t *ubuf;	/**< User buffer */

	chargrid_t *frontbuf;    /**< Front buffer */
	frontbuf_handle_t fbid;  /**< Front buffer handle */
	con_srvs_t srvs;         /**< Console service setup */
} console_t;

static loc_srv_t *console_srv;

/** Input server proxy */
static input_t *input;
static bool active = false;

/** Session to the output server */
static async_sess_t *output_sess;

/** Output dimensions */
static sysarg_t cols;
static sysarg_t rows;

/** Mouse pointer X coordinate */
static int pointer_x;
/** Mouse pointer Y coordinate */
static int pointer_y;
/** Character under mouse cursor */
static charfield_t pointer_bg;

static int mouse_scale_x = 4;
static int mouse_scale_y = 8;

/** Array of data for virtual consoles */
static console_t consoles[CONSOLE_COUNT];

/** Mutex for console switching */
static FIBRIL_MUTEX_INITIALIZE(switch_mtx);

static console_t *active_console = &consoles[0];

static errno_t input_ev_active(input_t *);
static errno_t input_ev_deactive(input_t *);
static errno_t input_ev_key(input_t *, unsigned, kbd_event_type_t, keycode_t,
    keymod_t, char32_t);
static errno_t input_ev_move(input_t *, unsigned, int, int);
static errno_t input_ev_abs_move(input_t *, unsigned, unsigned, unsigned,
    unsigned, unsigned);
static errno_t input_ev_button(input_t *, unsigned, int, int);
static errno_t input_ev_dclick(input_t *, unsigned, int);

static input_ev_ops_t input_ev_ops = {
	.active = input_ev_active,
	.deactive = input_ev_deactive,
	.key = input_ev_key,
	.move = input_ev_move,
	.abs_move = input_ev_abs_move,
	.button = input_ev_button,
	.dclick = input_ev_dclick
};

static errno_t cons_open(con_srvs_t *, con_srv_t *);
static errno_t cons_close(con_srv_t *);
static errno_t cons_read(con_srv_t *, void *, size_t, size_t *);
static errno_t cons_write(con_srv_t *, void *, size_t, size_t *);
static void cons_sync(con_srv_t *);
static void cons_clear(con_srv_t *);
static void cons_set_pos(con_srv_t *, sysarg_t col, sysarg_t row);
static errno_t cons_get_pos(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t cons_get_size(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t cons_get_color_cap(con_srv_t *, console_caps_t *);
static void cons_set_style(con_srv_t *, console_style_t);
static void cons_set_color(con_srv_t *, console_color_t, console_color_t,
    console_color_attr_t);
static void cons_set_rgb_color(con_srv_t *, pixel_t, pixel_t);
static void cons_set_cursor_visibility(con_srv_t *, bool);
static errno_t cons_set_caption(con_srv_t *, const char *);
static errno_t cons_get_event(con_srv_t *, cons_event_t *);
static errno_t cons_map(con_srv_t *, sysarg_t, sysarg_t, charfield_t **);
static void cons_unmap(con_srv_t *);
static void cons_buf_update(con_srv_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

static con_ops_t con_ops = {
	.open = cons_open,
	.close = cons_close,
	.read = cons_read,
	.write = cons_write,
	.sync = cons_sync,
	.clear = cons_clear,
	.set_pos = cons_set_pos,
	.get_pos = cons_get_pos,
	.get_size = cons_get_size,
	.get_color_cap = cons_get_color_cap,
	.set_style = cons_set_style,
	.set_color = cons_set_color,
	.set_rgb_color = cons_set_rgb_color,
	.set_cursor_visibility = cons_set_cursor_visibility,
	.set_caption = cons_set_caption,
	.get_event = cons_get_event,
	.map = cons_map,
	.unmap = cons_unmap,
	.update = cons_buf_update
};

static void pointer_draw(void);
static void pointer_undraw(void);

static console_t *srv_to_console(con_srv_t *srv)
{
	return srv->srvs->sarg;
}

static void cons_update(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);

	if ((active) && (cons == active_console)) {
		output_update(output_sess, cons->fbid);
		output_cursor_update(output_sess, cons->fbid);
	}

	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_update_cursor(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);

	if ((active) && (cons == active_console))
		output_cursor_update(output_sess, cons->fbid);

	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_damage(console_t *cons)
{
	fibril_mutex_lock(&switch_mtx);
	fibril_mutex_lock(&cons->mtx);

	if ((active) && (cons == active_console)) {
		output_damage(output_sess, cons->fbid, 0, 0, cons->cols,
		    cons->rows);
		output_cursor_update(output_sess, cons->fbid);
	}

	fibril_mutex_unlock(&cons->mtx);
	fibril_mutex_unlock(&switch_mtx);
}

static void cons_switch(unsigned int index)
{
	/*
	 * The first undefined index is reserved
	 * for switching to the kernel console.
	 */
	if (index == CONSOLE_COUNT) {
		if (console_kcon())
			active = false;

		return;
	}

	if (index > CONSOLE_COUNT)
		return;

	console_t *cons = &consoles[index];

	fibril_mutex_lock(&switch_mtx);
	pointer_undraw();

	if (cons == active_console) {
		fibril_mutex_unlock(&switch_mtx);
		return;
	}

	active_console = cons;

	pointer_draw();
	fibril_mutex_unlock(&switch_mtx);

	cons_damage(cons);
}

/** Draw mouse pointer. */
static void pointer_draw(void)
{
	charfield_t *ch;
	int col, row;

	/* Downscale coordinates to text resolution */
	col = pointer_x / mouse_scale_x;
	row = pointer_y / mouse_scale_y;

	/* Make sure they are in range */
	if (col < 0 || row < 0 || col >= (int)cols || row >= (int)rows)
		return;

	ch = chargrid_charfield_at(active_console->frontbuf, col, row);

	/*
	 * Store background attributes for undrawing the pointer.
	 * This is necessary as styles cannot be inverted with
	 * round trip (unlike RGB or INDEX)
	 */
	pointer_bg = *ch;

	/* In general the color should be a one's complement of the background */
	if (ch->attrs.type == CHAR_ATTR_INDEX) {
		ch->attrs.val.index.bgcolor ^= 0xf;
		ch->attrs.val.index.fgcolor ^= 0xf;
	} else if (ch->attrs.type == CHAR_ATTR_RGB) {
		ch->attrs.val.rgb.fgcolor ^= 0xffffff;
		ch->attrs.val.rgb.bgcolor ^= 0xffffff;
	} else if (ch->attrs.type == CHAR_ATTR_STYLE) {
		/* Don't have a proper inverse for each style */
		if (ch->attrs.val.style == STYLE_INVERTED)
			ch->attrs.val.style = STYLE_NORMAL;
		else
			ch->attrs.val.style = STYLE_INVERTED;
	}

	/* Make sure the cell gets updated */
	ch->flags |= CHAR_FLAG_DIRTY;
}

/** Undraw mouse pointer. */
static void pointer_undraw(void)
{
	charfield_t *ch;
	int col, row;

	col = pointer_x / mouse_scale_x;
	row = pointer_y / mouse_scale_y;
	if (col < 0 || row < 0 || col >= (int)cols || row >= (int)rows)
		return;

	ch = chargrid_charfield_at(active_console->frontbuf, col, row);
	*ch = pointer_bg;
	ch->flags |= CHAR_FLAG_DIRTY;
}

/** Queue console event.
 *
 * @param cons Console
 * @param ev Console event
 */
static void console_queue_cons_event(console_t *cons, cons_event_t *ev)
{
	/* Got key press/release event */
	cons_qevent_t *event =
	    (cons_qevent_t *) malloc(sizeof(cons_qevent_t));
	if (event == NULL)
		return;

	event->ev = *ev;
	link_initialize(&event->link);

	prodcons_produce(&cons->input_pc, &event->link);
}

static errno_t input_ev_active(input_t *input)
{
	active = true;
	output_claim(output_sess);
	cons_damage(active_console);

	return EOK;
}

static errno_t input_ev_deactive(input_t *input)
{
	active = false;
	output_yield(output_sess);

	return EOK;
}

static errno_t input_ev_key(input_t *input, unsigned kbd_id,
    kbd_event_type_t type, keycode_t key, keymod_t mods, char32_t c)
{
	cons_event_t event;
	bool alt;
	bool shift;

	alt = (mods & KM_ALT) != 0 && (mods & (KM_CTRL | KM_SHIFT)) == 0;
	shift = (mods & KM_SHIFT) != 0 && (mods & (KM_CTRL | KM_ALT)) == 0;

	/* Switch console on Alt+Fn or Shift+Fn */
	if ((key >= KC_F1) && (key <= KC_F1 + CONSOLE_COUNT) &&
	    (alt || shift)) {
		cons_switch(key - KC_F1);
	} else {
		/* Got key press/release event */
		event.type = CEV_KEY;

		(void)kbd_id;
		event.ev.key.type = type;
		event.ev.key.key = key;
		event.ev.key.mods = mods;
		event.ev.key.c = c;

		console_queue_cons_event(active_console, &event);
	}

	return EOK;
}

/** Update pointer position.
 *
 * @param new_x New X coordinate (in pixels)
 * @param new_y New Y coordinate (in pixels)
 */
static void pointer_update(int new_x, int new_y)
{
	bool upd_pointer;

	/* Make sure coordinates are in range */

	if (new_x < 0)
		new_x = 0;
	if (new_x >= (int)cols * mouse_scale_x)
		new_x = cols * mouse_scale_x - 1;
	if (new_y < 0)
		new_y = 0;
	if (new_y >= (int)rows * mouse_scale_y)
		new_y = rows * mouse_scale_y - 1;

	/* Determine if pointer moved to a different character cell */
	upd_pointer = (new_x / mouse_scale_x != pointer_x / mouse_scale_x) ||
	    (new_y / mouse_scale_y != pointer_y / mouse_scale_y);

	if (upd_pointer)
		pointer_undraw();

	/* Store new pointer position */
	pointer_x = new_x;
	pointer_y = new_y;

	if (upd_pointer) {
		pointer_draw();
		cons_update(active_console);
	}
}

static errno_t input_ev_move(input_t *input, unsigned pos_id, int dx, int dy)
{
	(void) pos_id;
	pointer_update(pointer_x + dx, pointer_y + dy);
	return EOK;
}

static errno_t input_ev_abs_move(input_t *input, unsigned pos_id, unsigned x,
    unsigned y, unsigned max_x, unsigned max_y)
{
	(void)pos_id;
	pointer_update(mouse_scale_x * cols * x / max_x, mouse_scale_y * rows * y / max_y);
	return EOK;
}

static errno_t input_ev_button(input_t *input, unsigned pos_id, int bnum,
    int bpress)
{
	cons_event_t event;

	(void)pos_id;

	event.type = CEV_POS;
	event.ev.pos.type = bpress ? POS_PRESS : POS_RELEASE;
	event.ev.pos.btn_num = bnum;
	event.ev.pos.hpos = pointer_x / mouse_scale_x;
	event.ev.pos.vpos = pointer_y / mouse_scale_y;

	console_queue_cons_event(active_console, &event);
	return EOK;
}

static errno_t input_ev_dclick(input_t *input, unsigned pos_id, int bnum)
{
	cons_event_t event;

	(void)pos_id;

	event.type = CEV_POS;
	event.ev.pos.type = POS_DCLICK;
	event.ev.pos.btn_num = bnum;
	event.ev.pos.hpos = pointer_x / mouse_scale_x;
	event.ev.pos.vpos = pointer_y / mouse_scale_y;

	console_queue_cons_event(active_console, &event);
	return EOK;
}

/** Process a character from the client (TTY emulation). */
static void cons_write_char(console_t *cons, char32_t ch)
{
	sysarg_t updated = 0;

	fibril_mutex_lock(&cons->mtx);
	pointer_undraw();

	switch (ch) {
	case '\n':
		updated = chargrid_newline(cons->frontbuf);
		break;
	case '\r':
		break;
	case '\t':
		updated = chargrid_tabstop(cons->frontbuf, 8);
		break;
	case '\b':
		updated = chargrid_backspace(cons->frontbuf);
		break;
	default:
		updated = chargrid_putuchar(cons->frontbuf, ch, true);
	}

	pointer_draw();
	fibril_mutex_unlock(&cons->mtx);

	if (updated > 1)
		cons_update(cons);
}

static void cons_set_cursor_vis(console_t *cons, bool visible)
{
	fibril_mutex_lock(&cons->mtx);
	pointer_undraw();
	chargrid_set_cursor_visibility(cons->frontbuf, visible);
	pointer_draw();
	fibril_mutex_unlock(&cons->mtx);

	cons_update_cursor(cons);
}

static errno_t cons_open(con_srvs_t *srvs, con_srv_t *srv)
{
	return EOK;
}

static errno_t cons_close(con_srv_t *srv)
{
	return EOK;
}

static errno_t cons_read(con_srv_t *srv, void *buf, size_t size, size_t *nread)
{
	uint8_t *bbuf = buf;
	console_t *cons = srv_to_console(srv);
	size_t pos = 0;

	/*
	 * Read input from keyboard and copy it to the buffer.
	 * We need to handle situation when wchar is split by 2 following
	 * reads.
	 */
	while (pos < size) {
		/* Copy to the buffer remaining characters. */
		while ((pos < size) && (cons->char_remains_len > 0)) {
			bbuf[pos] = cons->char_remains[0];
			pos++;

			/* Unshift the array. */
			for (size_t i = 1; i < cons->char_remains_len; i++)
				cons->char_remains[i - 1] = cons->char_remains[i];

			cons->char_remains_len--;
		}

		/* Still not enough? Then get another key from the queue. */
		if (pos < size) {
			link_t *link = prodcons_consume(&cons->input_pc);
			cons_qevent_t *qevent = list_get_instance(link,
			    cons_qevent_t, link);
			cons_event_t *event = &qevent->ev;

			/* Accept key presses of printable chars only. */
			if (event->type == CEV_KEY && event->ev.key.type == KEY_PRESS &&
			    (event->ev.key.c != 0)) {
				char32_t tmp[2] = { event->ev.key.c, 0 };
				wstr_to_str(cons->char_remains, UTF8_CHAR_BUFFER_SIZE, tmp);
				cons->char_remains_len = str_size(cons->char_remains);
			}

			free(qevent);
		}
	}

	*nread = size;
	return EOK;
}

static errno_t cons_write(con_srv_t *srv, void *data, size_t size, size_t *nwritten)
{
	console_t *cons = srv_to_console(srv);

	size_t off = 0;
	while (off < size)
		cons_write_char(cons, str_decode(data, &off, size));

	*nwritten = size;
	return EOK;
}

static void cons_sync(con_srv_t *srv)
{
	console_t *cons = srv_to_console(srv);

	cons_update(cons);
}

static void cons_clear(con_srv_t *srv)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	pointer_undraw();
	chargrid_clear(cons->frontbuf);
	pointer_draw();
	fibril_mutex_unlock(&cons->mtx);

	cons_update(cons);
}

static void cons_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	pointer_undraw();
	chargrid_set_cursor(cons->frontbuf, col, row);
	pointer_draw();
	fibril_mutex_unlock(&cons->mtx);

	cons_update_cursor(cons);
}

static errno_t cons_get_pos(con_srv_t *srv, sysarg_t *col, sysarg_t *row)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	chargrid_get_cursor(cons->frontbuf, col, row);
	fibril_mutex_unlock(&cons->mtx);

	return EOK;
}

static errno_t cons_get_size(con_srv_t *srv, sysarg_t *cols, sysarg_t *rows)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	*cols = cons->cols;
	*rows = cons->rows;
	fibril_mutex_unlock(&cons->mtx);

	return EOK;
}

static errno_t cons_get_color_cap(con_srv_t *srv, console_caps_t *ccaps)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	*ccaps = cons->ccaps;
	fibril_mutex_unlock(&cons->mtx);

	return EOK;
}

static void cons_set_style(con_srv_t *srv, console_style_t style)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	chargrid_set_style(cons->frontbuf, style);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_color(con_srv_t *srv, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	chargrid_set_color(cons->frontbuf, bgcolor, fgcolor, attr);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_rgb_color(con_srv_t *srv, pixel_t bgcolor,
    pixel_t fgcolor)
{
	console_t *cons = srv_to_console(srv);

	fibril_mutex_lock(&cons->mtx);
	chargrid_set_rgb_color(cons->frontbuf, bgcolor, fgcolor);
	fibril_mutex_unlock(&cons->mtx);
}

static void cons_set_cursor_visibility(con_srv_t *srv, bool visible)
{
	console_t *cons = srv_to_console(srv);

	cons_set_cursor_vis(cons, visible);
}

static errno_t cons_set_caption(con_srv_t *srv, const char *caption)
{
	console_t *cons = srv_to_console(srv);

	(void) cons;
	(void) caption;
	return EOK;
}

static errno_t cons_get_event(con_srv_t *srv, cons_event_t *event)
{
	console_t *cons = srv_to_console(srv);
	link_t *link = prodcons_consume(&cons->input_pc);
	cons_qevent_t *qevent = list_get_instance(link, cons_qevent_t, link);

	*event = qevent->ev;
	free(qevent);
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
static errno_t cons_map(con_srv_t *srv, sysarg_t cols, sysarg_t rows,
    charfield_t **rbuf)
{
	console_t *cons = srv_to_console(srv);
	void *buf;

	fibril_mutex_lock(&cons->mtx);

	if (cons->ubuf != NULL) {
		fibril_mutex_unlock(&cons->mtx);
		return EBUSY;
	}

	buf = as_area_create(AS_AREA_ANY, cols * rows * sizeof(charfield_t),
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (buf == AS_MAP_FAILED) {
		fibril_mutex_unlock(&cons->mtx);
		return ENOMEM;
	}

	cons->ucols = cols;
	cons->urows = rows;
	cons->ubuf = buf;
	fibril_mutex_unlock(&cons->mtx);

	*rbuf = buf;
	return EOK;
}

/** Delete shared buffer.
 *
 * @param srv Console server
 */
static void cons_unmap(con_srv_t *srv)
{
	console_t *cons = srv_to_console(srv);
	void *buf;

	fibril_mutex_lock(&cons->mtx);

	buf = cons->ubuf;
	cons->ubuf = NULL;

	if (buf != NULL)
		as_area_destroy(buf);

	fibril_mutex_unlock(&cons->mtx);
}

/** Update area of console from shared buffer.
 *
 * @param srv Console server
 * @param c0 Column coordinate of top-left corner (inclusive)
 * @param r0 Row coordinate of top-left corner (inclusive)
 * @param c1 Column coordinate of bottom-right corner (exclusive)
 * @param r1 Row coordinate of bottom-right corner (exclusive)
 */
static void cons_buf_update(con_srv_t *srv, sysarg_t c0, sysarg_t r0,
    sysarg_t c1, sysarg_t r1)
{
	console_t *cons = srv_to_console(srv);
	charfield_t *ch;
	sysarg_t col, row;

	fibril_mutex_lock(&cons->mtx);

	if (cons->ubuf == NULL) {
		fibril_mutex_unlock(&cons->mtx);
		return;
	}

	/* Make sure we have meaningful coordinates, within bounds */

	if (c1 > cons->ucols)
		c1 = cons->ucols;
	if (c1 > cons->cols)
		c1 = cons->cols;
	if (c0 >= c1) {
		fibril_mutex_unlock(&cons->mtx);
		return;
	}
	if (r1 > cons->urows)
		r1 = cons->urows;
	if (r1 > cons->rows)
		r1 = cons->rows;
	if (r0 >= r1) {
		fibril_mutex_unlock(&cons->mtx);
		return;
	}

	/* Update front buffer from user buffer */

	pointer_undraw();

	for (row = r0; row < r1; row++) {
		for (col = c0; col < c1; col++) {
			ch = chargrid_charfield_at(cons->frontbuf, col, row);
			*ch = cons->ubuf[row * cons->ucols + col];
		}
	}

	pointer_draw();
	fibril_mutex_unlock(&cons->mtx);

	/* Update console */
	cons_update(cons);
}

static void client_connection(ipc_call_t *icall, void *arg)
{
	console_t *cons = NULL;

	for (size_t i = 0; i < CONSOLE_COUNT; i++) {
		if (consoles[i].dsid == (service_id_t) ipc_get_arg2(icall)) {
			cons = &consoles[i];
			break;
		}
	}

	if (cons == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	if (!atomic_flag_test_and_set(&cons->refcnt))
		cons_set_cursor_vis(cons, true);

	con_conn(icall, &cons->srvs);
}

static errno_t input_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;

	errno_t rc = loc_service_get_id(svc, &dsid, 0);
	if (rc != EOK) {
		printf("%s: Input service %s not found\n", NAME, svc);
		return rc;
	}

	sess = loc_service_connect(dsid, INTERFACE_INPUT, 0);
	if (sess == NULL) {
		printf("%s: Unable to connect to input service %s\n", NAME,
		    svc);
		return EIO;
	}

	rc = input_open(sess, &input_ev_ops, NULL, &input);
	if (rc != EOK) {
		async_hangup(sess);
		printf("%s: Unable to communicate with service %s (%s)\n",
		    NAME, svc, str_error(rc));
		return rc;
	}

	return EOK;
}

static async_sess_t *output_connect(const char *svc)
{
	async_sess_t *sess;
	service_id_t dsid;

	errno_t rc = loc_service_get_id(svc, &dsid, 0);
	if (rc == EOK) {
		sess = loc_service_connect(dsid, INTERFACE_OUTPUT, 0);
		if (sess == NULL) {
			printf("%s: Unable to connect to output service %s\n",
			    NAME, svc);
			return NULL;
		}
	} else
		return NULL;

	return sess;
}

static bool console_srv_init(char *input_svc, char *output_svc)
{
	/* Connect to input service */
	errno_t rc = input_connect(input_svc);
	if (rc != EOK)
		return false;

	/* Connect to output service */
	output_sess = output_connect(output_svc);
	if (output_sess == NULL)
		return false;

	/* Register server */
	async_set_fallback_port_handler(client_connection, NULL);
	rc = loc_server_register(NAME, &console_srv);
	if (rc != EOK) {
		printf("%s: Unable to register server (%s)\n", NAME,
		    str_error(rc));
		return false;
	}

	output_get_dimensions(output_sess, &cols, &rows);
	output_set_style(output_sess, STYLE_NORMAL);

	console_caps_t ccaps;
	output_get_caps(output_sess, &ccaps);

	/*
	 * Inititalize consoles only if there are
	 * actually some output devices.
	 */
	if (ccaps != 0) {
		for (size_t i = 0; i < CONSOLE_COUNT; i++) {
			consoles[i].index = i;
			atomic_flag_clear(&consoles[i].refcnt);
			fibril_mutex_initialize(&consoles[i].mtx);
			prodcons_initialize(&consoles[i].input_pc);
			consoles[i].char_remains_len = 0;

			consoles[i].cols = cols;
			consoles[i].rows = rows;
			consoles[i].ccaps = ccaps;
			consoles[i].frontbuf =
			    chargrid_create(cols, rows, CHARGRID_FLAG_SHARED);

			if (consoles[i].frontbuf == NULL) {
				printf("%s: Unable to allocate frontbuffer %zu\n", NAME, i);
				return false;
			}

			consoles[i].fbid = output_frontbuf_create(output_sess,
			    consoles[i].frontbuf);
			if (consoles[i].fbid == 0) {
				printf("%s: Unable to create frontbuffer %zu\n", NAME, i);
				return false;
			}

			con_srvs_init(&consoles[i].srvs);
			consoles[i].srvs.ops = &con_ops;
			consoles[i].srvs.sarg = &consoles[i];

			char vc[LOC_NAME_MAXLEN + 1];
			snprintf(vc, LOC_NAME_MAXLEN, "%s/vc%zu", NAMESPACE, i);

			if (loc_service_register(console_srv, vc,
			    &consoles[i].dsid) != EOK) {
				printf("%s: Unable to register device %s\n", NAME, vc);
				return false;
			}
		}

		input_activate(input);
		active = true;
		cons_damage(active_console);
	}

	return true;
}

static void usage(char *name)
{
	printf("Usage: %s <input_dev> <output_dev>\n", name);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}

	printf("%s: HelenOS Console service\n", NAME);

	if (!console_srv_init(argv[1], argv[2]))
		return -1;

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Never reached */
	return 0;
}

/** @}
 */
