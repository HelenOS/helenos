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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef IMGMAP_SCREENBUFFER_H__
#define IMGMAP_SCREENBUFFER_H__

#include <sys/types.h>
#include <bool.h>
#include <io/color.h>
#include <io/style.h>
#include "fb.h"

typedef enum {
	SCREENBUFFER_FLAG_NONE = 0,
	SCREENBUFFER_FLAG_SHARED = 1
} screenbuffer_flag_t;

typedef enum {
	CHAR_ATTR_STYLE,
	CHAR_ATTR_INDEX,
	CHAR_ATTR_RGB
} char_attr_type_t;

typedef enum {
	CHAR_FLAG_NONE = 0,
	CHAR_FLAG_DIRTY = 1
} char_flags_t;

typedef struct {
	console_color_t bgcolor;
	console_color_t fgcolor;
	console_color_attr_t attr;
} char_attr_index_t;

typedef struct {
	pixel_t bgcolor;  /**< Background color */
	pixel_t fgcolor;  /**< Foreground color */
} char_attr_rgb_t;

typedef union {
	console_style_t style;
	char_attr_index_t index;
	char_attr_rgb_t rgb;
} char_attr_val_t;

typedef struct {
	char_attr_type_t type;
	char_attr_val_t val;
} char_attrs_t;

/** One field on screen. It contain one character and its attributes. */
typedef struct {
	wchar_t ch;          /**< Character itself */
	char_attrs_t attrs;  /**< Character attributes */
	char_flags_t flags;  /**< Character flags */
} charfield_t;

/** Compare two sets of attributes.
 *
 * @param a1 First attribute.
 * @param a2 Second attribute.
 *
 * @return True on equality
 *
 */
static inline bool attrs_same(char_attrs_t a1, char_attrs_t a2)
{
	if (a1.type != a2.type)
		return false;
	
	switch (a1.type) {
	case CHAR_ATTR_STYLE:
		return (a1.val.style == a2.val.style);
	case CHAR_ATTR_INDEX:
		return (a1.val.index.bgcolor == a2.val.index.bgcolor)
		    && (a1.val.index.fgcolor == a2.val.index.fgcolor)
		    && (a1.val.index.attr == a2.val.index.attr);
	case CHAR_ATTR_RGB:
		return (a1.val.rgb.bgcolor == a2.val.rgb.bgcolor)
		    && (a1.val.rgb.fgcolor == a2.val.rgb.fgcolor);
	}
	
	return false;
}

extern screenbuffer_t *screenbuffer_create(sysarg_t, sysarg_t,
    screenbuffer_flag_t);

extern charfield_t *screenbuffer_field_at(screenbuffer_t *, sysarg_t, sysarg_t);
extern bool screenbuffer_cursor_at(screenbuffer_t *, sysarg_t, sysarg_t);

extern sysarg_t screenbuffer_get_top_row(screenbuffer_t *);

extern sysarg_t screenbuffer_putchar(screenbuffer_t *, wchar_t, bool);
extern sysarg_t screenbuffer_newline(screenbuffer_t *);
extern sysarg_t screenbuffer_tabstop(screenbuffer_t *, sysarg_t);
extern sysarg_t screenbuffer_backspace(screenbuffer_t *);

extern void screenbuffer_clear(screenbuffer_t *);
extern void screenbuffer_clear_row(screenbuffer_t *, sysarg_t);

extern void screenbuffer_set_cursor(screenbuffer_t *, sysarg_t, sysarg_t);
extern void screenbuffer_set_cursor_visibility(screenbuffer_t *, bool);
extern bool screenbuffer_get_cursor_visibility(screenbuffer_t *);

extern void screenbuffer_get_cursor(screenbuffer_t *, sysarg_t *, sysarg_t *);

extern void screenbuffer_set_style(screenbuffer_t *, console_style_t);
extern void screenbuffer_set_color(screenbuffer_t *, console_color_t,
    console_color_t, console_color_attr_t);
extern void screenbuffer_set_rgb_color(screenbuffer_t *, pixel_t, pixel_t);

#endif

/** @}
 */
