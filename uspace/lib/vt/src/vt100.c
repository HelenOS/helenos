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

/** @addtogroup libvt
 * @{
 */

#include <ctype.h>
#include <errno.h>
#include <io/color.h>
#include <io/keycode.h>
#include <stdio.h>
#include <stdlib.h>
#include <vt/vt100.h>

/** Map console colors to VT100 color indices */
sgr_color_index_t color_map[] = {
	[COLOR_BLACK]   = CI_BLACK,
	[COLOR_BLUE]    = CI_BLUE,
	[COLOR_GREEN]   = CI_GREEN,
	[COLOR_CYAN]    = CI_CYAN,
	[COLOR_RED]     = CI_RED,
	[COLOR_MAGENTA] = CI_MAGENTA,
	[COLOR_YELLOW]  = CI_BROWN,
	[COLOR_WHITE]   = CI_WHITE
};

/** Clear screen.
 *
 * @param vt VT instance
 */
void vt100_cls(vt100_t *vt)
{
	vt->cb->control_puts(vt->arg, "\033[2J");
}

/** ECMA-48 Set Graphics Rendition.
 *
 * @param vt VT instance
 * @param mode SGR mode number
 */
void vt100_sgr(vt100_t *vt, unsigned int mode)
{
	char control[MAX_CONTROL];

	snprintf(control, MAX_CONTROL, "\033[%um", mode);
	vt->cb->control_puts(vt->arg, control);
}

/** Set Graphics Rendition with 5 arguments.
 *
 * @param vt VT instance
 * @param a1 First argument
 * @param a2 Second argument
 * @param a3 Third argument
 * @param a4 Fourth argument
 * @param a5 Fifth argument
 */
static void vt100_sgr5(vt100_t *vt, unsigned a1, unsigned a2,
    unsigned a3, unsigned a4, unsigned a5)
{
	char control[MAX_CONTROL];

	snprintf(control, MAX_CONTROL, "\033[%u;%u;%u;%u;%um",
	    a1, a2, a3, a4, a5);
	vt->cb->control_puts(vt->arg, control);
}

/** Set cussor position.
 *
 * @param vt VT instance
 * @param col Column (starting from 0)
 * @param row Row (starting from 0)
 */
void vt100_set_pos(vt100_t *vt, sysarg_t col, sysarg_t row)
{
	char control[MAX_CONTROL];

	snprintf(control, MAX_CONTROL, "\033[%" PRIun ";%" PRIun "f",
	    row + 1, col + 1);
	vt->cb->control_puts(vt->arg, control);
}

/** Set graphics rendition based on attributes,
 *
 * @param vt VT instance
 * @param attrs Character attributes
 */
void vt100_set_sgr(vt100_t *vt, char_attrs_t attrs)
{
	unsigned color;

	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			vt100_sgr(vt, SGR_RESET);
			vt100_sgr(vt, SGR_BGCOLOR + CI_WHITE);
			vt100_sgr(vt, SGR_FGCOLOR + CI_BLACK);
			break;
		case STYLE_EMPHASIS:
			vt100_sgr(vt, SGR_RESET);
			vt100_sgr(vt, SGR_BGCOLOR + CI_WHITE);
			vt100_sgr(vt, SGR_FGCOLOR + CI_RED);
			vt100_sgr(vt, SGR_BOLD);
			break;
		case STYLE_INVERTED:
			vt100_sgr(vt, SGR_RESET);
			vt100_sgr(vt, SGR_BGCOLOR + CI_BLACK);
			vt100_sgr(vt, SGR_FGCOLOR + CI_WHITE);
			break;
		case STYLE_SELECTED:
			vt100_sgr(vt, SGR_RESET);
			vt100_sgr(vt, SGR_BGCOLOR + CI_RED);
			vt100_sgr(vt, SGR_FGCOLOR + CI_WHITE);
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		vt100_sgr(vt, SGR_RESET);
		vt100_sgr(vt, SGR_BGCOLOR + color_map[attrs.val.index.bgcolor & 7]);
		vt100_sgr(vt, SGR_FGCOLOR + color_map[attrs.val.index.fgcolor & 7]);

		if (attrs.val.index.attr & CATTR_BRIGHT)
			vt100_sgr(vt, SGR_BOLD);

		break;
	case CHAR_ATTR_RGB:
		if (vt->enable_rgb == true) {
			vt100_sgr5(vt, 48, 2, RED(attrs.val.rgb.bgcolor),
			    GREEN(attrs.val.rgb.bgcolor),
			    BLUE(attrs.val.rgb.bgcolor));
			vt100_sgr5(vt, 38, 2, RED(attrs.val.rgb.fgcolor),
			    GREEN(attrs.val.rgb.fgcolor),
			    BLUE(attrs.val.rgb.fgcolor));
		} else {
			vt100_sgr(vt, SGR_RESET);
			color =
			    ((RED(attrs.val.rgb.fgcolor) >= 0x80) ? COLOR_RED : 0) |
			    ((GREEN(attrs.val.rgb.fgcolor) >= 0x80) ? COLOR_GREEN : 0) |
			    ((BLUE(attrs.val.rgb.fgcolor) >= 0x80) ? COLOR_BLUE : 0);
			vt100_sgr(vt, SGR_FGCOLOR + color_map[color]);
			color =
			    ((RED(attrs.val.rgb.bgcolor) >= 0x80) ? COLOR_RED : 0) |
			    ((GREEN(attrs.val.rgb.bgcolor) >= 0x80) ? COLOR_GREEN : 0) |
			    ((BLUE(attrs.val.rgb.bgcolor) >= 0x80) ? COLOR_BLUE : 0);
			vt100_sgr(vt, SGR_BGCOLOR + color_map[color]);
		}
		break;
	}
}

/** Create VT instance.
 *
 * @param arg Argument passed to callback functions
 * @param cols Number of columns
 * @param rows Number of rows
 * @param cb Callback functions
 *
 * @return Pointer to new VT instance on success, NULL on failure.
 */
vt100_t *vt100_create(void *arg, sysarg_t cols, sysarg_t rows, vt100_cb_t *cb)
{
	vt100_t *vt = malloc(sizeof(vt100_t));
	if (vt == NULL)
		return NULL;

	vt->cb = cb;
	vt->arg = arg;

	vt->cols = cols;
	vt->rows = rows;

	vt->cur_col = (sysarg_t) -1;
	vt->cur_row = (sysarg_t) -1;

	vt->cur_attrs.type = -1;
	vt->cur_attrs.val.style = -1;

	vt->state = vts_base;
	vt->inncnt = 0;

	return vt;
}

/** Resize VT instance.
 *
 * @param vt VT instance
 * @param cols New number of columns
 * @param rows New number of rows
 */
void vt100_resize(vt100_t *vt, sysarg_t cols, sysarg_t rows)
{
	vt->cols = cols;
	vt->rows = rows;

	if (vt->cur_col > cols - 1)
		vt->cur_col = cols - 1;
	if (vt->cur_row > rows - 1)
		vt->cur_row = rows - 1;
}

/** Destroy VT instance.
 *
 * @param vt VT instance
 */
void vt100_destroy(vt100_t *vt)
{
	free(vt);
}

/** Get VT size.
 *
 * @param vt VT instance
 * @param cols Place to store number of columns
 * @param rows Place to store number of rows
 */
void vt100_get_dimensions(vt100_t *vt, sysarg_t *cols,
    sysarg_t *rows)
{
	*cols = vt->cols;
	*rows = vt->rows;
}

/** Temporarily yield VT to other users.
 *
 * @param vt VT instance
 * @return EOK on success or an error code
 */
errno_t vt100_yield(vt100_t *vt)
{
	return EOK;
}

/** Reclaim VT.
 *
 * @param vt VT instance
 * @return EOK on success or an error code
 */
errno_t vt100_claim(vt100_t *vt)
{
	return EOK;
}

/** Go to specified position, if needed.
 *
 * @param vt VT instance
 * @param col Column (starting from 0)
 * @param row Row (starting from 0)
 */
void vt100_goto(vt100_t *vt, sysarg_t col, sysarg_t row)
{
	if ((col >= vt->cols) || (row >= vt->rows))
		return;

	if ((col != vt->cur_col) || (row != vt->cur_row)) {
		vt100_set_pos(vt, col, row);
		vt->cur_col = col;
		vt->cur_row = row;
	}
}

/** Set character attributes, if needed.
 *
 * @param vt VT instance
 * @param attrs Attributes
 */
void vt100_set_attr(vt100_t *vt, char_attrs_t attrs)
{
	if (!attrs_same(vt->cur_attrs, attrs)) {
		vt100_set_sgr(vt, attrs);
		vt->cur_attrs = attrs;
	}
}

/** Set cursor visibility.
 *
 * @param vt VT instance
 * @param @c true to make cursor visible, @c false to make it invisible
 */
void vt100_cursor_visibility(vt100_t *vt, bool visible)
{
	if (visible)
		vt->cb->control_puts(vt->arg, "\033[?25h");
	else
		vt->cb->control_puts(vt->arg, "\033[?25l");
}

/** Set mouse button press/release reporting.
 *
 * @param vt VT instance
 * @param @c true to enable button press/release reporting
 */
void vt100_set_button_reporting(vt100_t *vt, bool enable)
{
	if (enable) {
		/* Enable button tracking */
		vt->cb->control_puts(vt->arg, "\033[?1000h");
		/* Enable SGR encoding of mouse reports */
		vt->cb->control_puts(vt->arg, "\033[?1006h");
	} else {
		/* Disable button tracking */
		vt->cb->control_puts(vt->arg, "\033[?1000l");
		/* Disable SGR encoding of mouse reports */
		vt->cb->control_puts(vt->arg, "\033[?1006l");
	}
}

/** Set terminal title.
 *
 * @param vt VT instance
 * @param title Terminal title
 */
void vt100_set_title(vt100_t *vt, const char *title)
{
	vt->cb->control_puts(vt->arg, "\033]0;");
	vt->cb->control_puts(vt->arg, title);
	vt->cb->control_puts(vt->arg, "\a");
}

/** Print Unicode character.
 *
 * @param vt VT instance
 * @parma ch Unicode character
 */
void vt100_putuchar(vt100_t *vt, char32_t ch)
{
	vt->cb->putuchar(vt->arg, ch == 0 ? ' ' : ch);
	vt->cur_col++;

	if (vt->cur_col >= vt->cols) {
		vt->cur_row += vt->cur_col / vt->cols;
		vt->cur_col %= vt->cols;
	}
}

/** Flush VT.
 *
 * @param vt VT instance
 */
void vt100_flush(vt100_t *vt)
{
	vt->cb->flush(vt->arg);
}

/** Recognize a key.
 *
 * Generate a key callback and reset decoder state.
 *
 * @param vt VT instance
 * @param mods Key modifiers
 * @param key Key code
 * @param c Character
 */
static void vt100_key(vt100_t *vt, keymod_t mods, keycode_t key, char c)
{
	vt->cb->key(vt->arg, mods, key, c);
	vt->state = vts_base;
}

/** Generate position event callback.
 * *
 * @param vt VT instance
 * @param ev Position event
 */
static void vt100_pos_event(vt100_t *vt, pos_event_t *ev)
{
	vt->cb->pos_event(vt->arg, ev);
}

/** Clear number decoder state.
 *
 * @param vt VT instance
 */
static void vt100_clear_innum(vt100_t *vt)
{
	unsigned i;

	vt->inncnt = 0;
	for (i = 0; i < INNUM_MAX; i++)
		vt->innum[i] = 0;
}

/** Process input character with prefix 1b.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b(vt100_t *vt, char c)
{
	switch (c) {
	case 0x1b:
		vt100_key(vt, 0, KC_ESCAPE, c);
		break;
	case 0x60:
		vt100_key(vt, KM_ALT, KC_BACKTICK, c);
		break;

	case 0x31:
		vt100_key(vt, KM_ALT, KC_1, c);
		break;
	case 0x32:
		vt100_key(vt, KM_ALT, KC_2, c);
		break;
	case 0x33:
		vt100_key(vt, KM_ALT, KC_3, c);
		break;
	case 0x34:
		vt100_key(vt, KM_ALT, KC_4, c);
		break;
	case 0x35:
		vt100_key(vt, KM_ALT, KC_5, c);
		break;
	case 0x36:
		vt100_key(vt, KM_ALT, KC_6, c);
		break;
	case 0x37:
		vt100_key(vt, KM_ALT, KC_7, c);
		break;
	case 0x38:
		vt100_key(vt, KM_ALT, KC_8, c);
		break;
	case 0x39:
		vt100_key(vt, KM_ALT, KC_9, c);
		break;
	case 0x30:
		vt100_key(vt, KM_ALT, KC_0, c);
		break;

	case 0x2d:
		vt100_key(vt, KM_ALT, KC_MINUS, c);
		break;
	case 0x3d:
		vt100_key(vt, KM_ALT, KC_EQUALS, c);
		break;

	case 0x71:
		vt100_key(vt, KM_ALT, KC_Q, c);
		break;
	case 0x77:
		vt100_key(vt, KM_ALT, KC_W, c);
		break;
	case 0x65:
		vt100_key(vt, KM_ALT, KC_E, c);
		break;
	case 0x72:
		vt100_key(vt, KM_ALT, KC_R, c);
		break;
	case 0x74:
		vt100_key(vt, KM_ALT, KC_T, c);
		break;
	case 0x79:
		vt100_key(vt, KM_ALT, KC_Y, c);
		break;
	case 0x75:
		vt100_key(vt, KM_ALT, KC_U, c);
		break;
	case 0x69:
		vt100_key(vt, KM_ALT, KC_I, c);
		break;
	case 0x6f:
		vt100_key(vt, KM_ALT, KC_O, c);
		break;
	case 0x70:
		vt100_key(vt, KM_ALT, KC_P, c);
		break;

		/* 0x1b, 0x5b is used by other keys/sequences */

	case 0x5d:
		vt100_key(vt, KM_ALT, KC_RBRACKET, c);
		break;

	case 0x61:
		vt100_key(vt, KM_ALT, KC_A, c);
		break;
	case 0x73:
		vt100_key(vt, KM_ALT, KC_S, c);
		break;
	case 0x64:
		vt100_key(vt, KM_ALT, KC_D, c);
		break;
	case 0x66:
		vt100_key(vt, KM_ALT, KC_F, c);
		break;
	case 0x67:
		vt100_key(vt, KM_ALT, KC_G, c);
		break;
	case 0x68:
		vt100_key(vt, KM_ALT, KC_H, c);
		break;
	case 0x6a:
		vt100_key(vt, KM_ALT, KC_J, c);
		break;
	case 0x6b:
		vt100_key(vt, KM_ALT, KC_K, c);
		break;
	case 0x6c:
		vt100_key(vt, KM_ALT, KC_L, c);
		break;

	case 0x3b:
		vt100_key(vt, KM_ALT, KC_SEMICOLON, c);
		break;
	case 0x27:
		vt100_key(vt, KM_ALT, KC_QUOTE, c);
		break;
	case 0x5c:
		vt100_key(vt, KM_ALT, KC_BACKSLASH, c);
		break;

	case 0x7a:
		vt100_key(vt, KM_ALT, KC_Z, c);
		break;
	case 0x78:
		vt100_key(vt, KM_ALT, KC_X, c);
		break;
	case 0x63:
		vt100_key(vt, KM_ALT, KC_C, c);
		break;
	case 0x76:
		vt100_key(vt, KM_ALT, KC_V, c);
		break;
	case 0x62:
		vt100_key(vt, KM_ALT, KC_B, c);
		break;
	case 0x6e:
		vt100_key(vt, KM_ALT, KC_N, c);
		break;
	case 0x6d:
		vt100_key(vt, KM_ALT, KC_M, c);
		break;

	case 0x2c:
		vt100_key(vt, KM_ALT, KC_COMMA, c);
		break;
	case 0x2e:
		vt100_key(vt, KM_ALT, KC_PERIOD, c);
		break;
	case 0x2f:
		vt100_key(vt, KM_ALT, KC_SLASH, c);
		break;

	case 0x4f:
		vt->state = vts_1b4f;
		break;
	case 0x5b:
		vt->state = vts_1b5b;
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 4f.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b4f(vt100_t *vt, char c)
{
	switch (c) {
	case 0x50:
		vt100_key(vt, 0, KC_F1, 0);
		break;
	case 0x51:
		vt100_key(vt, 0, KC_F2, 0);
		break;
	case 0x52:
		vt100_key(vt, 0, KC_F3, 0);
		break;
	case 0x53:
		vt100_key(vt, 0, KC_F4, 0);
		break;
	case 0x48:
		vt100_key(vt, 0, KC_HOME, 0);
		break;
	case 0x46:
		vt100_key(vt, 0, KC_END, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b(vt100_t *vt, char c)
{
	switch (c) {
	case 0x31:
		vt->state = vts_1b5b31;
		break;
	case 0x32:
		vt->state = vts_1b5b32;
		break;
	case 0x35:
		vt->state = vts_1b5b35;
		break;
	case 0x33:
		vt->state = vts_1b5b33;
		break;
	case 0x36:
		vt->state = vts_1b5b36;
		break;
	case 0x3c:
		vt->state = vts_1b5b3c;
		break;
	case 0x41:
		vt100_key(vt, 0, KC_UP, 0);
		break;
	case 0x44:
		vt100_key(vt, 0, KC_LEFT, 0);
		break;
	case 0x42:
		vt100_key(vt, 0, KC_DOWN, 0);
		break;
	case 0x43:
		vt100_key(vt, 0, KC_RIGHT, 0);
		break;
	case 0x48:
		vt100_key(vt, 0, KC_HOME, 0);
		break;
	case 0x46:
		vt100_key(vt, 0, KC_END, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b31(vt100_t *vt, char c)
{
	switch (c) {
	case 0x35:
		vt->state = vts_1b5b3135;
		break;
	case 0x37:
		vt->state = vts_1b5b3137;
		break;
	case 0x38:
		vt->state = vts_1b5b3138;
		break;
	case 0x39:
		vt->state = vts_1b5b3139;
		break;
	case 0x3b:
		vt->state = vts_1b5b313b;
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 35.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3135(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F5, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 37.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3137(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F6, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 38.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3138(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F7, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 39.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3139(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F8, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 3b.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b313b(vt100_t *vt, char c)
{
	switch (c) {
	case 0x32:
		vt->state = vts_1b5b313b32;
		break;
	case 0x33:
		vt->state = vts_1b5b313b33;
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 3b 32.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b313b32(vt100_t *vt, char c)
{
	switch (c) {
	case 0x41:
		vt100_key(vt, KM_SHIFT, KC_UP, 0);
		break;
	case 0x44:
		vt100_key(vt, KM_SHIFT, KC_LEFT, 0);
		break;
	case 0x42:
		vt100_key(vt, KM_SHIFT, KC_DOWN, 0);
		break;
	case 0x43:
		vt100_key(vt, KM_SHIFT, KC_RIGHT, 0);
		break;
	case 0x48:
		vt100_key(vt, KM_SHIFT, KC_HOME, 0);
		break;
	case 0x46:
		vt100_key(vt, KM_SHIFT, KC_END, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 31 3b 33.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b313b33(vt100_t *vt, char c)
{
	switch (c) {
	case 0x41:
		vt100_key(vt, KM_ALT, KC_UP, 0);
		break;
	case 0x44:
		vt100_key(vt, KM_ALT, KC_LEFT, 0);
		break;
	case 0x42:
		vt100_key(vt, KM_ALT, KC_DOWN, 0);
		break;
	case 0x43:
		vt100_key(vt, KM_ALT, KC_RIGHT, 0);
		break;
	case 0x48:
		vt100_key(vt, KM_ALT, KC_HOME, 0);
		break;
	case 0x46:
		vt100_key(vt, KM_ALT, KC_END, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b32(vt100_t *vt, char c)
{
	switch (c) {
	case 0x30:
		vt->state = vts_1b5b3230;
		break;
	case 0x31:
		vt->state = vts_1b5b3231;
		break;
	case 0x33:
		vt->state = vts_1b5b3233;
		break;
	case 0x34:
		vt->state = vts_1b5b3234;
		break;
	case 0x35:
		vt->state = vts_1b5b3235;
		break;
	case 0x38:
		vt->state = vts_1b5b3238;
		break;
	case 0x7e:
		vt100_key(vt, 0, KC_INSERT, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 30.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3230(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F9, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 31.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3231(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F10, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 33.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3233(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F11, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 34.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3234(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_F12, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 35.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3235(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_PRTSCR, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 32 38.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3238(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_PAUSE, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 35.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b35(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_PAGE_UP, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 33.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b33(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_DELETE, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 36.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b36(vt100_t *vt, char c)
{
	switch (c) {
	case 0x7e:
		vt100_key(vt, 0, KC_PAGE_DOWN, 0);
		break;
	default:
		vt->state = vts_base;
		break;
	}
}

/** Process input character with prefix 1b 5b 3c - mouse report.
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_1b5b3c(vt100_t *vt, char c)
{
	pos_event_t ev;

	if (isdigit(c)) {
		/* Decode next base-10 digit */
		vt->innum[vt->inncnt] = vt->innum[vt->inncnt] * 10 + (c - '0');
	} else if (c == ';') {
		/* Move to next parameter */
		if (vt->inncnt >= INNUM_MAX - 1) {
			vt->state = vts_base;
			vt100_clear_innum(vt);
			return;
		}
		++vt->inncnt;
	} else {
		switch (c) {
		case 'M':
		case 'm':
			/* Button press / release */
			ev.pos_id = 0;
			ev.type = (c == 'M') ? POS_PRESS : POS_RELEASE;

			/*
			 * VT reports button 0 = left button,
			 * 1 = middle, 2 = right.
			 * pos_event needs 1 = left, 2 = right, 3 = middle,...
			 * so right and middle need to be reversed.
			 */
			switch (vt->innum[0]) {
			case 1:
				ev.btn_num = 3;
				break;
			case 2:
				ev.btn_num = 2;
				break;
			default:
				ev.btn_num = 1 + vt->innum[0];
				break;
			}
			ev.hpos = vt->innum[1] - 1;
			ev.vpos = vt->innum[2] - 1;
			vt100_pos_event(vt, &ev);
			break;
		}
		vt100_clear_innum(vt);
		vt->state = vts_base;
	}
}

/** Process input character (base state, no prefix).
 *
 * @param vt VT instance
 * @param c Input character
 */
static void vt100_rcvd_base(vt100_t *vt, char c)
{
	switch (c) {
		/*
		 * Not shifted
		 */
	case 0x60:
		vt100_key(vt, 0, KC_BACKTICK, c);
		break;

	case 0x31:
		vt100_key(vt, 0, KC_1, c);
		break;
	case 0x32:
		vt100_key(vt, 0, KC_2, c);
		break;
	case 0x33:
		vt100_key(vt, 0, KC_3, c);
		break;
	case 0x34:
		vt100_key(vt, 0, KC_4, c);
		break;
	case 0x35:
		vt100_key(vt, 0, KC_5, c);
		break;
	case 0x36:
		vt100_key(vt, 0, KC_6, c);
		break;
	case 0x37:
		vt100_key(vt, 0, KC_7, c);
		break;
	case 0x38:
		vt100_key(vt, 0, KC_8, c);
		break;
	case 0x39:
		vt100_key(vt, 0, KC_9, c);
		break;
	case 0x30:
		vt100_key(vt, 0, KC_0, c);
		break;
	case 0x2d:
		vt100_key(vt, 0, KC_MINUS, c);
		break;
	case 0x3d:
		vt100_key(vt, 0, KC_EQUALS, c);
		break;

	case 0x08:
		vt100_key(vt, 0, KC_BACKSPACE, c);
		break;

	case 0x09:
		vt100_key(vt, 0, KC_TAB, c);
		break;

	case 0x71:
		vt100_key(vt, 0, KC_Q, c);
		break;
	case 0x77:
		vt100_key(vt, 0, KC_W, c);
		break;
	case 0x65:
		vt100_key(vt, 0, KC_E, c);
		break;
	case 0x72:
		vt100_key(vt, 0, KC_R, c);
		break;
	case 0x74:
		vt100_key(vt, 0, KC_T, c);
		break;
	case 0x79:
		vt100_key(vt, 0, KC_Y, c);
		break;
	case 0x75:
		vt100_key(vt, 0, KC_U, c);
		break;
	case 0x69:
		vt100_key(vt, 0, KC_I, c);
		break;
	case 0x6f:
		vt100_key(vt, 0, KC_O, c);
		break;
	case 0x70:
		vt100_key(vt, 0, KC_P, c);
		break;

	case 0x5b:
		vt100_key(vt, 0, KC_LBRACKET, c);
		break;
	case 0x5d:
		vt100_key(vt, 0, KC_RBRACKET, c);
		break;

	case 0x61:
		vt100_key(vt, 0, KC_A, c);
		break;
	case 0x73:
		vt100_key(vt, 0, KC_S, c);
		break;
	case 0x64:
		vt100_key(vt, 0, KC_D, c);
		break;
	case 0x66:
		vt100_key(vt, 0, KC_F, c);
		break;
	case 0x67:
		vt100_key(vt, 0, KC_G, c);
		break;
	case 0x68:
		vt100_key(vt, 0, KC_H, c);
		break;
	case 0x6a:
		vt100_key(vt, 0, KC_J, c);
		break;
	case 0x6b:
		vt100_key(vt, 0, KC_K, c);
		break;
	case 0x6c:
		vt100_key(vt, 0, KC_L, c);
		break;

	case 0x3b:
		vt100_key(vt, 0, KC_SEMICOLON, c);
		break;
	case 0x27:
		vt100_key(vt, 0, KC_QUOTE, c);
		break;
	case 0x5c:
		vt100_key(vt, 0, KC_BACKSLASH, c);
		break;

	case 0x7a:
		vt100_key(vt, 0, KC_Z, c);
		break;
	case 0x78:
		vt100_key(vt, 0, KC_X, c);
		break;
	case 0x63:
		vt100_key(vt, 0, KC_C, c);
		break;
	case 0x76:
		vt100_key(vt, 0, KC_V, c);
		break;
	case 0x62:
		vt100_key(vt, 0, KC_B, c);
		break;
	case 0x6e:
		vt100_key(vt, 0, KC_N, c);
		break;
	case 0x6d:
		vt100_key(vt, 0, KC_M, c);
		break;

	case 0x2c:
		vt100_key(vt, 0, KC_COMMA, c);
		break;
	case 0x2e:
		vt100_key(vt, 0, KC_PERIOD, c);
		break;
	case 0x2f:
		vt100_key(vt, 0, KC_SLASH, c);
		break;

		/*
		 * Shifted
		 */
	case 0x7e:
		vt100_key(vt, KM_SHIFT, KC_BACKTICK, c);
		break;

	case 0x21:
		vt100_key(vt, KM_SHIFT, KC_1, c);
		break;
	case 0x40:
		vt100_key(vt, KM_SHIFT, KC_2, c);
		break;
	case 0x23:
		vt100_key(vt, KM_SHIFT, KC_3, c);
		break;
	case 0x24:
		vt100_key(vt, KM_SHIFT, KC_4, c);
		break;
	case 0x25:
		vt100_key(vt, KM_SHIFT, KC_5, c);
		break;
	case 0x5e:
		vt100_key(vt, KM_SHIFT, KC_6, c);
		break;
	case 0x26:
		vt100_key(vt, KM_SHIFT, KC_7, c);
		break;
	case 0x2a:
		vt100_key(vt, KM_SHIFT, KC_8, c);
		break;
	case 0x28:
		vt100_key(vt, KM_SHIFT, KC_9, c);
		break;
	case 0x29:
		vt100_key(vt, KM_SHIFT, KC_0, c);
		break;
	case 0x5f:
		vt100_key(vt, KM_SHIFT, KC_MINUS, c);
		break;
	case 0x2b:
		vt100_key(vt, KM_SHIFT, KC_EQUALS, c);
		break;

	case 0x51:
		vt100_key(vt, KM_SHIFT, KC_Q, c);
		break;
	case 0x57:
		vt100_key(vt, KM_SHIFT, KC_W, c);
		break;
	case 0x45:
		vt100_key(vt, KM_SHIFT, KC_E, c);
		break;
	case 0x52:
		vt100_key(vt, KM_SHIFT, KC_R, c);
		break;
	case 0x54:
		vt100_key(vt, KM_SHIFT, KC_T, c);
		break;
	case 0x59:
		vt100_key(vt, KM_SHIFT, KC_Y, c);
		break;
	case 0x55:
		vt100_key(vt, KM_SHIFT, KC_U, c);
		break;
	case 0x49:
		vt100_key(vt, KM_SHIFT, KC_I, c);
		break;
	case 0x4f:
		vt100_key(vt, KM_SHIFT, KC_O, c);
		break;
	case 0x50:
		vt100_key(vt, KM_SHIFT, KC_P, c);
		break;

	case 0x7b:
		vt100_key(vt, KM_SHIFT, KC_LBRACKET, c);
		break;
	case 0x7d:
		vt100_key(vt, KM_SHIFT, KC_RBRACKET, c);
		break;

	case 0x41:
		vt100_key(vt, KM_SHIFT, KC_A, c);
		break;
	case 0x53:
		vt100_key(vt, KM_SHIFT, KC_S, c);
		break;
	case 0x44:
		vt100_key(vt, KM_SHIFT, KC_D, c);
		break;
	case 0x46:
		vt100_key(vt, KM_SHIFT, KC_F, c);
		break;
	case 0x47:
		vt100_key(vt, KM_SHIFT, KC_G, c);
		break;
	case 0x48:
		vt100_key(vt, KM_SHIFT, KC_H, c);
		break;
	case 0x4a:
		vt100_key(vt, KM_SHIFT, KC_J, c);
		break;
	case 0x4b:
		vt100_key(vt, KM_SHIFT, KC_K, c);
		break;
	case 0x4c:
		vt100_key(vt, KM_SHIFT, KC_L, c);
		break;

	case 0x3a:
		vt100_key(vt, KM_SHIFT, KC_SEMICOLON, c);
		break;
	case 0x22:
		vt100_key(vt, KM_SHIFT, KC_QUOTE, c);
		break;
	case 0x7c:
		vt100_key(vt, KM_SHIFT, KC_BACKSLASH, c);
		break;

	case 0x5a:
		vt100_key(vt, KM_SHIFT, KC_Z, c);
		break;
	case 0x58:
		vt100_key(vt, KM_SHIFT, KC_X, c);
		break;
	case 0x43:
		vt100_key(vt, KM_SHIFT, KC_C, c);
		break;
	case 0x56:
		vt100_key(vt, KM_SHIFT, KC_V, c);
		break;
	case 0x42:
		vt100_key(vt, KM_SHIFT, KC_B, c);
		break;
	case 0x4e:
		vt100_key(vt, KM_SHIFT, KC_N, c);
		break;
	case 0x4d:
		vt100_key(vt, KM_SHIFT, KC_M, c);
		break;

	case 0x3c:
		vt100_key(vt, KM_SHIFT, KC_COMMA, c);
		break;
	case 0x3e:
		vt100_key(vt, KM_SHIFT, KC_PERIOD, c);
		break;
	case 0x3f:
		vt100_key(vt, KM_SHIFT, KC_SLASH, c);
		break;

		/*
		 * ...
		 */
	case 0x20:
		vt100_key(vt, 0, KC_SPACE, c);
		break;
	case 0x0a:
		vt100_key(vt, 0, KC_ENTER, '\n');
		break;
	case 0x0d:
		vt100_key(vt, 0, KC_ENTER, '\n');
		break;

		/*
		 * Ctrl + key
		 */
	case 0x11:
		vt100_key(vt, KM_CTRL, KC_Q, c);
		break;
	case 0x17:
		vt100_key(vt, KM_CTRL, KC_W, c);
		break;
	case 0x05:
		vt100_key(vt, KM_CTRL, KC_E, c);
		break;
	case 0x12:
		vt100_key(vt, KM_CTRL, KC_R, c);
		break;
	case 0x14:
		vt100_key(vt, KM_CTRL, KC_T, c);
		break;
	case 0x19:
		vt100_key(vt, KM_CTRL, KC_Y, c);
		break;
	case 0x15:
		vt100_key(vt, KM_CTRL, KC_U, c);
		break;
	case 0x0f:
		vt100_key(vt, KM_CTRL, KC_O, c);
		break;
	case 0x10:
		vt100_key(vt, KM_CTRL, KC_P, c);
		break;

	case 0x01:
		vt100_key(vt, KM_CTRL, KC_A, c);
		break;
	case 0x13:
		vt100_key(vt, KM_CTRL, KC_S, c);
		break;
	case 0x04:
		vt100_key(vt, KM_CTRL, KC_D, c);
		break;
	case 0x06:
		vt100_key(vt, KM_CTRL, KC_F, c);
		break;
	case 0x07:
		vt100_key(vt, KM_CTRL, KC_G, c);
		break;
	case 0x0b:
		vt100_key(vt, KM_CTRL, KC_K, c);
		break;
	case 0x0c:
		vt100_key(vt, KM_CTRL, KC_L, c);
		break;

	case 0x1a:
		vt100_key(vt, KM_CTRL, KC_Z, c);
		break;
	case 0x18:
		vt100_key(vt, KM_CTRL, KC_X, c);
		break;
	case 0x03:
		vt100_key(vt, KM_CTRL, KC_C, c);
		break;
	case 0x16:
		vt100_key(vt, KM_CTRL, KC_V, c);
		break;
	case 0x02:
		vt100_key(vt, KM_CTRL, KC_B, c);
		break;
	case 0x0e:
		vt100_key(vt, KM_CTRL, KC_N, c);
		break;

	case 0x7f:
		vt100_key(vt, 0, KC_BACKSPACE, '\b');
		break;

	case 0x1b:
		vt->state = vts_1b;
		break;
	}
}

void vt100_rcvd_char(vt100_t *vt, char c)
{
	switch (vt->state) {
	case vts_base:
		vt100_rcvd_base(vt, c);
		break;
	case vts_1b:
		vt100_rcvd_1b(vt, c);
		break;
	case vts_1b4f:
		vt100_rcvd_1b4f(vt, c);
		break;
	case vts_1b5b:
		vt100_rcvd_1b5b(vt, c);
		break;
	case vts_1b5b31:
		vt100_rcvd_1b5b31(vt, c);
		break;
	case vts_1b5b3135:
		vt100_rcvd_1b5b3135(vt, c);
		break;
	case vts_1b5b3137:
		vt100_rcvd_1b5b3137(vt, c);
		break;
	case vts_1b5b3138:
		vt100_rcvd_1b5b3138(vt, c);
		break;
	case vts_1b5b3139:
		vt100_rcvd_1b5b3139(vt, c);
		break;
	case vts_1b5b313b:
		vt100_rcvd_1b5b313b(vt, c);
		break;
	case vts_1b5b313b32:
		vt100_rcvd_1b5b313b32(vt, c);
		break;
	case vts_1b5b313b33:
		vt100_rcvd_1b5b313b33(vt, c);
		break;
	case vts_1b5b32:
		vt100_rcvd_1b5b32(vt, c);
		break;
	case vts_1b5b3230:
		vt100_rcvd_1b5b3230(vt, c);
		break;
	case vts_1b5b3231:
		vt100_rcvd_1b5b3231(vt, c);
		break;
	case vts_1b5b3233:
		vt100_rcvd_1b5b3233(vt, c);
		break;
	case vts_1b5b3234:
		vt100_rcvd_1b5b3234(vt, c);
		break;
	case vts_1b5b3235:
		vt100_rcvd_1b5b3235(vt, c);
		break;
	case vts_1b5b3238:
		vt100_rcvd_1b5b3238(vt, c);
		break;
	case vts_1b5b35:
		vt100_rcvd_1b5b35(vt, c);
		break;
	case vts_1b5b33:
		vt100_rcvd_1b5b33(vt, c);
		break;
	case vts_1b5b36:
		vt100_rcvd_1b5b36(vt, c);
		break;
	case vts_1b5b3c:
		vt100_rcvd_1b5b3c(vt, c);
		break;
	}
}

/** @}
 */
