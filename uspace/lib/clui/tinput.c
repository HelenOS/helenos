/*
 * Copyright (c) 2011 Jiri Svoboda
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
#include <stdbool.h>
#include <tinput.h>

#define LIN_TO_COL(ti, lpos) ((lpos) % ((ti)->con_cols))
#define LIN_TO_ROW(ti, lpos) ((lpos) / ((ti)->con_cols))
#define LIN_POS(ti, col, row) ((col) + (row) * (ti)->con_cols)

/** Seek direction */
typedef enum {
	seek_backward = -1,
	seek_forward = 1
} seek_dir_t;

static void tinput_init(tinput_t *);
static void tinput_insert_string(tinput_t *, const char *);
static void tinput_sel_get_bounds(tinput_t *, size_t *, size_t *);
static bool tinput_sel_active(tinput_t *);
static void tinput_sel_all(tinput_t *);
static void tinput_sel_delete(tinput_t *);
static void tinput_key_ctrl(tinput_t *, kbd_event_t *);
static void tinput_key_shift(tinput_t *, kbd_event_t *);
static void tinput_key_ctrl_shift(tinput_t *, kbd_event_t *);
static void tinput_key_unmod(tinput_t *, kbd_event_t *);
static void tinput_pre_seek(tinput_t *, bool);
static void tinput_post_seek(tinput_t *, bool);

static void tinput_console_set_lpos(tinput_t *ti, unsigned lpos)
{
	console_set_pos(ti->console, LIN_TO_COL(ti, lpos),
	    LIN_TO_ROW(ti, lpos));
}

/** Create a new text input field. */
tinput_t *tinput_new(void)
{
	tinput_t *ti;

	ti = calloc(1, sizeof(tinput_t));
	if (ti == NULL)
		return NULL;

	tinput_init(ti);
	return ti;
}

/** Destroy text input field. */
void tinput_destroy(tinput_t *ti)
{
	if (ti->prompt != NULL)
		free(ti->prompt);
	free(ti);
}

static void tinput_display_prompt(tinput_t *ti)
{
	tinput_console_set_lpos(ti, ti->prompt_coord);

	console_set_style(ti->console, STYLE_EMPHASIS);
	printf("%s", ti->prompt);
	console_flush(ti->console);
	console_set_style(ti->console, STYLE_NORMAL);
}

static void tinput_display_tail(tinput_t *ti, size_t start, size_t pad)
{
	wchar_t *dbuf = malloc((INPUT_MAX_SIZE + 1) * sizeof(wchar_t));
	if (!dbuf)
		return;

	size_t sa;
	size_t sb;
	tinput_sel_get_bounds(ti, &sa, &sb);

	tinput_console_set_lpos(ti, ti->text_coord + start);
	console_set_style(ti->console, STYLE_NORMAL);

	size_t p = start;
	if (p < sa) {
		memcpy(dbuf, ti->buffer + p, (sa - p) * sizeof(wchar_t));
		dbuf[sa - p] = '\0';
		printf("%ls", dbuf);
		p = sa;
	}

	if (p < sb) {
		console_flush(ti->console);
		console_set_style(ti->console, STYLE_SELECTED);

		memcpy(dbuf, ti->buffer + p,
		    (sb - p) * sizeof(wchar_t));
		dbuf[sb - p] = '\0';
		printf("%ls", dbuf);
		p = sb;
	}

	console_flush(ti->console);
	console_set_style(ti->console, STYLE_NORMAL);

	if (p < ti->nc) {
		memcpy(dbuf, ti->buffer + p,
		    (ti->nc - p) * sizeof(wchar_t));
		dbuf[ti->nc - p] = '\0';
		printf("%ls", dbuf);
	}

	for (p = 0; p < pad; p++)
		putchar(' ');

	console_flush(ti->console);

	free(dbuf);
}

static char *tinput_get_str(tinput_t *ti)
{
	return wstr_to_astr(ti->buffer);
}

static void tinput_position_caret(tinput_t *ti)
{
	tinput_console_set_lpos(ti, ti->text_coord + ti->pos);
}

/** Update text_coord, prompt_coord in case the screen could have scrolled. */
static void tinput_update_origin(tinput_t *ti)
{
	unsigned end_coord = ti->text_coord + ti->nc;
	unsigned end_row = LIN_TO_ROW(ti, end_coord);

	unsigned scroll_rows;

	/* Update coords if the screen scrolled. */
	if (end_row >= ti->con_rows) {
		scroll_rows = end_row - ti->con_rows + 1;
		ti->text_coord -= ti->con_cols * scroll_rows;
		ti->prompt_coord -= ti->con_cols * scroll_rows;
	}
}

static void tinput_jump_after(tinput_t *ti)
{
	tinput_console_set_lpos(ti, ti->text_coord + ti->nc);
	console_flush(ti->console);
	putchar('\n');
}

static errno_t tinput_display(tinput_t *ti)
{
	sysarg_t col0, row0;

	if (console_get_pos(ti->console, &col0, &row0) != EOK)
		return EIO;

	ti->prompt_coord = row0 * ti->con_cols + col0;
	ti->text_coord = ti->prompt_coord + str_length(ti->prompt);

	tinput_display_prompt(ti);
	tinput_display_tail(ti, 0, 0);
	tinput_position_caret(ti);

	return EOK;
}

static void tinput_insert_char(tinput_t *ti, wchar_t c)
{
	if (ti->nc == INPUT_MAX_SIZE)
		return;

	unsigned new_width = LIN_TO_COL(ti, ti->text_coord) + ti->nc + 1;
	if (new_width % ti->con_cols == 0) {
		/* Advancing to new line. */
		sysarg_t new_height = (new_width / ti->con_cols) + 1;
		if (new_height >= ti->con_rows) {
			/* Disallow text longer than 1 page for now. */
			return;
		}
	}

	size_t i;
	for (i = ti->nc; i > ti->pos; i--)
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
	size_t ilen = min(str_length(str), INPUT_MAX_SIZE - ti->nc);
	if (ilen == 0)
		return;

	unsigned new_width = LIN_TO_COL(ti, ti->text_coord) + ti->nc + ilen;
	unsigned new_height = (new_width / ti->con_cols) + 1;
	if (new_height >= ti->con_rows) {
		/* Disallow text longer than 1 page for now. */
		return;
	}

	if (ti->nc > 0) {
		size_t i;
		for (i = ti->nc; i > ti->pos; i--)
			ti->buffer[i + ilen - 1] = ti->buffer[i - 1];
	}

	size_t off = 0;
	size_t i = 0;
	while (i < ilen) {
		wchar_t c = str_decode(str, &off, STR_NO_LIMIT);
		if (c == '\0')
			break;

		/* Filter out non-printable chars. */
		if (c < 32)
			c = 32;

		ti->buffer[ti->pos + i] = c;
		i++;
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
	if (tinput_sel_active(ti)) {
		tinput_sel_delete(ti);
		return;
	}

	if (ti->pos == 0)
		return;

	size_t i;
	for (i = ti->pos; i < ti->nc; i++)
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

		while (true) {
			ti->pos += 1;

			if (ti->pos == ti->nc)
				break;

			if ((ti->buffer[ti->pos - 1] == ' ') &&
			    (ti->buffer[ti->pos] != ' '))
				break;
		}
	} else {
		if (ti->pos == 0)
			return;

		while (true) {
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
		if (ti->pos >= ti->con_cols)
			ti->pos = ti->pos - ti->con_cols;
	}

	tinput_post_seek(ti, shift_held);
}

static void tinput_seek_scrpos(tinput_t *ti, int col, int line, bool shift_held)
{
	unsigned lpos;
	tinput_pre_seek(ti, shift_held);

	lpos = LIN_POS(ti, col, line);

	if (lpos > ti->text_coord)
		ti->pos = lpos -  ti->text_coord;
	else
		ti->pos = 0;
	if (ti->pos > ti->nc)
		ti->pos = ti->nc;

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
	if ((tinput_sel_active(ti)) && (!shift_held)) {
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
	if (ti->hnum < HISTORY_LEN) {
		ti->hnum += 1;
	} else {
		if (ti->history[HISTORY_LEN] != NULL)
			free(ti->history[HISTORY_LEN]);
	}

	size_t i;
	for (i = ti->hnum; i > 1; i--)
		ti->history[i] = ti->history[i - 1];

	ti->history[1] = str_dup(str);

	if (ti->history[0] != NULL) {
		free(ti->history[0]);
		ti->history[0] = NULL;
	}
}

static void tinput_set_str(tinput_t *ti, const char *str)
{
	str_to_wstr(ti->buffer, INPUT_MAX_SIZE, str);
	ti->nc = wstr_length(ti->buffer);
	ti->pos = ti->nc;
	ti->sel_start = ti->pos;
}

static void tinput_sel_get_bounds(tinput_t *ti, size_t *sa, size_t *sb)
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
	return (ti->sel_start != ti->pos);
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
	size_t sa;
	size_t sb;

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
	size_t sa;
	size_t sb;

	tinput_sel_get_bounds(ti, &sa, &sb);

	char *str;

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
	/* TODO: Give the user some kind of warning. */
	return;
}

static void tinput_paste_from_cb(tinput_t *ti)
{
	char *str;
	errno_t rc = clipboard_get_str(&str);

	if ((rc != EOK) || (str == NULL)) {
		/* TODO: Give the user some kind of warning. */
		return;
	}

	tinput_insert_string(ti, str);
	free(str);
}

static void tinput_history_seek(tinput_t *ti, int offs)
{
	if (offs >= 0) {
		if (ti->hpos + offs > ti->hnum)
			return;
	} else {
		if (ti->hpos < (size_t) -offs)
			return;
	}

	if (ti->history[ti->hpos] != NULL) {
		free(ti->history[ti->hpos]);
		ti->history[ti->hpos] = NULL;
	}

	ti->history[ti->hpos] = tinput_get_str(ti);
	ti->hpos += offs;

	int pad = (int) ti->nc - str_length(ti->history[ti->hpos]);
	if (pad < 0)
		pad = 0;

	tinput_set_str(ti, ti->history[ti->hpos]);
	tinput_display_tail(ti, 0, pad);
	tinput_update_origin(ti);
	tinput_position_caret(ti);
}

/** Compare two entries in array of completions. */
static int compl_cmp(const void *va, const void *vb)
{
	const char *a = *(const char **) va;
	const char *b = *(const char **) vb;

	return str_cmp(a, b);
}

static size_t common_pref_len(const char *a, const char *b)
{
	size_t i;
	size_t a_off, b_off;
	wchar_t ca, cb;

	i = 0;
	a_off = 0;
	b_off = 0;

	while (true) {
		ca = str_decode(a, &a_off, STR_NO_LIMIT);
		cb = str_decode(b, &b_off, STR_NO_LIMIT);

		if (ca == '\0' || cb == '\0' || ca != cb)
			break;
		++i;
	}

	return i;
}

/* Print a list of completions */
static void tinput_show_completions(tinput_t *ti, char **compl, size_t cnum)
{
	unsigned int i;
	/* Determine the maximum width of the completion in chars */
	size_t max_width = 0;
	for (i = 0; i < cnum; i++)
		max_width = max(max_width, str_width(compl[i]));

	unsigned int cols = max(1, (ti->con_cols + 1) / (max_width + 1));
	unsigned int padding = 0;
	if (cols * max_width + (cols - 1) < ti->con_cols) {
		padding = ti->con_cols - cols * max_width - (cols - 1);
	}
	unsigned int col_width = max_width + padding / cols;
	unsigned int rows = cnum / cols + ((cnum % cols) != 0);

	unsigned int row, col;

	for (row = 0; row < rows; row++) {
		unsigned int display_col = 0;
		for (col = 0; col < cols; col++) {
			size_t compl_idx = col * rows + row;
			if (compl_idx >= cnum)
				break;
			if (col) {
				printf(" ");
				display_col++;
			}
			printf("%s", compl[compl_idx]);
			size_t compl_width = str_width(compl[compl_idx]);
			display_col += compl_width;
			if (col < cols - 1) {
				for (i = compl_width; i < col_width; i++) {
					printf(" ");
					display_col++;
				}
			}
		}
		if ((display_col % ti->con_cols) > 0)
			printf("\n");
	}
	fflush(stdout);
}


static void tinput_text_complete(tinput_t *ti)
{
	void *state;
	size_t cstart;
	char *ctmp;
	char **compl;     	/* Array of completions */
	size_t compl_len;	/* Current length of @c compl array */
	size_t cnum;
	size_t i;
	errno_t rc;

	if (ti->compl_ops == NULL)
		return;

	/*
	 * Obtain list of all possible completions (growing array).
	 */

	rc = (*ti->compl_ops->init)(ti->buffer, ti->pos, &cstart, &state);
	if (rc != EOK)
		return;

	cnum = 0;

	compl_len = 1;
	compl = malloc(compl_len * sizeof(char *));
	if (compl == NULL) {
		printf("Error: Out of memory.\n");
		return;
	}

	while (true) {
		rc = (*ti->compl_ops->get_next)(state, &ctmp);
		if (rc != EOK)
			break;

		if (cnum >= compl_len) {
			/* Extend array */
			compl_len = 2 * compl_len;
			compl = realloc(compl, compl_len * sizeof(char *));
			if (compl == NULL) {
				printf("Error: Out of memory.\n");
				break;
			}
		}

		compl[cnum] = str_dup(ctmp);
		if (compl[cnum] == NULL) {
			printf("Error: Out of memory.\n");
			break;
		}
		cnum++;
	}

	(*ti->compl_ops->fini)(state);

	if (cnum > 1) {
		/*
		 * More than one match. Determine maximum common prefix.
		 */
		size_t cplen;

		cplen = str_length(compl[0]);
		for (i = 1; i < cnum; i++)
			cplen = min(cplen, common_pref_len(compl[0], compl[i]));

		/* Compute how many bytes we should skip. */
		size_t istart = str_lsize(compl[0], ti->pos - cstart);

		if (cplen > istart) {
			/* Insert common prefix. */

			/* Copy remainder of common prefix. */
			char *cpref = str_ndup(compl[0] + istart,
			    str_lsize(compl[0], cplen - istart));

			/* Insert it. */
			tinput_insert_string(ti, cpref);
			free(cpref);
		} else {
			/* No common prefix. Sort and display all entries. */

			qsort(compl, cnum, sizeof(char *), compl_cmp);

			tinput_jump_after(ti);
			tinput_show_completions(ti, compl, cnum);
			tinput_display(ti);
		}
	} else if (cnum == 1) {
		/*
		 * We have exactly one match. Insert it.
		 */

		/* Compute how many bytes of completion string we should skip. */
		size_t istart = str_lsize(compl[0], ti->pos - cstart);

		/* Insert remainder of completion string at current position. */
		tinput_insert_string(ti, compl[0] + istart);
	}

	for (i = 0; i < cnum; i++)
		free(compl[i]);
	free(compl);
}

/** Initialize text input field.
 *
 * Must be called before using the field. It clears the history.
 */
static void tinput_init(tinput_t *ti)
{
	ti->console = console_init(stdin, stdout);
	ti->hnum = 0;
	ti->hpos = 0;
	ti->history[0] = NULL;
}

/** Set prompt string.
 *
 * @param ti		Text input
 * @param prompt	Prompt string
 *
 * @return		EOK on success, ENOMEM if out of memory.
 */
errno_t tinput_set_prompt(tinput_t *ti, const char *prompt)
{
	if (ti->prompt != NULL)
		free(ti->prompt);

	ti->prompt = str_dup(prompt);
	if (ti->prompt == NULL)
		return ENOMEM;

	return EOK;
}

/** Set completion ops.
 *
 * Set pointer to completion ops structure that will be used for text
 * completion.
 */
void tinput_set_compl_ops(tinput_t *ti, tinput_compl_ops_t *compl_ops)
{
	ti->compl_ops = compl_ops;
}

/** Handle key press event. */
static void tinput_key_press(tinput_t *ti, kbd_event_t *kev)
{
	if (kev->key == KC_LSHIFT)
		ti->lshift_held = true;
	if (kev->key == KC_RSHIFT)
		ti->rshift_held = true;

	if (((kev->mods & KM_CTRL) != 0) &&
	    ((kev->mods & (KM_ALT | KM_SHIFT)) == 0))
		tinput_key_ctrl(ti, kev);

	if (((kev->mods & KM_SHIFT) != 0) &&
	    ((kev->mods & (KM_CTRL | KM_ALT)) == 0))
		tinput_key_shift(ti, kev);

	if (((kev->mods & KM_CTRL) != 0) &&
	    ((kev->mods & KM_SHIFT) != 0) &&
	    ((kev->mods & KM_ALT) == 0))
		tinput_key_ctrl_shift(ti, kev);

	if ((kev->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0)
		tinput_key_unmod(ti, kev);

	if (kev->c >= ' ') {
		tinput_sel_delete(ti);
		tinput_insert_char(ti, kev->c);
	}
}

/** Handle key release event. */
static void tinput_key_release(tinput_t *ti, kbd_event_t *kev)
{
	if (kev->key == KC_LSHIFT)
		ti->lshift_held = false;
	if (kev->key == KC_RSHIFT)
		ti->rshift_held = false;
}

/** Position event */
static void tinput_pos(tinput_t *ti, pos_event_t *ev)
{
	if (ev->type == POS_PRESS) {
		tinput_seek_scrpos(ti, ev->hpos, ev->vpos,
		    ti->lshift_held || ti->rshift_held);
	}
}

/** Read in one line of input with initial text provided.
 *
 * @param ti   Text input
 * @param istr Initial string
 * @param dstr Place to save pointer to new string
 *
 * @return EOK on success
 * @return ENOENT if user requested abort
 * @return EIO if communication with console failed
 *
 */
errno_t tinput_read_i(tinput_t *ti, const char *istr, char **dstr)
{
	console_flush(ti->console);
	if (console_get_size(ti->console, &ti->con_cols, &ti->con_rows) != EOK)
		return EIO;

	tinput_set_str(ti, istr);

	ti->sel_start = 0;
	ti->done = false;
	ti->exit_clui = false;

	if (tinput_display(ti) != EOK)
		return EIO;

	while (!ti->done) {
		console_flush(ti->console);

		cons_event_t ev;
		if (!console_get_event(ti->console, &ev))
			return EIO;

		switch (ev.type) {
		case CEV_KEY:
			if (ev.ev.key.type == KEY_PRESS)
				tinput_key_press(ti, &ev.ev.key);
			else
				tinput_key_release(ti, &ev.ev.key);
			break;
		case CEV_POS:
			tinput_pos(ti, &ev.ev.pos);
			break;
		}
	}

	if (ti->exit_clui)
		return ENOENT;

	ti->pos = ti->nc;
	tinput_position_caret(ti);
	putchar('\n');

	char *str = tinput_get_str(ti);
	if (str_cmp(str, "") != 0)
		tinput_history_insert(ti, str);

	ti->hpos = 0;

	*dstr = str;
	return EOK;
}

/** Read in one line of input.
 *
 * @param ti   Text input
 * @param dstr Place to save pointer to new string.
 *
 * @return EOK on success
 * @return ENOENT if user requested abort
 * @return EIO if communication with console failed
 *
 */
errno_t tinput_read(tinput_t *ti, char **dstr)
{
	return tinput_read_i(ti, "", dstr);
}

static void tinput_key_ctrl(tinput_t *ti, kbd_event_t *ev)
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
	case KC_Q:
		/* Signal libary client to quit interactive loop. */
		ti->done = true;
		ti->exit_clui = true;
		break;
	default:
		break;
	}
}

static void tinput_key_ctrl_shift(tinput_t *ti, kbd_event_t *ev)
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

static void tinput_key_shift(tinput_t *ti, kbd_event_t *ev)
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

static void tinput_key_unmod(tinput_t *ti, kbd_event_t *ev)
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
		tinput_history_seek(ti, 1);
		break;
	case KC_DOWN:
		tinput_history_seek(ti, -1);
		break;
	case KC_TAB:
		tinput_text_complete(ti);
		break;
	default:
		break;
	}
}
