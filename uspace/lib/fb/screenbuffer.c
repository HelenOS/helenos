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

/** @addtogroup console
 * @{
 */
/** @file
 */

#include <io/style.h>
#include <malloc.h>
#include <unistd.h>
#include <assert.h>
#include <bool.h>
#include <as.h>
#include "screenbuffer.h"

/** Structure for buffering state of one virtual console.
 *
 */
struct screenbuffer {
	size_t size;                /**< Structure size */
	screenbuffer_flag_t flags;  /**< Screenbuffer flags */
	
	sysarg_t cols;              /**< Number of columns */
	sysarg_t rows;              /**< Number of rows */
	
	sysarg_t col;               /**< Current column */
	sysarg_t row;               /**< Current row */
	bool cursor_visible;        /**< Cursor visibility */
	
	char_attrs_t attrs;         /**< Current attributes */
	
	sysarg_t top_row;           /**< The first row in the cyclic buffer */
	charfield_t data[];         /**< Screen contents (cyclic buffer) */
};

/** Create a screenbuffer.
 *
 * @param[in] cols  Number of columns.
 * @param[in] rows  Number of rows.
 * @param[in] flags Screenbuffer flags.
 *
 * @return New screenbuffer.
 * @return NULL on failure.
 *
 */
screenbuffer_t *screenbuffer_create(sysarg_t cols, sysarg_t rows,
    screenbuffer_flag_t flags)
{
	size_t size =
	    sizeof(screenbuffer_t) + cols * rows * sizeof(charfield_t);
	screenbuffer_t *scrbuf;
	
	if ((flags & SCREENBUFFER_FLAG_SHARED) == SCREENBUFFER_FLAG_SHARED) {
		scrbuf = (screenbuffer_t *) as_area_create(AS_AREA_ANY, size,
		    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE);
		if (scrbuf == AS_MAP_FAILED)
			return NULL;
	} else {
		scrbuf = (screenbuffer_t *) malloc(size);
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
	screenbuffer_clear(scrbuf);
	
	return scrbuf;
}

/** Return keyfield by coordinates
 *
 * The back buffer is organized as a cyclic buffer.
 * Therefore we must take into account the topmost column.
 *
 * @param scrbuf Screenbuffer
 * @param col    Column position on screen
 * @param row    Row position on screen
 *
 * @return Keyfield structure on (row, col)
 *
 */
charfield_t *screenbuffer_field_at(screenbuffer_t *scrbuf, sysarg_t col,
    sysarg_t row)
{
	return scrbuf->data +
	    ((row + scrbuf->top_row) % scrbuf->rows) * scrbuf->cols +
	    col;
}

bool screenbuffer_cursor_at(screenbuffer_t *scrbuf, sysarg_t col, sysarg_t row)
{
	return ((scrbuf->cursor_visible) && (scrbuf->col == col) &&
	    (scrbuf->row == row));
}

sysarg_t screenbuffer_get_top_row(screenbuffer_t *scrbuf)
{
	return scrbuf->top_row;
}

static sysarg_t screenbuffer_update_rows(screenbuffer_t *scrbuf)
{
	if (scrbuf->row == scrbuf->rows) {
		scrbuf->row = scrbuf->rows - 1;
		scrbuf->top_row = (scrbuf->top_row + 1) % scrbuf->rows;
		screenbuffer_clear_row(scrbuf, scrbuf->row);
		
		return scrbuf->rows;
	}
	
	return 2;
}

static sysarg_t screenbuffer_update_cols(screenbuffer_t *scrbuf)
{
	/* Column overflow */
	if (scrbuf->col == scrbuf->cols) {
		scrbuf->col = 0;
		scrbuf->row++;
		return screenbuffer_update_rows(scrbuf);
	}
	
	return 1;
}

/** Store one character to screenbuffer.
 *
 * Its position is determined by scrbuf->col
 * and scrbuf->row.
 *
 * @param scrbuf Screenbuffer.
 * @param ch     Character to store.
 * @param update Update coordinates.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to a new row, this value is 2.
 *
 */
sysarg_t screenbuffer_putchar(screenbuffer_t *scrbuf, wchar_t ch, bool update)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);
	
	charfield_t *field =
	    screenbuffer_field_at(scrbuf, scrbuf->col, scrbuf->row);
	
	field->ch = ch;
	field->attrs = scrbuf->attrs;
	field->flags |= CHAR_FLAG_DIRTY;
	
	if (update) {
		scrbuf->col++;
		return screenbuffer_update_cols(scrbuf);
	}
	
	return 1;
}

/** Jump to a new row in screenbuffer.
 *
 * @param scrbuf Screenbuffer.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 2 (the original row and the new
 *         row).
 *
 */
sysarg_t screenbuffer_newline(screenbuffer_t *scrbuf)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);
	
	scrbuf->col = 0;
	scrbuf->row++;
	
	return screenbuffer_update_rows(scrbuf);
}

/** Jump to a new row in screenbuffer.
 *
 * @param scrbuf   Screenbuffer.
 * @param tab_size Tab size.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to a new row, this value is 2.
 *
 */
sysarg_t screenbuffer_tabstop(screenbuffer_t *scrbuf, sysarg_t tab_size)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);
	
	sysarg_t spaces = tab_size - scrbuf->cols % tab_size;
	sysarg_t flush = 1;
	
	for (sysarg_t i = 0; i < spaces; i++)
		flush += screenbuffer_putchar(scrbuf, ' ', true) - 1;
	
	return flush;
}

/** Jump to the previous character in screenbuffer.
 *
 * Currently no scrollback is supported.
 *
 * @param scrbuf Screenbuffer.
 *
 * @return Number of rows which have been affected. In usual
 *         situations this is 1. If the current position was
 *         updated to the previous row, this value is 2.
 * @return 0 if no backspace is possible.
 *
 */
sysarg_t screenbuffer_backspace(screenbuffer_t *scrbuf)
{
	assert(scrbuf->col < scrbuf->cols);
	assert(scrbuf->row < scrbuf->rows);
	
	if ((scrbuf->col == 0) && (scrbuf->row == 0))
		return 0;
	
	if (scrbuf->col == 0) {
		scrbuf->col = scrbuf->cols - 1;
		scrbuf->row--;
		
		screenbuffer_putchar(scrbuf, ' ', false);
		return 2;
	}
	
	scrbuf->col--;
	screenbuffer_putchar(scrbuf, ' ', false);
	return 1;
}

/** Clear the screenbuffer.
 *
 * @param scrbuf Screenbuffer.
 *
 */
void screenbuffer_clear(screenbuffer_t *scrbuf)
{
	for (size_t pos = 0; pos < (scrbuf->cols * scrbuf->rows); pos++) {
		scrbuf->data[pos].ch = 0;
		scrbuf->data[pos].attrs = scrbuf->attrs;
		scrbuf->data[pos].flags = CHAR_FLAG_DIRTY;
	}
	
	scrbuf->col = 0;
	scrbuf->row = 0;
}

/** Update current screenbuffer coordinates
 *
 * @param scrbuf Screenbuffer.
 * @param col    New column.
 * @param row    New row.
 *
 */
void screenbuffer_set_cursor(screenbuffer_t *scrbuf, sysarg_t col, sysarg_t row)
{
	scrbuf->col = col;
	scrbuf->row = row;
}

void screenbuffer_set_cursor_visibility(screenbuffer_t *scrbuf, bool visible)
{
	scrbuf->cursor_visible = visible;
}

/** Get current screenbuffer coordinates
 *
 * @param scrbuf Screenbuffer.
 * @param col    Column.
 * @param row    Row.
 *
 */
void screenbuffer_get_cursor(screenbuffer_t *scrbuf, sysarg_t *col,
    sysarg_t *row)
{
	assert(col);
	assert(row);
	
	*col = scrbuf->col;
	*row = scrbuf->row;
}

bool screenbuffer_get_cursor_visibility(screenbuffer_t *scrbuf)
{
	return scrbuf->cursor_visible;
}

/** Clear one buffer row.
 *
 * @param scrbuf Screenbuffer.
 * @param row    Row to clear.
 *
 */
void screenbuffer_clear_row(screenbuffer_t *scrbuf, sysarg_t row)
{
	for (sysarg_t col = 0; col < scrbuf->cols; col++) {
		charfield_t *field =
		    screenbuffer_field_at(scrbuf, col, row);
		
		field->ch = 0;
		field->attrs = scrbuf->attrs;
		field->flags |= CHAR_FLAG_DIRTY;
	}
}

/** Set screenbuffer style.
 *
 * @param scrbuf Screenbuffer.
 * @param style  Style.
 *
 */
void screenbuffer_set_style(screenbuffer_t *scrbuf, console_style_t style)
{
	scrbuf->attrs.type = CHAR_ATTR_STYLE;
	scrbuf->attrs.val.style = style;
}

/** Set screenbuffer color.
 *
 * @param scrbuf  Screenbuffer.
 * @param bgcolor Background color.
 * @param fgcolor Foreground color.
 * @param attr    Color attribute.
 *
 */
void screenbuffer_set_color(screenbuffer_t *scrbuf, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t attr)
{
	scrbuf->attrs.type = CHAR_ATTR_INDEX;
	scrbuf->attrs.val.index.bgcolor = bgcolor;
	scrbuf->attrs.val.index.fgcolor = fgcolor;
	scrbuf->attrs.val.index.attr = attr;
}

/** Set screenbuffer RGB color.
 *
 * @param scrbuf  Screenbuffer.
 * @param bgcolor Background color.
 * @param fgcolor Foreground color.
 *
 */
void screenbuffer_set_rgb_color(screenbuffer_t *scrbuf, pixel_t bgcolor,
    pixel_t fgcolor)
{
	scrbuf->attrs.type = CHAR_ATTR_RGB;
	scrbuf->attrs.val.rgb.bgcolor = bgcolor;
	scrbuf->attrs.val.rgb.fgcolor = fgcolor;
}

/** @}
 */
