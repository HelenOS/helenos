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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef LIBC_IO_CHARGRID_H_
#define LIBC_IO_CHARGRID_H_

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

extern sysarg_t chargrid_putchar(chargrid_t *, wchar_t, bool);
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
