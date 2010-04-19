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

#ifndef SCREENBUFFER_H__
#define SCREENBUFFER_H__

#include <stdint.h>
#include <sys/types.h>
#include <ipc/ipc.h>
#include <bool.h>

#define DEFAULT_FOREGROUND  0x000000  /**< default console foreground color */
#define DEFAULT_BACKGROUND  0xf0f0f0  /**< default console background color */

typedef struct {
	uint8_t style;
} attr_style_t;

typedef struct {
	uint8_t fg_color;
	uint8_t bg_color;
	uint8_t flags;
} attr_idx_t;

typedef struct {
	uint32_t bg_color;  /**< background color */
	uint32_t fg_color;  /**< foreground color */
} attr_rgb_t;

typedef struct {
	enum {
		at_style,
		at_idx,
		at_rgb
	} t;
	union {
		attr_style_t s;
		attr_idx_t i;
		attr_rgb_t r;
	} a;
} attrs_t;

/** One field on screen. It contain one character and its attributes. */
typedef struct {
	wchar_t character;  /**< Character itself */
	attrs_t attrs;      /**< Character attributes */
} keyfield_t;

/** Structure for buffering state of one virtual console.
 */
typedef struct {
	keyfield_t *buffer;      /**< Screen content - characters and
	                              their attributes (used as a circular buffer) */
	ipcarg_t size_x;         /**< Number of columns  */
	ipcarg_t size_y;         /**< Number of rows */
	
	/** Coordinates of last printed character for determining cursor position */
	ipcarg_t position_x;
	ipcarg_t position_y;
	
	attrs_t attrs;           /**< Current attributes. */
	size_t top_line;         /**< Points to buffer[][] line that will
	                              be printed at screen as the first line */
	bool is_cursor_visible;  /**< Cursor state - default is visible */
} screenbuffer_t;

/** Returns keyfield for position on screen
 *
 * Screenbuffer->buffer is cyclic buffer so we
 * must couted in index of the topmost line.
 *
 * @param scr Screenbuffer
 * @param x   Position on screen
 * @param y   Position on screen
 *
 * @return Keyfield structure with character and its attributes on x, y
 *
 */
static inline keyfield_t *get_field_at(screenbuffer_t *scr, ipcarg_t x, ipcarg_t y)
{
	return scr->buffer + x + ((y + scr->top_line) % scr->size_y) * scr->size_x;
}

/** Compares two sets of attributes.
 *
 * @param s1 First style
 * @param s2 Second style
 *
 * @return Nonzero on equality
 *
 */
static inline int attrs_same(attrs_t a1, attrs_t a2)
{
	if (a1.t != a2.t)
		return 0;
	
	switch (a1.t) {
	case at_style:
		return (a1.a.s.style == a2.a.s.style);
	case at_idx:
		return (a1.a.i.fg_color == a2.a.i.fg_color)
		    && (a1.a.i.bg_color == a2.a.i.bg_color)
		    && (a1.a.i.flags == a2.a.i.flags);
	case at_rgb:
		return (a1.a.r.fg_color == a2.a.r.fg_color)
		    && (a1.a.r.bg_color == a2.a.r.bg_color);
	}
	
	return 0;
}

extern void screenbuffer_putchar(screenbuffer_t *, wchar_t);
extern screenbuffer_t *screenbuffer_init(screenbuffer_t *, ipcarg_t, ipcarg_t);

extern void screenbuffer_clear(screenbuffer_t *);
extern void screenbuffer_clear_line(screenbuffer_t *, ipcarg_t);
extern void screenbuffer_copy_buffer(screenbuffer_t *, keyfield_t *);
extern void screenbuffer_goto(screenbuffer_t *, ipcarg_t, ipcarg_t);
extern void screenbuffer_set_style(screenbuffer_t *, uint8_t);
extern void screenbuffer_set_color(screenbuffer_t *, uint8_t, uint8_t, uint8_t);
extern void screenbuffer_set_rgb_color(screenbuffer_t *, uint32_t, uint32_t);

#endif

/** @}
 */
