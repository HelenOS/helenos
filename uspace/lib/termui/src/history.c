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

#include "history.h"

#include <assert.h>
#include <limits.h>
#include <macros.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>

#define BLANK_CELLS_LEN 64
static const termui_cell_t _blank_cells[BLANK_CELLS_LEN];

static bool _lines_empty(struct line_buffer *lines)
{
	return lines->head == lines->tail;
}

static void _line_idx_inc(const struct line_buffer *lines, size_t *idx)
{
	if (*idx == lines->buf_len - 1)
		*idx = 0;
	else
		(*idx)++;
}

static void _line_idx_dec(const struct line_buffer *lines, size_t *idx)
{
	if (*idx == 0)
		*idx = lines->buf_len - 1;
	else
		(*idx)--;
}

////////////////////////////////////////////////////////////////////////////////

static void _cell_buffer_shrink(struct cell_buffer *cells)
{
	assert(cells->max_len > 0);
	assert(cells->buf_len > cells->max_len);

	size_t new_len = max(cells->max_len, cells->head_top);

	termui_cell_t *new_buf = reallocarray(cells->buf,
	    new_len, sizeof(termui_cell_t));

	if (new_buf) {
		cells->buf = new_buf;
		cells->buf_len = new_len;
	}
}

static void _line_buffer_shrink(struct line_buffer *lines)
{
	assert(lines->max_len > 0);
	assert(lines->buf_len > lines->max_len);
	assert(lines->head <= lines->tail);

	size_t new_len = max(lines->max_len, lines->tail + 1);

	struct history_line *new_buf = reallocarray(lines->buf,
	    new_len, sizeof(struct history_line));

	if (new_buf) {
		lines->buf = new_buf;
		lines->buf_len = new_len;
	}
}

static void _evict_cells(struct cell_buffer *cells, size_t idx, size_t len)
{
	assert(idx == cells->head_offset);
	assert(len <= cells->head_top);
	assert(idx <= cells->head_top - len);

	cells->head_offset += len;

	if (cells->head_offset >= cells->head_top) {

		cells->head_offset = 0;
		cells->head_top = cells->tail_top;
		cells->tail_top = 0;

		if (cells->buf_len > cells->max_len)
			_cell_buffer_shrink(cells);
	}
}

static bool _index_valid(const struct history *history, size_t idx)
{
	const struct line_buffer *lines = &history->lines;

	if (lines->head <= lines->tail)
		return idx >= lines->head && idx < lines->tail;
	else
		return (idx >= lines->head && idx < lines->buf_len) ||
		    (idx < lines->tail);
}

#define _history_check(history) do { \
	assert(history->lines.head < history->lines.buf_len); \
	assert(history->lines.tail < history->lines.buf_len); \
	assert(history->cells.tail_top <= history->cells.head_offset); \
	assert(history->cells.head_offset <= history->cells.head_top); \
	assert(history->cells.head_top <= history->cells.buf_len); \
	assert(_index_valid(history, history->viewport_top) || history->viewport_top == history->lines.tail); \
	if (history->append) assert(!_lines_empty(&history->lines)); \
} while (false)

static void _evict_oldest_line(struct history *history)
{
	struct line_buffer *lines = &history->lines;
	struct cell_buffer *cells = &history->cells;

	_history_check(history);

	bool head = (history->viewport_top == lines->head);

	struct history_line line = lines->buf[lines->head];
	_line_idx_inc(lines, &lines->head);

	if (lines->head == lines->tail) {
		lines->head = 0;
		lines->tail = 0;
		history->viewport_top = 0;
		history->append = false;
		history->row_delta = 0;
	}

	if (head) {
		history->viewport_top = lines->head;
		history->row_delta = 0;
	}

	_history_check(history);

	if (lines->head == 0 && lines->buf_len > lines->max_len)
		_line_buffer_shrink(lines);

	_history_check(history);

	_evict_cells(cells, line.idx, line.len);

	_history_check(history);
}

////////////////////////////////////////////////////////////////////////////////

static void _cell_buffer_try_extend(struct cell_buffer *cells, size_t len)
{
	static const size_t MIN_EXTEND_LEN = 128;

	if (cells->buf_len >= cells->max_len)
		return;

	if (cells->tail_top > 0 && len <= cells->buf_len - cells->tail_top) {
		/*
		 * Don't extend when we will have enough space, since head is gonna get
		 * wiped either way (we don't move already existing lines).
		 * This only matters when allocation has failed previously.
		 */
		return;
	}

	/* Specify a minimum initial allocation size. */
	len = max(len, MIN_EXTEND_LEN);

	/* Try to roughly double the buffer size. */
	len = max(len, cells->buf_len);

	/* Limit the new size to max_len. */
	len = min(len, cells->max_len - cells->head_top);

	size_t new_len =
	    min(cells->head_top + len, SIZE_MAX / sizeof(termui_cell_t));

	assert(new_len > cells->buf_len);
	assert(new_len <= cells->max_len);

	termui_cell_t *new_buf =
	    realloc(cells->buf, new_len * sizeof(termui_cell_t));
	if (!new_buf) {
		fprintf(stderr, "termui: Out of memory for scrollback\n");
		return;
	}

	cells->buf = new_buf;
	cells->buf_len = new_len;
}

static void _line_buffer_try_extend(struct line_buffer *lines)
{
	static const size_t MIN_EXTEND_LEN = 128;

	if (lines->buf_len >= lines->max_len)
		return;

	if (lines->tail < lines->head)
		return;

	/* Specify a minimum initial allocation size. */
	size_t len = MIN_EXTEND_LEN;

	/* Try to roughly double the buffer size. */
	len = max(len, lines->buf_len);

	/* Limit the new size to max_len. */
	len = min(len, lines->max_len - lines->buf_len);

	size_t new_len = min(lines->buf_len + len, SIZE_MAX - sizeof(struct history_line));

	assert(new_len >= lines->buf_len);
	assert(new_len <= lines->max_len);

	if (new_len == lines->buf_len)
		return;

	struct history_line *new_buf =
	    realloc(lines->buf, new_len * sizeof(struct history_line));
	if (!new_buf) {
		fprintf(stderr, "termui: Out of memory for scrollback\n");
		return;
	}

	lines->buf = new_buf;
	lines->buf_len = new_len;
}

static bool _cell_buffer_fits_line(const struct cell_buffer *cells, size_t len)
{
	if (cells->tail_top > 0) {
		return len <= cells->head_offset - cells->tail_top;
	} else {
		return len <= cells->buf_len - cells->head_top || len <= cells->head_offset;
	}
}

static struct history_line *_current_line(struct line_buffer *lines)
{
	assert(!_lines_empty(lines));
	return &lines->buf[(lines->tail ? lines->tail : lines->buf_len) - 1];
}

static void _alloc_line(struct history *history)
{
	struct line_buffer *lines = &history->lines;

	size_t idx = 0;
	if (!_lines_empty(lines))
		idx = _current_line(lines)->idx + _current_line(lines)->len;

	if (lines->buf_len == 0) {
		/* Initial allocation. */
		_line_buffer_try_extend(lines);

		if (lines->buf_len == 0) {
			fprintf(stderr, "termui: Could not allocate initial scrollback buffer\n");
			return;
		}
	}

	assert(lines->tail < lines->buf_len);

	bool viewport_inactive = (history->viewport_top == lines->tail);

	lines->tail++;

	if (lines->tail >= lines->buf_len)
		_line_buffer_try_extend(lines);

	if (lines->tail >= lines->buf_len)
		lines->tail = 0;

	if (lines->tail == lines->head)
		_evict_oldest_line(history);

	assert(lines->tail != lines->head);

	if (viewport_inactive)
		history->viewport_top = lines->tail;

	_current_line(lines)->idx = idx;
	_current_line(lines)->len = 0;

	history->append = true;

	_history_check(history);
}

/** Allocate a line of cells in the cell buffer.
 * @return Index of first allocated cell in the buffer.
 */
static size_t _alloc_cells(struct cell_buffer *cells, size_t len)
{
	assert(_cell_buffer_fits_line(cells, len));

	size_t idx;

	if (cells->tail_top == 0 && cells->buf_len - cells->head_top >= len) {
		idx = cells->head_top;
		cells->head_top += len;
		assert(cells->head_top <= cells->buf_len);
	} else {
		idx = cells->tail_top;
		cells->tail_top += len;
		assert(cells->tail_top <= cells->head_offset);
	}

	return idx;
}

static termui_cell_t *_history_append(struct history *history, size_t len)
{
	struct line_buffer *lines = &history->lines;
	struct cell_buffer *cells = &history->cells;

	/*
	 * Ideally, buffer gets reallocated to its maximum size
	 * before we start recycling it.
	 */
	if (!_cell_buffer_fits_line(cells, len))
		_cell_buffer_try_extend(cells, len);

	if (len > cells->buf_len) {
		/*
		 * This can only happen if allocation fails early on,
		 * since len is normally limited to row width.
		 */
		return NULL;
	}

	/* Recycle old lines to make space in the buffer. */
	while (!_cell_buffer_fits_line(cells, len)) {
		assert(!_lines_empty(lines));
		_evict_oldest_line(history);
	}

	/* Allocate cells for the line. */
	size_t idx = _alloc_cells(cells, len);

	/* Allocate the line, if necessary. */
	if (!history->append || _lines_empty(lines)) {
		_alloc_line(history);

		if (_lines_empty(lines)) {
			/* Initial allocation failed. */
			return NULL;
		}
	}

	struct history_line *line = _current_line(lines);

	assert(idx == line->idx + line->len || idx == 0);

	/* Deal with crossing the buffer's edge. */
	if (idx != line->idx + line->len) {
		if (line->len > 0) {
			/* Breaks off an incomplete line at the end of buffer. */
			_alloc_line(history);
			line = _current_line(lines);
		}

		line->idx = 0;
	}

	line->len += len;

	return &cells->buf[idx];
}

/**
 * @param history
 * @return True if the top row of the viewport is a scrollback row.
 */
bool _scrollback_active(const struct history *history)
{
	if (history->viewport_top == history->lines.tail)
		return false;

	assert(_index_valid(history, history->viewport_top));
	return true;
}

static size_t _history_line_rows(const struct history *history, size_t idx)
{
	assert(_index_valid(history, idx));

	struct history_line line = history->lines.buf[idx];

	if (line.len == 0)
		return 1;

	return (line.len - 1) / history->cols + 1;
}

static int _history_scroll_down(struct history *history, int requested)
{
	assert(requested > 0);

	size_t delta = requested;

	/* Skip first line. */

	if (history->row_delta > 0) {
		size_t rows = _history_line_rows(history, history->viewport_top);
		assert(rows > history->row_delta);

		if (delta < rows - history->row_delta) {
			history->row_delta += delta;
			_history_check(history);
			return requested;
		}

		delta -= rows - history->row_delta;
		history->row_delta = 0;

		_line_idx_inc(&history->lines, &history->viewport_top);
	}

	/* Skip as many lines as necessary. */

	while (_scrollback_active(history)) {
		size_t rows = _history_line_rows(history, history->viewport_top);

		if (delta < rows) {
			/* Found the right line. */
			history->row_delta = delta;
			_history_check(history);
			return requested;
		}

		delta -= rows;

		_line_idx_inc(&history->lines, &history->viewport_top);
	}

	/* Scrolled past the end of history. */
	_history_check(history);
	return requested - delta;
}

static int _history_scroll_up(struct history *history, int requested)
{
	assert(requested < 0);

	/* Prevent overflow. */
	if (history->row_delta > INT_MAX) {
		history->row_delta += requested;
		_history_check(history);
		return requested;
	}

	int delta = requested + (int) history->row_delta;
	history->row_delta = 0;

	while (delta < 0 && history->viewport_top != history->lines.head) {
		_line_idx_dec(&history->lines, &history->viewport_top);

		size_t rows = _history_line_rows(history, history->viewport_top);

		if (rows > INT_MAX) {
			history->row_delta = rows + delta;
			_history_check(history);
			return requested;
		}

		delta += (int) rows;
	}

	_history_check(history);

	if (delta < 0)
		return requested - delta;

	assert(delta >= 0);
	history->row_delta = (size_t) delta;
	return requested;
}

static int _history_scroll_to_top(struct history *history)
{
	history->viewport_top = history->lines.head;
	history->row_delta = 0;
	_history_check(history);
	return INT_MIN;
}

static int _history_scroll_to_bottom(struct history *history)
{
	history->viewport_top = history->lines.tail;
	_history_check(history);
	return INT_MAX;
}

/** Scroll the viewport by the given number of rows.
 *
 * @param history
 * @param delta  How many rows to scroll. Negative delta scrolls upward.
 * @return  How many rows have actually been scrolled before top/bottom.
 */
int _history_scroll(struct history *history, int delta)
{
	if (delta == INT_MIN)
		return _history_scroll_to_top(history);
	if (delta == INT_MAX)
		return _history_scroll_to_bottom(history);
	if (delta > 0)
		return _history_scroll_down(history, delta);
	if (delta < 0)
		return _history_scroll_up(history, delta);

	return 0;
}

/** Sets new width for the viewport, recalculating current position so that the
 * top viewport row remains in place, and returning a piece of the last history
 * line if the top active screen row is a continuation of it.
 *
 * @param history
 * @param new_cols       New column width of the viewport.
 * @param[out] recouped  Number of cells returned to active screen.
 * @return  Pointer to the cell data for returned cells.
 */
const termui_cell_t *_history_reflow(struct history *history, size_t new_cols, size_t *recouped)
{
	history->row_delta = (history->row_delta * history->cols) / new_cols;
	history->cols = new_cols;

	if (!history->append) {
		*recouped = 0;
		return NULL;
	}

	/* Return the part of last line that's not aligned at row boundary. */
	assert(!_lines_empty(&history->lines));

	size_t last_idx = history->lines.tail;
	_line_idx_dec(&history->lines, &last_idx);

	struct history_line *last = &history->lines.buf[last_idx];
	*recouped = last->len % new_cols;

	if (last->idx + last->len == history->cells.head_top) {
		history->cells.head_top -= *recouped;
	} else {
		assert(last->idx + last->len == history->cells.tail_top);
		history->cells.tail_top -= *recouped;
	}

	last->len -= *recouped;
	if (last->len == 0 && last->idx == 0) {
		assert(history->cells.tail_top == 0);
		last->idx = history->cells.head_top;
	}

	return &history->cells.buf[last->idx + last->len];
}

/** Counts the number of scrollback rows present in the viewport.
 *
 * @param history
 * @param max  Number of viewport rows.
 * @return Count.
 */
int _history_viewport_rows(const struct history *history, size_t max)
{
	if (!_scrollback_active(history))
		return 0;

	size_t current = history->viewport_top;
	size_t rows = _history_line_rows(history, current) - history->row_delta;
	_line_idx_inc(&history->lines, &current);

	while (rows < max && current != history->lines.tail) {
		rows += _history_line_rows(history, current);
		_line_idx_inc(&history->lines, &current);
	}

	return (rows > max) ? max : rows;
}

static void _update_blank(int col, int row, int len, termui_update_cb_t cb, void *udata)
{
	while (len > BLANK_CELLS_LEN) {
		cb(udata, col, row, _blank_cells, BLANK_CELLS_LEN);
		col += BLANK_CELLS_LEN;
		len -= BLANK_CELLS_LEN;
	}

	if (len > 0)
		cb(udata, col, row, _blank_cells, len);
}

static void _adjust_row_delta(const struct history *history, size_t *line_idx, size_t *delta)
{
	while (*line_idx != history->lines.tail) {
		size_t rows = _history_line_rows(history, *line_idx);
		if (*delta < rows)
			return;

		*delta -= rows;
		_line_idx_inc(&history->lines, line_idx);
	}
}

/** Run update callback for a range of visible scrollback srows.
 *
 * @param history
 * @param row    First viewport row we want to update.
 * @param count  Number of viewport rows we want to update.
 * @param cb     Callback to call for every row.
 * @param udata  Callback userdata.
 * @return  Actual number of rows updated (may be less than count if
 *     the rest of rows are from the active screen).
 */
int _history_iter_rows(const struct history *history, int row, int count, termui_update_cb_t cb, void *udata)
{
	assert(history->row_delta <= SIZE_MAX - row);

	size_t current_line = history->viewport_top;
	size_t delta = history->row_delta + (size_t) row;
	/* Get to the first row to be returned. */
	_adjust_row_delta(history, &current_line, &delta);

	int initial_count = count;

	while (count > 0 && current_line != history->lines.tail) {
		/* Process each line. */
		assert(_index_valid(history, current_line));

		struct history_line line = history->lines.buf[current_line];
		assert(line.len <= history->cells.buf_len);
		assert(line.idx <= history->cells.buf_len - line.len);

		if (line.len == 0) {
			/* Special case for empty line. */
			_update_blank(0, row, history->cols, cb, udata);
			row++;
			count--;
			_line_idx_inc(&history->lines, &current_line);
			continue;
		}

		const termui_cell_t *cells = &history->cells.buf[line.idx];
		size_t line_offset = delta * history->cols;
		assert(line_offset < line.len);
		delta = 0;

		/* Callback for each full row. */
		while (count > 0 && line_offset + history->cols <= line.len) {
			assert(line.idx + line_offset <= history->cells.buf_len - history->cols);
			cb(udata, 0, row, &cells[line_offset], history->cols);

			line_offset += history->cols;
			row++;
			count--;
		}

		if (count > 0 && line_offset < line.len) {
			/* Callback for the last (incomplete) row. */

			cb(udata, 0, row, &cells[line_offset], line.len - line_offset);

			size_t col = line.len - line_offset;
			assert(col < history->cols);

			/* Callbacks for the blank section in the last row. */
			_update_blank(col, row, history->cols - col, cb, udata);

			row++;
			count--;
		}

		_line_idx_inc(&history->lines, &current_line);
	}

	return initial_count - count;
}

/** Append a row from active screen to scrollback history.
 *
 * @param history
 * @param b     Pointer to the row in active screen buffer.
 * @param last  False if the row was overflowed, meaning the next row will be
 *              appended to the same history line as this row.
 */
void _history_append_row(struct history *history, const termui_cell_t *b, bool last)
{
	size_t len = history->cols;

	/* Reduce multiple trailing empty cells to just one. */
	if (last) {
		while (len > 1 && _cell_is_empty(b[len - 1]) && _cell_is_empty(b[len - 2]))
			len--;
	}

	memcpy(_history_append(history, len), b, sizeof(termui_cell_t) * len);

	if (last)
		history->append = false;
}
