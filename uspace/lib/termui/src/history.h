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
#include <stdbool.h>

#define INTERNAL __attribute__((visibility("internal")))

static bool _cell_is_empty(const termui_cell_t cell)
{
	return cell.glyph_idx == 0 && cell.bgcolor == 0 && cell.fgcolor == 0 &&
	    cell.padding == 0;
}

struct cell_buffer {
	termui_cell_t *buf;

	size_t head_offset;
	size_t head_top;

	/* Tail offset is implicitly zero. */
	size_t tail_top;

	size_t buf_len;
	size_t max_len;
};

struct history_line {
	size_t idx;
	size_t len;
};

struct line_buffer {
	struct history_line *buf;

	size_t head;
	size_t tail;

	size_t buf_len;
	size_t max_len;
};

struct history {
	size_t viewport_top;
	size_t row_delta;

	size_t cols;

	struct cell_buffer cells;
	struct line_buffer lines;

	bool append;
};

INTERNAL bool _scrollback_active(const struct history *history);
INTERNAL void _history_append_row(struct history *history, const termui_cell_t *b, bool last);
INTERNAL int _history_viewport_rows(const struct history *history, size_t max);
INTERNAL int _history_iter_rows(const struct history *history, int row, int count, termui_update_cb_t cb, void *udata);
INTERNAL int _history_scroll(struct history *history, int delta);
INTERNAL const termui_cell_t *_history_reflow(struct history *history, size_t new_cols, size_t *recouped);
