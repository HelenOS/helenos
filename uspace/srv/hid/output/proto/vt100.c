/*
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

/** @file
 */

#include <inttypes.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/color.h>
#include <types/common.h>
#include "vt100.h"

#define MAX_CONTROL  20

typedef enum {
	CI_BLACK   = 0,
	CI_RED     = 1,
	CI_GREEN   = 2,
	CI_BROWN   = 3,
	CI_BLUE    = 4,
	CI_MAGENTA = 5,
	CI_CYAN    = 6,
	CI_WHITE   = 7
} sgr_color_index_t;

typedef enum {
	SGR_RESET       = 0,
	SGR_BOLD        = 1,
	SGR_UNDERLINE   = 4,
	SGR_BLINK       = 5,
	SGR_REVERSE     = 7,
	SGR_FGCOLOR     = 30,
	SGR_BGCOLOR     = 40
} sgr_command_t;

static sgr_color_index_t color_map[] = {
	[COLOR_BLACK]   = CI_BLACK,
	[COLOR_BLUE]    = CI_RED,
	[COLOR_GREEN]   = CI_GREEN,
	[COLOR_CYAN]    = CI_CYAN,
	[COLOR_RED]     = CI_RED,
	[COLOR_MAGENTA] = CI_MAGENTA,
	[COLOR_YELLOW]  = CI_BROWN,
	[COLOR_WHITE]   = CI_WHITE
};

/** ECMA-48 Set Graphics Rendition. */
static void vt100_sgr(vt100_state_t *state, unsigned int mode)
{
	char control[MAX_CONTROL];
	
	snprintf(control, MAX_CONTROL, "\033[%um", mode);
	state->control_puts(control);
}

static void vt100_set_pos(vt100_state_t *state, sysarg_t col, sysarg_t row)
{
	char control[MAX_CONTROL];
	
	snprintf(control, MAX_CONTROL, "\033[%" PRIun ";%" PRIun "f",
	    row + 1, col + 1);
	state->control_puts(control);
}

static void vt100_set_sgr(vt100_state_t *state, char_attrs_t attrs)
{
	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			vt100_sgr(state, SGR_RESET);
			vt100_sgr(state, SGR_BGCOLOR + CI_WHITE);
			vt100_sgr(state, SGR_FGCOLOR + CI_BLACK);
			break;
		case STYLE_EMPHASIS:
			vt100_sgr(state, SGR_RESET);
			vt100_sgr(state, SGR_BGCOLOR + CI_WHITE);
			vt100_sgr(state, SGR_FGCOLOR + CI_RED);
			vt100_sgr(state, SGR_BOLD);
			break;
		case STYLE_INVERTED:
			vt100_sgr(state, SGR_RESET);
			vt100_sgr(state, SGR_BGCOLOR + CI_BLACK);
			vt100_sgr(state, SGR_FGCOLOR + CI_WHITE);
			break;
		case STYLE_SELECTED:
			vt100_sgr(state, SGR_RESET);
			vt100_sgr(state, SGR_BGCOLOR + CI_RED);
			vt100_sgr(state, SGR_FGCOLOR + CI_WHITE);
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		vt100_sgr(state, SGR_RESET);
		vt100_sgr(state, SGR_BGCOLOR + color_map[attrs.val.index.bgcolor & 7]);
		vt100_sgr(state, SGR_FGCOLOR + color_map[attrs.val.index.fgcolor & 7]);
		
		if (attrs.val.index.attr & CATTR_BRIGHT)
			vt100_sgr(state, SGR_BOLD);
		
		break;
	case CHAR_ATTR_RGB:
		vt100_sgr(state, SGR_RESET);
		
		if (attrs.val.rgb.bgcolor <= attrs.val.rgb.fgcolor)
			vt100_sgr(state, SGR_REVERSE);
		
		break;
	}
}

vt100_state_t *vt100_state_create(sysarg_t cols, sysarg_t rows,
    vt100_putchar_t putchar_fn, vt100_control_puts_t control_puts_fn,
	vt100_flush_t flush_fn)
{
	vt100_state_t *state = malloc(sizeof(vt100_state_t));
	if (state == NULL)
		return NULL;
	
	state->putchar = putchar_fn;
	state->control_puts = control_puts_fn;
	state->flush = flush_fn;
	
	state->cols = cols;
	state->rows = rows;
	
	state->cur_col = (sysarg_t) -1;
	state->cur_row = (sysarg_t) -1;
	
	state->cur_attrs.type = CHAR_ATTR_STYLE;
	state->cur_attrs.val.style = STYLE_NORMAL;
	
	/* Initialize graphic rendition attributes */
	vt100_sgr(state, SGR_RESET);
	vt100_sgr(state, SGR_FGCOLOR + CI_BLACK);
	vt100_sgr(state, SGR_BGCOLOR + CI_WHITE);
	state->control_puts("\033[2J");
	state->control_puts("\033[?25l");

	return state;
}

void vt100_state_destroy(vt100_state_t *state)
{
	free(state);
}

void vt100_get_dimensions(vt100_state_t *state, sysarg_t *cols,
    sysarg_t *rows)
{
	*cols = state->cols;
	*rows = state->rows;
}

errno_t vt100_yield(vt100_state_t *state)
{
	return EOK;
}

errno_t vt100_claim(vt100_state_t *state)
{
	return EOK;
}

void vt100_goto(vt100_state_t *state, sysarg_t col, sysarg_t row)
{
	if ((col >= state->cols) || (row >= state->rows))
		return;
	
	if ((col != state->cur_col) || (row != state->cur_row)) {
		vt100_set_pos(state, col, row);
		state->cur_col = col;
		state->cur_row = row;
	}
}

void vt100_set_attr(vt100_state_t *state, char_attrs_t attrs)
{
	if (!attrs_same(state->cur_attrs, attrs)) {
		vt100_set_sgr(state, attrs);
		state->cur_attrs = attrs;
	}
}

void vt100_cursor_visibility(vt100_state_t *state, bool visible)
{
	if (visible)
		state->control_puts("\033[?25h");
	else
		state->control_puts("\033[?25l");
}

void vt100_putchar(vt100_state_t *state, wchar_t ch)
{
	state->putchar(ch == 0 ? ' ' : ch);
	state->cur_col++;
	
	if (state->cur_col >= state->cols) {
		state->cur_row += state->cur_col / state->cols;
		state->cur_col %= state->cols;
	}
}

void vt100_flush(vt100_state_t *state)
{
	state->flush();
}

/** @}
 */
