/*
 * Copyright (c) 2010 Jiri Svoboda
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

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <io/console.h>
#include <io/keycode.h>
#include <io/style.h>
#include <io/color.h>
#include <vfs/vfs.h>
#include <clipboard.h>
#include <macros.h>
#include <errno.h>
#include <assert.h>
#include <bool.h>

#include "tinput.h"

/** Seek direction */
typedef enum {
	seek_backward = -1,
	seek_forward = 1
} seek_dir_t;

static void tinput_init(tinput_t *ti);
static void tinput_insert_string(tinput_t *ti, const char *str);
static void tinput_sel_get_bounds(tinput_t *ti, int *sa, int *sb);
static bool tinput_sel_active(tinput_t *ti);
static void tinput_sel_all(tinput_t *ti);
static void tinput_sel_delete(tinput_t *ti);
static void tinput_key_ctrl(tinput_t *ti, console_event_t *ev);
static void tinput_key_shift(tinput_t *ti, console_event_t *ev);
static void tinput_key_ctrl_shift(tinput_t *ti, console_event_t *ev);
static void tinput_key_unmod(tinput_t *ti, console_event_t *ev);
static void tinput_pre_seek(tinput_t *ti, bool shift_held);
static void tinput_post_seek(tinput_t *ti, bool shift_held);

/** Create a new text input field. */
tinput_t *tinput_new(void)
{
	tinput_t *ti;

	ti = malloc(sizeof(tinput_t));
	if (ti == NULL)
		return NULL;

	tinput_init(ti);
	return ti;
}

/** Destroy text input field. */
void tinput_destroy(tinput_t *ti)
{
	free(ti);
}

static void tinput_display_tail(tinput_t *ti, int start, int pad)
{
	static wchar_t dbuf[INPUT_MAX_SIZE + 1];
	int sa, sb;
	int i, p;

	tinput_sel_get_bounds(ti, &sa, &sb);

	console_goto(fphone(stdout), (ti->col0 + start) % ti->con_cols,
	    ti->row0 + (ti->col0 + start) / ti->con_cols);
	console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);

	p = start;
	if (p < sa) {
		memcpy(dbuf, ti->buffer + p, (sa - p) * sizeof(wchar_t));
		dbuf[sa - p] = '\0';
		printf("%ls", dbuf);
		p = sa;
	}

	if (p < sb) {
		fflush(stdout);
		console_set_color(fphone(stdout), COLOR_BLACK, COLOR_RED, 0);
		memcpy(dbuf, ti->buffer + p,
		    (sb - p) * sizeof(wchar_t));
		dbuf[sb - p] = '\0';
		printf("%ls", dbuf);
		p = sb;
	}

	fflush(stdout);
	console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);

	if (p < ti->nc) {
		memcpy(dbuf, ti->buffer + p,
		    (ti->nc - p) * sizeof(wchar_t));
		dbuf[ti->nc - p] = '\0';
		printf("%ls", dbuf);
	}

	for (i = 0; i < pad; ++i)
		putchar(' ');
	fflush(stdout);
}

static char *tinput_get_str(tinput_t *ti)
{
	return wstr_to_astr(ti->buffer);
}

static void tinput_position_caret(tinput_t *ti)
{
	console_goto(fphone(stdout), (ti->col0 + ti->pos) % ti->con_cols,
	    ti->row0 + (ti->col0 + ti->pos) / ti->con_cols);
}

/** Update row0 in case the screen could have scrolled. */
static void tinput_update_origin(tinput_t *ti)
{
	int width, rows;

	width = ti->col0 + ti->nc;
	rows = (width / ti->con_cols) + 1;

	/* Update row0 if the screen scrolled. */
	if (ti->row0 + rows > ti->con_rows)
		ti->row0 = ti->con_rows - rows;	
}

static void tinput_insert_char(tinput_t *ti, wchar_t c)
{
	int i;
	int new_width, new_height;

	if (ti->nc == INPUT_MAX_SIZE)
		return;

	new_width = ti->col0 + ti->nc + 1;
	if (new_width % ti->con_cols == 0) {
		/* Advancing to new line. */
		new_height = (new_width / ti->con_cols) + 1;
		if (new_height >= ti->con_rows)
			return; /* Disallow text longer than 1 page for now. */
	}

	for (i = ti->nc; i > ti->pos; --i)
		ti->buffer[i] = ti->buffer[i - 1];

	ti->buffer[ti->pos] = c;
	ti->pos += 1;
	ti->nc += 1;
	ti->buffer[ti->nc] = '\0';
	ti->sel_start = ti->pos;

	tinput_display_tail(ti, ti->pos - 1, 0);
	tinput_update_origin(ti);
	tinput_position_caret(ti);
}

static void tinput_insert_string(tinput_t *ti, const char *str)
{
	int i;
	int new_width, new_height;
	int ilen;
	wchar_t c;
	size_t off;

	ilen = min((ssize_t) str_length(str), INPUT_MAX_SIZE - ti->nc);
	if (ilen == 0)
		return;

	new_width = ti->col0 + ti->nc + ilen;
	new_height = (new_width / ti->con_cols) + 1;
	if (new_height >= ti->con_rows)
		return; /* Disallow text longer than 1 page for now. */

	for (i = ti->nc - 1; i >= ti->pos; --i)
		ti->buffer[i + ilen] = ti->buffer[i];

	off = 0; i = 0;
	while (i < ilen) {
		c = str_decode(str, &off, STR_NO_LIMIT);
		if (c == '\0')
			break;

		/* Filter out non-printable chars. */
		if (c < 32)
			c = 32;

		ti->buffer[ti->pos + i] = c;
		++i;
	}

	ti->pos += ilen;
	ti->nc += ilen;
	ti->buffer[ti->nc] = '\0';
	ti->sel_start = ti->pos;

	tinput_display_tail(ti, ti->pos - ilen, 0);
	tinput_update_origin(ti);
	tinput_position_caret(ti);
}

static void tinput_backspace(tinput_t *ti)
{
	int i;

	if (tinput_sel_active(ti)) {
		tinput_sel_delete(ti);
		return;
	}

	if (ti->pos == 0)
		return;

	for (i = ti->pos; i < ti->nc; ++i)
		ti->buffer[i - 1] = ti->buffer[i];
	ti->pos -= 1;
	ti->nc -= 1;
	ti->buffer[ti->nc] = '\0';
	ti->sel_start = ti->pos;

	tinput_display_tail(ti, ti->pos, 1);
	tinput_position_caret(ti);
}

static void tinput_delete(tinput_t *ti)
{
	if (tinput_sel_active(ti)) {
		tinput_sel_delete(ti);
		return;
	}

	if (ti->pos == ti->nc)
		return;

	ti->pos += 1;
	ti->sel_start = ti->pos;

	tinput_backspace(ti);
}

static void tinput_seek_cell(tinput_t *ti, seek_dir_t dir, bool shift_held)
{
	tinput_pre_seek(ti, shift_held);

	if (dir == seek_forward) {
		if (ti->pos < ti->nc)
			ti->pos += 1;
	} else {
		if (ti->pos > 0)
			ti->pos -= 1;
	}

	tinput_post_seek(ti, shift_held);
}

static void tinput_seek_word(tinput_t *ti, seek_dir_t dir, bool shift_held)
{
	tinput_pre_seek(ti, shift_held);

	if (dir == seek_forward) {
		if (ti->pos == ti->nc)
			return;

		while (1) {
			ti->pos += 1;

			if (ti->pos == ti->nc)
				break;

			if (ti->buffer[ti->pos - 1] == ' ' &&
			    ti->buffer[ti->pos] != ' ')
				break;
		}
	} else {
		if (ti->pos == 0)
			return;

		while (1) {
			ti->pos -= 1;

			if (ti->pos == 0)
				break;

			if (ti->buffer[ti->pos - 1] == ' ' &&
			    ti->buffer[ti->pos] != ' ')
				break;
		}

	}

	tinput_post_seek(ti, shift_held);
}

static void tinput_seek_vertical(tinput_t *ti, seek_dir_t dir, bool shift_held)
{
	tinput_pre_seek(ti, shift_held);

	if (dir == seek_forward) {
		if (ti->pos + ti->con_cols <= ti->nc)
			ti->pos = ti->pos + ti->con_cols;
	} else {
		if (ti->pos - ti->con_cols >= 0)
			ti->pos = ti->pos - ti->con_cols;
	}

	tinput_post_seek(ti, shift_held);
}

static void tinput_seek_max(tinput_t *ti, seek_dir_t dir, bool shift_held)
{
	tinput_pre_seek(ti, shift_held);

	if (dir == seek_backward)
		ti->pos = 0;
	else
		ti->pos = ti->nc;

	tinput_post_seek(ti, shift_held);
}

static void tinput_pre_seek(tinput_t *ti, bool shift_held)
{
	if (tinput_sel_active(ti) && !shift_held) {
		/* Unselect and redraw. */
		ti->sel_start = ti->pos;
		tinput_display_tail(ti, 0, 0);
		tinput_position_caret(ti);
	}
}

static void tinput_post_seek(tinput_t *ti, bool shift_held)
{
	if (shift_held) {
		/* Selecting text. Need redraw. */
		tinput_display_tail(ti, 0, 0);
	} else {
		/* Shift not held. Keep selection empty. */
		ti->sel_start = ti->pos;
	}
	tinput_position_caret(ti);
}

static void tinput_history_insert(tinput_t *ti, char *str)
{
	int i;

	if (ti->hnum < HISTORY_LEN) {
		ti->hnum += 1;
	} else {
		if (ti->history[HISTORY_LEN] != NULL)
			free(ti->history[HISTORY_LEN]);
	}

	for (i = ti->hnum; i > 1; --i)
		ti->history[i] = ti->history[i - 1];

	ti->history[1] = str_dup(str);

	if (ti->history[0] != NULL) {
		free(ti->history[0]);
		ti->history[0] = NULL;
	}
}

static void tinput_set_str(tinput_t *ti, char *str)
{
	str_to_wstr(ti->buffer, INPUT_MAX_SIZE, str);
	ti->nc = wstr_length(ti->buffer);
	ti->pos = ti->nc;
	ti->sel_start = ti->pos;
}

static void tinput_sel_get_bounds(tinput_t *ti, int *sa, int *sb)
{
	if (ti->sel_start < ti->pos) {
		*sa = ti->sel_start;
		*sb = ti->pos;
	} else {
		*sa = ti->pos;
		*sb = ti->sel_start;
	}
}

static bool tinput_sel_active(tinput_t *ti)
{
	return ti->sel_start != ti->pos;
}

static void tinput_sel_all(tinput_t *ti)
{
	ti->sel_start = 0;
	ti->pos = ti->nc;
	tinput_display_tail(ti, 0, 0);
	tinput_position_caret(ti);
}

static void tinput_sel_delete(tinput_t *ti)
{
	int sa, sb;

	tinput_sel_get_bounds(ti, &sa, &sb);
	if (sa == sb)
		return;

	memmove(ti->buffer + sa, ti->buffer + sb,
	    (ti->nc - sb) * sizeof(wchar_t));
	ti->pos = ti->sel_start = sa;
	ti->nc -= (sb - sa);
	ti->buffer[ti->nc] = '\0';

	tinput_display_tail(ti, sa, sb - sa);
	tinput_position_caret(ti);
}

static void tinput_sel_copy_to_cb(tinput_t *ti)
{
	int sa, sb;
	char *str;

	tinput_sel_get_bounds(ti, &sa, &sb);

	if (sb < ti->nc) {
		wchar_t tmp_c = ti->buffer[sb];
		ti->buffer[sb] = '\0';
		str = wstr_to_astr(ti->buffer + sa);
		ti->buffer[sb] = tmp_c;
	} else
		str = wstr_to_astr(ti->buffer + sa);
	
	if (str == NULL)
		goto error;

	if (clipboard_put_str(str) != EOK)
		goto error;

	free(str);
	return;
error:
	return;
	/* TODO: Give the user some warning. */
}

static void tinput_paste_from_cb(tinput_t *ti)
{
	char *str;
	int rc;

	rc = clipboard_get_str(&str);
	if (rc != EOK || str == NULL)
		return; /* TODO: Give the user some warning. */

	tinput_insert_string(ti, str);
	free(str);
}

static void tinput_history_seek(tinput_t *ti, int offs)
{
	int pad;

	if (ti->hpos + offs < 0 || ti->hpos + offs > ti->hnum)
		return;

	if (ti->history[ti->hpos] != NULL) {
		free(ti->history[ti->hpos]);
		ti->history[ti->hpos] = NULL;
	}

	ti->history[ti->hpos] = tinput_get_str(ti);
	ti->hpos += offs;

	pad = ti->nc - str_length(ti->history[ti->hpos]);
	if (pad < 0) pad = 0;

	tinput_set_str(ti, ti->history[ti->hpos]);
	tinput_display_tail(ti, 0, pad);
	tinput_update_origin(ti);
	tinput_position_caret(ti);
}

/** Initialize text input field.
 *
 * Must be called before using the field. It clears the history.
 */
static void tinput_init(tinput_t *ti)
{
	ti->hnum = 0;
	ti->hpos = 0;
	ti->history[0] = NULL;
}

/** Read in one line of input. */
char *tinput_read(tinput_t *ti)
{
	console_event_t ev;
	char *str;

	fflush(stdout);

	if (console_get_size(fphone(stdin), &ti->con_cols, &ti->con_rows) != EOK)
		return NULL;
	if (console_get_pos(fphone(stdin), &ti->col0, &ti->row0) != EOK)
		return NULL;

	ti->pos = ti->sel_start = 0;
	ti->nc = 0;
	ti->buffer[0] = '\0';
	ti->done = false;

	while (!ti->done) {
		fflush(stdout);
		if (!console_get_event(fphone(stdin), &ev))
			return NULL;

		if (ev.type != KEY_PRESS)
			continue;

		if ((ev.mods & KM_CTRL) != 0 &&
		    (ev.mods & (KM_ALT | KM_SHIFT)) == 0) {
			tinput_key_ctrl(ti, &ev);
		}

		if ((ev.mods & KM_SHIFT) != 0 &&
		    (ev.mods & (KM_CTRL | KM_ALT)) == 0) {
			tinput_key_shift(ti, &ev);
		}

		if ((ev.mods & KM_CTRL) != 0 &&
		    (ev.mods & KM_SHIFT) != 0 &&
		    (ev.mods & KM_ALT) == 0) {
			tinput_key_ctrl_shift(ti, &ev);
		}

		if ((ev.mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
			tinput_key_unmod(ti, &ev);
		}

		if (ev.c >= ' ') {
			tinput_sel_delete(ti);
			tinput_insert_char(ti, ev.c);
		}
	}

	ti->pos = ti->nc;
	tinput_position_caret(ti);
	putchar('\n');

	str = tinput_get_str(ti);
	if (str_cmp(str, "") != 0)
		tinput_history_insert(ti, str);

	ti->hpos = 0;

	return str;
}

static void tinput_key_ctrl(tinput_t *ti, console_event_t *ev)
{
	switch (ev->key) {
	case KC_LEFT:
		tinput_seek_word(ti, seek_backward, false);
		break;
	case KC_RIGHT:
		tinput_seek_word(ti, seek_forward, false);
		break;
	case KC_UP:
		tinput_seek_vertical(ti, seek_backward, false);
		break;
	case KC_DOWN:
		tinput_seek_vertical(ti, seek_forward, false);
		break;
	case KC_X:
		tinput_sel_copy_to_cb(ti);
		tinput_sel_delete(ti);
		break;
	case KC_C:
		tinput_sel_copy_to_cb(ti);
		break;
	case KC_V:
		tinput_sel_delete(ti);
		tinput_paste_from_cb(ti);
		break;
	case KC_A:
		tinput_sel_all(ti);
		break;
	default:
		break;
	}
}

static void tinput_key_ctrl_shift(tinput_t *ti, console_event_t *ev)
{
	switch (ev->key) {
	case KC_LEFT:
		tinput_seek_word(ti, seek_backward, true);
		break;
	case KC_RIGHT:
		tinput_seek_word(ti, seek_forward, true);
		break;
	case KC_UP:
		tinput_seek_vertical(ti, seek_backward, true);
		break;
	case KC_DOWN:
		tinput_seek_vertical(ti, seek_forward, true);
		break;
	default:
		break;
	}
}

static void tinput_key_shift(tinput_t *ti, console_event_t *ev)
{
	switch (ev->key) {
	case KC_LEFT:
		tinput_seek_cell(ti, seek_backward, true);
		break;
	case KC_RIGHT:
		tinput_seek_cell(ti, seek_forward, true);
		break;
	case KC_UP:
		tinput_seek_vertical(ti, seek_backward, true);
		break;
	case KC_DOWN:
		tinput_seek_vertical(ti, seek_forward, true);
		break;
	case KC_HOME:
		tinput_seek_max(ti, seek_backward, true);
		break;
	case KC_END:
		tinput_seek_max(ti, seek_forward, true);
		break;
	default:
		break;
	}
}

static void tinput_key_unmod(tinput_t *ti, console_event_t *ev)
{
	switch (ev->key) {
	case KC_ENTER:
	case KC_NENTER:
		ti->done = true;
		break;
	case KC_BACKSPACE:
		tinput_backspace(ti);
		break;
	case KC_DELETE:
		tinput_delete(ti);
		break;
	case KC_LEFT:
		tinput_seek_cell(ti, seek_backward, false);
		break;
	case KC_RIGHT:
		tinput_seek_cell(ti, seek_forward, false);
		break;
	case KC_HOME:
		tinput_seek_max(ti, seek_backward, false);
		break;
	case KC_END:
		tinput_seek_max(ti, seek_forward, false);
		break;
	case KC_UP:
		tinput_history_seek(ti, +1);
		break;
	case KC_DOWN:
		tinput_history_seek(ti, -1);
		break;
	default:
		break;
	}
}
