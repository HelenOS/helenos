/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup liboutput
 * @{
 */
/** @file
 */

#include <io/style.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <as.h>
#include <io/chargrid.h>

/** Create a chargrid.
 *
 * @param[in] cols  Number of columns.
 * @param[in] rows  Number of rows.
 * @param[in] flags Chargrid flags.
 *
 * @return New chargrid.
 * @return NULL on failure.
 *
 */
chargrid_t *chargrid_create(sysarg_t cols, sysarg_t rows,
    chargrid_flag_t flags)
{
	size_t size =
	    sizeof(chargrid_t) + cols * rows * sizeof(charfield_t);
	chargrid_t *scrbuf;

	if ((flags & CHARGRID_FLAG_SHARED) == CHARGRID_FLAG_SHARED) {
		scrbuf = (chargrid_t *) as_area_create(AS_AREA_ANY, size,
		    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE,
		    AS_AREA_UNPAGED);
		if (scrbuf == AS_MAP_FAILED)
			return NULL;
	} else {
		scrbuf = (chargrid_t *) malloc(size);
		if (scrbuf == NULL)
			return NULL;
	}

	scrbuf->size = size;
	scrbuf->flags = flags;
	scrbuf->cols = cols;
	scrbuf->rows = rows;
	scrbuf->cursor_visible = false;

	scrbuf->attrs.type = CHAR_ATTR_STYLE;
	scrbuf->attrs.val.style = STYLE_NORMAL;

	scrbuf->top_row = 0;
	chargrid_clear(scrbuf);

	return scrbuf;
}

void chargrid_destroy(chargrid_t *srcbuf)
{
	// TODO
}

bool chargrid_cursor_at(chargrid_t *scrbuf, sysarg_t col, sysarg_t row)
{
	return ((scrbuf->cursor_visible) && (scrbuf->col == col) &&
	    (scrbuf->row == row));
}

sysarg_t chargrid_get_top_row(chargrid_t *scrbuf)
{
	return scrbuf->top_row;
}

static sysarg_t chargrid_update_rows(chargrid_t *scrbuf)
{
	if (scrbuf->row == scrbuf->rows) {
		scrbuf->row = scrbuf->rows - 1;
		scrbuf->top_row = (scrbuf->top_row + 1) % scrbuf->rows;
		chargrid_clear_row(scrbuf, scrbuf->row);

		return scrbuf->rows;
	}

	return 2;
}

static sysarg_t chargrid_update_cols(chargrid_t *scrbuf)
{
	/* Column overflow */
	if (scrbuf->col == scrbuf->cols) {
		scrbuf->col = 0;
		scrbuf->row++;
		return chargrid_update_rows(scrbuf);
	}

	return 1;
}

/** Store one character to chargrid.
 *
 * Its position is determined by scrbuf->col
 * and scrbuf->row.
 *
 * @param scrbuf Chargrid.
 * @param ch     Character to store.
 * @param update Update coordinates.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to a new row, this value is 2.
 *
 */
sysarg_t chargrid_putuchar(chargrid_t *scrbuf, char32_t ch, bool update)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);

	charfield_t *field =
	    chargrid_charfield_at(scrbuf, scrbuf->col, scrbuf->row);

	field->ch = ch;
	field->attrs = scrbuf->attrs;
	field->flags |= CHAR_FLAG_DIRTY;

	if (update) {
		scrbuf->col++;
		return chargrid_update_cols(scrbuf);
	}

	return 1;
}

/** Jump to a new row in chargrid.
 *
 * @param scrbuf Chargrid.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 2 (the original row and the new
 *         row).
 *
 */
sysarg_t chargrid_newline(chargrid_t *scrbuf)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);

	scrbuf->col = 0;
	scrbuf->row++;

	return chargrid_update_rows(scrbuf);
}

/** Jump to a new row in chargrid.
 *
 * @param scrbuf   Chargrid.
 * @param tab_size Tab size.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to a new row, this value is 2.
 *
 */
sysarg_t chargrid_tabstop(chargrid_t *scrbuf, sysarg_t tab_size)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);

	sysarg_t spaces = tab_size - scrbuf->cols % tab_size;
	sysarg_t flush = 1;

	for (sysarg_t i = 0; i < spaces; i++)
		flush += chargrid_putuchar(scrbuf, ' ', true) - 1;

	return flush;
}

/** Jump to the previous character in chargrid.
 *
 * Currently no scrollback is supported.
 *
 * @param scrbuf Chargrid.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to the previous row, this value is 2.
 * @return 0 if no backspace is possible.
 *
 */
sysarg_t chargrid_backspace(chargrid_t *scrbuf)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);

	if ((scrbuf->col == 0) && (scrbuf->row == 0))
		return 0;

	if (scrbuf->col == 0) {
		scrbuf->col = scrbuf->cols - 1;
		scrbuf->row--;

		chargrid_putuchar(scrbuf, ' ', false);
		return 2;
	}

	scrbuf->col--;
	chargrid_putuchar(scrbuf, ' ', false);
	return 1;
}

/** Clear the chargrid.
 *
 * @param scrbuf Chargrid.
 *
 */
void chargrid_clear(chargrid_t *scrbuf)
{
	for (size_t pos = 0; pos < (scrbuf->cols * scrbuf->rows); pos++) {
		scrbuf->data[pos].ch = 0;
		scrbuf->data[pos].attrs = scrbuf->attrs;
		scrbuf->data[pos].flags = CHAR_FLAG_DIRTY;
	}

	scrbuf->col = 0;
	scrbuf->row = 0;
}

/** Update current chargrid coordinates
 *
 * @param scrbuf Chargrid.
 * @param col    New column.
 * @param row    New row.
 *
 */
void chargrid_set_cursor(chargrid_t *scrbuf, sysarg_t col, sysarg_t row)
{
	if (col >= scrbuf->cols || row >= scrbuf->rows)
		return;

	scrbuf->col = col;
	scrbuf->row = row;
}

void chargrid_set_cursor_visibility(chargrid_t *scrbuf, bool visible)
{
	scrbuf->cursor_visible = visible;
}

/** Get current chargrid coordinates
 *
 * @param scrbuf Chargrid.
 * @param col    Column.
 * @param row    Row.
 *
 */
void chargrid_get_cursor(chargrid_t *scrbuf, sysarg_t *col,
    sysarg_t *row)
{
	assert(col);
	assert(row);

	*col = scrbuf->col;
	*row = scrbuf->row;
}

bool chargrid_get_cursor_visibility(chargrid_t *scrbuf)
{
	return scrbuf->cursor_visible;
}

/** Clear one buffer row.
 *
 * @param scrbuf Chargrid.
 * @param row    Row to clear.
 *
 */
void chargrid_clear_row(chargrid_t *scrbuf, sysarg_t row)
{
	for (sysarg_t col = 0; col < scrbuf->cols; col++) {
		charfield_t *field =
		    chargrid_charfield_at(scrbuf, col, row);

		field->ch = 0;
		field->attrs = scrbuf->attrs;
		field->flags |= CHAR_FLAG_DIRTY;
	}
}

/** Set chargrid style.
 *
 * @param scrbuf Chargrid.
 * @param style  Style.
 *
 */
void chargrid_set_style(chargrid_t *scrbuf, console_style_t style)
{
	scrbuf->attrs.type = CHAR_ATTR_STYLE;
	scrbuf->attrs.val.style = style;
}

/** Set chargrid color.
 *
 * @param scrbuf  Chargrid.
 * @param bgcolor Background color.
 * @param fgcolor Foreground color.
 * @param attr    Color attribute.
 *
 */
void chargrid_set_color(chargrid_t *scrbuf, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	scrbuf->attrs.type = CHAR_ATTR_INDEX;
	scrbuf->attrs.val.index.bgcolor = bgcolor;
	scrbuf->attrs.val.index.fgcolor = fgcolor;
	scrbuf->attrs.val.index.attr = attr;
}

/** Set chargrid RGB color.
 *
 * @param scrbuf  Chargrid.
 * @param bgcolor Background color.
 * @param fgcolor Foreground color.
 *
 */
void chargrid_set_rgb_color(chargrid_t *scrbuf, pixel_t bgcolor,
    pixel_t fgcolor)
{
	scrbuf->attrs.type = CHAR_ATTR_RGB;
	scrbuf->attrs.val.rgb.bgcolor = bgcolor;
	scrbuf->attrs.val.rgb.fgcolor = fgcolor;
}

/** @}
 */
