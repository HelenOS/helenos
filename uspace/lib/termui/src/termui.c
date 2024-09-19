/*
 * Copyright (c) 2024 Jiří Zárevúcky
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

#include <termui.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "history.h"

struct termui {
	int cols;
	int rows;

	int col;
	int row;

	bool cursor_visible;

	// How much of the screen is in use. Relevant for clrscr.
	int used_rows;

	// Row index of the first screen row in the circular screen buffer.
	int first_row;
	// rows * cols circular buffer of the current virtual screen contents.
	// Does not necessarily correspond to the currently visible text,
	// if scrollback is active.
	termui_cell_t *screen;
	// Set to one if the corresponding row has overflowed into the next row.
	uint8_t *overflow_flags;

	/* Used to remove extra newline when CRLF is placed exactly on row boundary. */
	bool overflow;

	struct history history;

	termui_cell_t style;
	termui_cell_t default_cell;

	termui_scroll_cb_t scroll_cb;
	termui_update_cb_t update_cb;
	termui_refresh_cb_t refresh_cb;
	void *scroll_udata;
	void *update_udata;
	void *refresh_udata;
};

static int _real_row(const termui_t *termui, int row)
{
	row += termui->first_row;
	if (row >= termui->rows)
		row -= termui->rows;

	return row;
}

#define _screen_cell(termui, col, row) \
	((termui)->screen[(termui)->cols * _real_row((termui), (row)) + (col)])

#define _current_cell(termui) \
	_screen_cell((termui), (termui)->col, (termui)->row)

#define _overflow_flag(termui, row) \
	((termui)->overflow_flags[_real_row((termui), (row))])

/** Sets current cell style/color.
 */
void termui_set_style(termui_t *termui, termui_cell_t style)
{
	termui->style = style;
}

static void _termui_evict_row(termui_t *termui)
{
	if (termui->used_rows <= 0)
		return;

	bool last = !_overflow_flag(termui, 0);

	for (int col = 0; col < termui->cols; col++)
		_screen_cell(termui, col, 0).cursor = 0;

	/* Append first row of the screen to history. */
	_history_append_row(&termui->history, &_screen_cell(termui, 0, 0), last);

	_overflow_flag(termui, 0) = false;

	/* Clear the row we moved to history. */
	for (int col = 0; col < termui->cols; col++)
		_screen_cell(termui, col, 0) = termui->default_cell;

	termui->used_rows--;

	termui->row--;
	if (termui->row < 0) {
		termui->row = 0;
		termui->col = 0;
	}

	termui->first_row++;
	if (termui->first_row >= termui->rows)
		termui->first_row -= termui->rows;

	assert(termui->first_row < termui->rows);
}

/**
 * Get active screen row. This always points to the primary output buffer,
 * unaffected by viewport shifting. Can be used for modifying the screen
 * directly. For displaying viewport, use termui_force_viewport_update().
 */
termui_cell_t *termui_get_active_row(termui_t *termui, int row)
{
	assert(row >= 0);
	assert(row < termui->rows);

	return &_screen_cell(termui, 0, row);
}

static void _update_active_cells(termui_t *termui, int col, int row, int cells)
{
	int viewport_rows = _history_viewport_rows(&termui->history, termui->rows);
	int active_rows_shown = termui->rows - viewport_rows;

	/* Send update if the cells are visible in viewport. */
	if (termui->update_cb && active_rows_shown > row)
		termui->update_cb(termui->update_udata, col, row + viewport_rows, &_screen_cell(termui, col, row), cells);
}

static void _update_current_cell(termui_t *termui)
{
	_update_active_cells(termui, termui->col, termui->row, 1);
}

static void _cursor_off(termui_t *termui)
{
	if (termui->cursor_visible) {
		_current_cell(termui).cursor = 0;
		_update_current_cell(termui);
	}
}

static void _cursor_on(termui_t *termui)
{
	if (termui->cursor_visible) {
		_current_cell(termui).cursor = 1;
		_update_current_cell(termui);
	}
}

static void _advance_line(termui_t *termui)
{
	if (termui->row + 1 >= termui->rows) {
		size_t old_top = termui->history.viewport_top;

		_termui_evict_row(termui);

		if (old_top != termui->history.viewport_top && termui->refresh_cb)
			termui->refresh_cb(termui->refresh_udata);

		if (termui->scroll_cb && !_scrollback_active(&termui->history))
			termui->scroll_cb(termui->scroll_udata, 1);
	}

	if (termui->rows > 1)
		termui->row++;

	if (termui->row >= termui->used_rows)
		termui->used_rows = termui->row + 1;

	assert(termui->row < termui->rows);
}

void termui_put_lf(termui_t *termui)
{
	_cursor_off(termui);
	termui->overflow = false;
	_advance_line(termui);
	_cursor_on(termui);
}

void termui_put_cr(termui_t *termui)
{
	_cursor_off(termui);

	/* CR right after overflow from previous row. */
	if (termui->overflow && termui->row > 0) {
		termui->row--;
		_overflow_flag(termui, termui->row) = 0;
	}

	termui->overflow = false;

	// Set position to start of current line.
	termui->col = 0;

	_cursor_on(termui);
}

/* Combined CR & LF to cut down on cursor update callbacks. */
void termui_put_crlf(termui_t *termui)
{
	_cursor_off(termui);

	/* CR right after overflow from previous row. */
	if (termui->overflow && termui->row > 0) {
		termui->row--;
		_overflow_flag(termui, termui->row) = 0;
	}

	termui->overflow = false;

	// Set position to start of next row.
	_advance_line(termui);
	termui->col = 0;

	_cursor_on(termui);
}

void termui_put_tab(termui_t *termui)
{
	_cursor_off(termui);

	termui->overflow = false;

	int new_col = (termui->col / 8 + 1) * 8;
	if (new_col >= termui->cols)
		new_col = termui->cols - 1;
	termui->col = new_col;

	_cursor_on(termui);
}

void termui_put_backspace(termui_t *termui)
{
	_cursor_off(termui);

	termui->overflow = false;

	if (termui->col == 0) {
		if (termui->row > 0 && _overflow_flag(termui, termui->row - 1)) {
			termui->row--;
			termui->col = termui->cols - 1;
			_overflow_flag(termui, termui->row) = false;
		}
	} else {
		termui->col--;
	}

	_cursor_on(termui);
}

/**
 * Put glyph at current position, and advance column by width, overflowing into
 * next row and scrolling the active screen if necessary.
 *
 * If width > 1, the function makes sure the glyph isn't split by end of row.
 * The following (width - 1) cells are filled with padding cells,
 * and it's the user's responsibility to render this correctly.
 */
void termui_put_glyph(termui_t *termui, uint32_t glyph_idx, int width)
{
	if (termui->row >= termui->used_rows)
		termui->used_rows = termui->row + 1;

	termui_cell_t padding_cell = termui->style;
	padding_cell.padding = 1;
	termui_cell_t cell = termui->style;
	cell.glyph_idx = glyph_idx;

	// FIXME: handle wide glyphs in history correctly after resize

	if (termui->col + width > termui->cols) {
		/* Have to go to next row first. */
		int blanks = termui->cols - termui->col;
		for (int i = 0; i < blanks; i++)
			_screen_cell(termui, termui->col + i, termui->row) = padding_cell;

		_update_active_cells(termui, termui->col, termui->row, blanks);

		_overflow_flag(termui, termui->row) = 1;
		_advance_line(termui);
		termui->col = 0;
	}

	_current_cell(termui) = cell;
	termui->col++;

	for (int i = 1; i < width; i++) {
		_current_cell(termui) = padding_cell;
		termui->col++;
	}

	if (termui->col < termui->cols) {
		/* The changed cells are all adjacent. */
		if (termui->cursor_visible)
			_current_cell(termui).cursor = 1;
		_update_active_cells(termui, termui->col - width, termui->row, width + 1);
		termui->overflow = false;
	} else {
		/* Update the written cells and then update cursor on next row. */
		_update_active_cells(termui, termui->col - width, termui->row, width);

		_overflow_flag(termui, termui->row) = 1;
		_advance_line(termui);
		termui->col = 0;
		termui->overflow = true;

		_cursor_on(termui);
	}
}

termui_color_t termui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	r = r >> 3;
	g = g >> 3;
	b = b >> 3;

	return 0x8000 | r << 10 | g << 5 | b;
}

void termui_color_to_rgb(const termui_color_t c, uint8_t *r, uint8_t *g, uint8_t *b)
{
	assert((c & 0x8000) != 0);

	/* 15b encoding, bit 15 is set to reserve lower half for other uses. */

	int bb = c & 0x1f;
	int gg = (c >> 5) & 0x1f;
	int rr = (c >> 10) & 0x1f;

	/*
	 * 3 extra low order bits are filled from high-order bits to get the full
	 * range instead of topping out at 0xf8.
	 */
	*r = (rr << 3) | (rr >> 2);
	*g = (gg << 3) | (gg >> 2);
	*b = (bb << 3) | (bb >> 2);

	assert(termui_color_from_rgb(*r, *g, *b) == c);
}

/** Get terminal width.
 */
int termui_get_cols(const termui_t *termui)
{
	return termui->cols;
}

/** Get terminal height.
 */
int termui_get_rows(const termui_t *termui)
{
	return termui->rows;
}

/** Get cursor position
 */
void termui_get_pos(const termui_t *termui, int *col, int *row)
{
	*col = termui->col;
	*row = termui->row;
}

/** Set cursor position.
 */
void termui_set_pos(termui_t *termui, int col, int row)
{
	if (col < 0)
		col = 0;

	if (col >= termui->cols)
		col = termui->cols - 1;

	if (row < 0)
		row = 0;

	if (row >= termui->rows)
		row = termui->rows - 1;

	_cursor_off(termui);

	termui->col = col;
	termui->row = row;

	_cursor_on(termui);
}

/** Clear screen by scrolling out all text currently on screen.
 * Sets position to (0, 0).
 */
void termui_clear_screen(termui_t *termui)
{
	_cursor_off(termui);
	termui_put_crlf(termui);

	int unused_rows = termui->rows - termui->used_rows;

	while (termui->used_rows > 0)
		_termui_evict_row(termui);

	/* Clear out potential garbage left by direct screen access. */
	for (int row = 0; row < unused_rows; row++) {
		for (int col = 0; col < termui->cols; col++) {
			_screen_cell(termui, col, row) = termui->default_cell;
		}
	}

	termui->row = 0;
	termui->col = 0;

	_cursor_on(termui);

	if (termui->refresh_cb)
		termui->refresh_cb(termui->refresh_udata);
}

/** Erase all text starting at the given row.
 * Erased text is not appended to history.
 * If cursor was in the erased section, it's set to the beginning of it.
 */
void termui_wipe_screen(termui_t *termui, int first_row)
{
	if (first_row >= termui->rows)
		return;

	if (first_row < 0)
		first_row = 0;

	for (int row = first_row; row < termui->rows; row++) {
		for (int col = 0; col < termui->cols; col++)
			_screen_cell(termui, col, row) = termui->default_cell;

		_overflow_flag(termui, row) = false;
		_update_active_cells(termui, 0, row, termui->cols);
	}

	if (termui->used_rows > first_row)
		termui->used_rows = first_row;

	if (termui->row >= first_row) {
		termui->row = first_row;
		termui->col = 0;
		_cursor_on(termui);
	}
}

void termui_set_scroll_cb(termui_t *termui, termui_scroll_cb_t cb, void *userdata)
{
	termui->scroll_cb = cb;
	termui->scroll_udata = userdata;
}

void termui_set_update_cb(termui_t *termui, termui_update_cb_t cb, void *userdata)
{
	termui->update_cb = cb;
	termui->update_udata = userdata;
}

void termui_set_refresh_cb(termui_t *termui, termui_refresh_cb_t cb, void *userdata)
{
	termui->refresh_cb = cb;
	termui->refresh_udata = userdata;
}

/** Makes update callbacks for all indicated viewport rows.
 * Useful when refreshing the screens or handling a scroll callback.
 */
void termui_force_viewport_update(const termui_t *termui, int first_row, int rows)
{
	assert(first_row >= 0);
	assert(rows >= 0);
	assert(first_row + rows <= termui->rows);

	if (!termui->update_cb)
		return;

	int sb_rows = _history_viewport_rows(&termui->history, termui->rows);
	int updated = _history_iter_rows(&termui->history, first_row, rows, termui->update_cb, termui->update_udata);

	first_row += updated;
	rows -= updated;

	assert(sb_rows <= first_row);

	for (int row = first_row; row < first_row + rows; row++) {
		termui->update_cb(termui->update_udata, 0, row, &_screen_cell(termui, 0, row - sb_rows), termui->cols);
	}
}

bool termui_scrollback_is_active(const termui_t *termui)
{
	return _scrollback_active(&termui->history);
}

termui_t *termui_create(int cols, int rows, size_t history_lines)
{
	/* Prevent numerical overflows. */
	if (cols < 2 || rows < 1 || INT_MAX / cols < rows)
		return NULL;

	int cells = cols * rows;

	termui_t *termui = calloc(1, sizeof(termui_t));
	if (!termui)
		return NULL;

	termui->cols = cols;
	termui->rows = rows;
	termui->history.lines.max_len = history_lines;
	if (history_lines > SIZE_MAX / cols)
		termui->history.cells.max_len = SIZE_MAX;
	else
		termui->history.cells.max_len = history_lines * cols;
	termui->history.cols = cols;

	termui->screen = calloc(cells, sizeof(termui->screen[0]));
	if (!termui->screen) {
		free(termui);
		return NULL;
	}

	termui->overflow_flags = calloc(rows, sizeof(termui->overflow_flags[0]));
	if (!termui->overflow_flags) {
		free(termui->screen);
		free(termui);
		return NULL;
	}

	return termui;
}

void termui_destroy(termui_t *termui)
{
	free(termui->screen);
	free(termui);
}

/** Scrolls the viewport.
 * Negative delta scrolls towards older rows, positive towards newer.
 * Scroll callback is called with the actual number of rows scrolled.
 * No callback is called for rows previously off-screen.
 *
 * @param termui
 * @param delta  Number of rows to scroll.
 */
void termui_history_scroll(termui_t *termui, int delta)
{
	int scrolled = _history_scroll(&termui->history, delta);

	if (scrolled != 0 && termui->scroll_cb)
		termui->scroll_cb(termui->scroll_udata, scrolled);
}

void termui_set_cursor_visibility(termui_t *termui, bool visible)
{
	if (termui->cursor_visible == visible)
		return;

	termui->cursor_visible = visible;

	_current_cell(termui).cursor = visible;
	_update_current_cell(termui);
}

bool termui_get_cursor_visibility(const termui_t *termui)
{
	return termui->cursor_visible;
}

static void _termui_put_cells(termui_t *termui, const termui_cell_t *cells, int n)
{
	while (n > 0) {
		_current_cell(termui) = cells[0];
		cells++;
		n--;

		termui->col++;

		if (termui->col == termui->cols) {
			_overflow_flag(termui, termui->row) = 1;
			_advance_line(termui);
			termui->col = 0;
			termui->overflow = true;
		} else {
			termui->overflow = false;
		}
	}

	if (termui->row >= termui->used_rows)
		termui->used_rows = termui->row + 1;
}

/** Resize active screen and scrollback depth.
 */
errno_t termui_resize(termui_t *termui, int cols, int rows, size_t history_lines)
{
	/* Prevent numerical overflows. */
	if (cols < 2 || rows < 1 || INT_MAX / cols < rows)
		return ERANGE;

	int cells = cols * rows;

	termui_cell_t *new_screen = calloc(cells, sizeof(new_screen[0]));
	if (!new_screen)
		return ENOMEM;

	uint8_t *new_flags = calloc(rows, sizeof(new_flags[0]));
	if (!new_flags) {
		free(new_screen);
		return ENOMEM;
	}

	termui_t old_termui = *termui;

	termui->rows = rows;
	termui->cols = cols;
	termui->row = 0;
	termui->col = 0;
	termui->used_rows = 0;
	termui->first_row = 0;
	termui->screen = new_screen;
	termui->overflow_flags = new_flags;
	termui->overflow = false;

	bool cursor_visible = termui->cursor_visible;
	termui->cursor_visible = false;

	termui->history.lines.max_len = history_lines;

	if (history_lines > SIZE_MAX / cols)
		termui->history.cells.max_len = SIZE_MAX;
	else
		termui->history.cells.max_len = history_lines * cols;

	/* Temporarily remove callbacks. */
	termui->scroll_cb = NULL;
	termui->update_cb = NULL;
	termui->refresh_cb = NULL;

	size_t recouped;
	const termui_cell_t *c = _history_reflow(&termui->history, cols, &recouped);

	/* Return piece of the incomplete line in scrollback back to active screen. */
	if (recouped > 0)
		_termui_put_cells(termui, c, recouped);

	/* Mark cursor position. */
	_current_cell(&old_termui).cursor = 1;

	/* Write the contents of old screen into the new one. */
	for (int row = 0; row < old_termui.used_rows; row++) {
		int real_row_offset = _real_row(&old_termui, row) * old_termui.cols;

		if (_overflow_flag(&old_termui, row)) {
			_termui_put_cells(termui, &old_termui.screen[real_row_offset], old_termui.cols);
		} else {
			/* Trim trailing blanks. */
			int len = old_termui.cols;
			while (len > 0 && _cell_is_empty(old_termui.screen[real_row_offset + len - 1]))
				len--;

			_termui_put_cells(termui, &old_termui.screen[real_row_offset], len);

			/* Mark cursor at the end of row, if any. */
			if (len < old_termui.cols)
				_current_cell(termui).cursor = old_termui.screen[real_row_offset + len].cursor;

			if (row < old_termui.used_rows - 1)
				termui_put_crlf(termui);
		}
	}

	/* Find cursor */
	int new_col = 0;
	int new_row = 0;
	for (int col = 0; col < termui->cols; col++) {
		for (int row = 0; row < termui->rows; row++) {
			if (_screen_cell(termui, col, row).cursor) {
				_screen_cell(termui, col, row).cursor = 0;
				new_col = col;
				new_row = row;
			}
		}
	}

	free(old_termui.screen);
	free(old_termui.overflow_flags);

	termui->col = new_col;
	termui->row = new_row;

	termui->cursor_visible = cursor_visible;
	_cursor_on(termui);

	termui->scroll_cb = old_termui.scroll_cb;
	termui->update_cb = old_termui.update_cb;
	termui->refresh_cb = old_termui.refresh_cb;

	if (termui->refresh_cb)
		termui->refresh_cb(termui->refresh_udata);

	return EOK;
}
