/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_CHARFIELD_H_
#define _LIBC_IO_CHARFIELD_H_

#include <stdbool.h>
#include <uchar.h>
#include <io/color.h>
#include <io/style.h>
#include <io/pixel.h>

typedef enum {
	CHAR_FLAG_NONE = 0,
	CHAR_FLAG_DIRTY = 1
} char_flags_t;

typedef enum {
	CHAR_ATTR_STYLE,
	CHAR_ATTR_INDEX,
	CHAR_ATTR_RGB
} char_attr_type_t;

typedef struct {
	console_color_t bgcolor;
	console_color_t fgcolor;
	console_color_attr_t attr;
} char_attr_index_t;

typedef struct {
	pixel_t bgcolor;
	pixel_t fgcolor;
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

typedef struct {
	char32_t ch;
	char_attrs_t attrs;
	char_flags_t flags;
} charfield_t;

static inline bool attrs_same(char_attrs_t a1, char_attrs_t a2)
{
	if (a1.type != a2.type)
		return false;

	switch (a1.type) {
	case CHAR_ATTR_STYLE:
		return (a1.val.style == a2.val.style);
	case CHAR_ATTR_INDEX:
		return (a1.val.index.bgcolor == a2.val.index.bgcolor) &&
		    (a1.val.index.fgcolor == a2.val.index.fgcolor) &&
		    (a1.val.index.attr == a2.val.index.attr);
	case CHAR_ATTR_RGB:
		return (a1.val.rgb.bgcolor == a2.val.rgb.bgcolor) &&
		    (a1.val.rgb.fgcolor == a2.val.rgb.fgcolor);
	}

	return false;
}

#endif

/** @}
 */
