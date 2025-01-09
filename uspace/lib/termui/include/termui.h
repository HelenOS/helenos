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

#ifndef USPACE_LIB_TERMUI_TERMUI_H_
#define USPACE_LIB_TERMUI_TERMUI_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GLYPH_IDX_
#define GLYPH_IDX_ENDL 0xffffffu

struct termui;
typedef struct termui termui_t;

/* RGB555 color representation. See termui_color_from/to_rgb() */
typedef uint16_t termui_color_t;
#define TERMUI_COLOR_DEFAULT 0

typedef struct {
	unsigned italic : 1;
	unsigned bold : 1;
	unsigned underline : 1;
	unsigned blink : 1;
	unsigned strike : 1;
	unsigned inverted : 1;
	unsigned cursor : 1;
	/*
	 * Padding cells for wide characters.
	 * Placed at the end of rows where a wide character should have gone
	 * but didn't fit, and after wide characters to mark out the full space
	 * taken.
	 */
	unsigned padding : 1;
	// This is enough range for full Unicode coverage several times over.
	// The library is almost completely oblivious to the meaning of glyph index,
	// with the sole exception that zero is assumed to mean no glyph/empty cell.
	// User application can utilize the extended range to, for example:
	//  - support multiple fonts / fallback fonts
	//  - support select combining character sequences that don't
	//    have equivalent precomposed characters in Unicode
	//  - support additional graphical features that aren't included in
	//    this structure
	// Empty cells are initialized to all zeros.
	unsigned glyph_idx : 24;
	termui_color_t fgcolor;
	termui_color_t bgcolor;
} termui_cell_t;

/** Update callback for viewport contents. The updated region is always limited
 * to a single row. One row can be updated by multiple invocations.
 * @param userdata
 * @param col   First column of the updated region.
 * @param row   Viewport row of the updated region.
 * @param cell  Updated cell data array.
 * @param len   Length of the updated region.
 */
typedef void (*termui_update_cb_t)(void *userdata, int col, int row, const termui_cell_t *cell, int len);

/** Scrolling callback.
 * The entire viewport was shifted by the given number of rows.
 * For example, when a new line is added at the bottom of a full screen,
 * this is called with delta = +1.
 * The recipient must call termui_force_viewport_update() for previously
 * off-screen rows manually (allowing this callback to be implemented
 * the same as refresh).
 *
 * @param userdata
 * @param delta  Number of rows. Positive when viewport content moved up.
 */
typedef void (*termui_scroll_cb_t)(void *userdata, int delta);

/** Refresh callback. Instructs user to re-render the entire screen.
 *
 * @param userdata
 */
typedef void (*termui_refresh_cb_t)(void *userdata);

termui_t *termui_create(int cols, int rows, size_t history_lines);
void termui_destroy(termui_t *termui);

errno_t termui_resize(termui_t *termui, int cols, int rows, size_t history_lines);

void termui_set_scroll_cb(termui_t *termui, termui_scroll_cb_t cb, void *userdata);
void termui_set_update_cb(termui_t *termui, termui_update_cb_t cb, void *userdata);
void termui_set_refresh_cb(termui_t *termui, termui_refresh_cb_t cb, void *userdata);

void termui_put_lf(termui_t *termui);
void termui_put_cr(termui_t *termui);
void termui_put_crlf(termui_t *termui);
void termui_put_tab(termui_t *termui);
void termui_put_backspace(termui_t *termui);
void termui_put_glyph(termui_t *termui, uint32_t glyph, int width);
void termui_clear_screen(termui_t *termui);
void termui_wipe_screen(termui_t *termui, int first_row);

void termui_set_style(termui_t *termui, termui_cell_t style);
void termui_set_pos(termui_t *termui, int col, int row);
void termui_get_pos(const termui_t *termui, int *col, int *row);
int termui_get_cols(const termui_t *termui);
int termui_get_rows(const termui_t *termui);

bool termui_get_cursor_visibility(const termui_t *termui);
void termui_set_cursor_visibility(termui_t *termui, bool visible);
termui_cell_t *termui_get_active_row(termui_t *termui, int row);
void termui_history_scroll(termui_t *termui, int delta);
void termui_force_viewport_update(const termui_t *termui, int first_row, int rows);
bool termui_scrollback_is_active(const termui_t *termui);

termui_color_t termui_color_from_rgb(uint8_t r, uint8_t g, uint8_t b);
void termui_color_to_rgb(termui_color_t c, uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* USPACE_LIB_TERMUI_TERMUI_H_ */
