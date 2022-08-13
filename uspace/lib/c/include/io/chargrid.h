/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_IO_CHARGRID_H_
#define _LIBC_IO_CHARGRID_H_

#include <io/charfield.h>
#include <types/common.h>
#include <stddef.h>

typedef enum {
	CHARGRID_FLAG_NONE = 0,
	CHARGRID_FLAG_SHARED = 1
} chargrid_flag_t;

typedef struct {
	size_t size;            /**< Structure size */
	chargrid_flag_t flags;  /**< Screenbuffer flags */

	sysarg_t cols;          /**< Number of columns */
	sysarg_t rows;          /**< Number of rows */

	sysarg_t col;           /**< Current column */
	sysarg_t row;           /**< Current row */
	bool cursor_visible;    /**< Cursor visibility */

	char_attrs_t attrs;     /**< Current attributes */

	sysarg_t top_row;       /**< The first row in the cyclic buffer */
	charfield_t data[];     /**< Screen contents (cyclic buffer) */
} chargrid_t;

static inline charfield_t *chargrid_charfield_at(chargrid_t *chargrid,
    sysarg_t col, sysarg_t row)
{
	return chargrid->data +
	    ((row + chargrid->top_row) % chargrid->rows) * chargrid->cols +
	    col;
}

extern chargrid_t *chargrid_create(sysarg_t, sysarg_t,
    chargrid_flag_t);
extern void chargrid_destroy(chargrid_t *);

extern bool chargrid_cursor_at(chargrid_t *, sysarg_t, sysarg_t);

extern sysarg_t chargrid_get_top_row(chargrid_t *);

extern sysarg_t chargrid_putuchar(chargrid_t *, char32_t, bool);
extern sysarg_t chargrid_newline(chargrid_t *);
extern sysarg_t chargrid_tabstop(chargrid_t *, sysarg_t);
extern sysarg_t chargrid_backspace(chargrid_t *);

extern void chargrid_clear(chargrid_t *);
extern void chargrid_clear_row(chargrid_t *, sysarg_t);

extern void chargrid_set_cursor(chargrid_t *, sysarg_t, sysarg_t);
extern void chargrid_set_cursor_visibility(chargrid_t *, bool);
extern bool chargrid_get_cursor_visibility(chargrid_t *);

extern void chargrid_get_cursor(chargrid_t *, sysarg_t *, sysarg_t *);

extern void chargrid_set_style(chargrid_t *, console_style_t);
extern void chargrid_set_color(chargrid_t *, console_color_t,
    console_color_t, console_color_attr_t);
extern void chargrid_set_rgb_color(chargrid_t *, pixel_t, pixel_t);

#endif

/** @}
 */
