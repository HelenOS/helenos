/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
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
/**
 * @file
 */

#include <errno.h>
#include <io/chargrid.h>
#include "../output.h"
#include "../proto/vt100.h"
#include "serial.h"

#define SERIAL_COLS  80
#define SERIAL_ROWS  24

/** Draw the character at the specified position.
 *
 * @param state VT100 protocol state.
 * @param field Character field.
 * @param col   Horizontal screen position.
 * @param row   Vertical screen position.
 *
 */
static void draw_char(vt100_state_t *state, charfield_t *field,
    sysarg_t col, sysarg_t row)
{
	vt100_goto(state, col, row);
	vt100_set_attr(state, field->attrs);
	vt100_putchar(state, field->ch);
}

static errno_t serial_yield(outdev_t *dev)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;

	return vt100_yield(state);
}

static errno_t serial_claim(outdev_t *dev)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;

	return vt100_claim(state);
}

static void serial_get_dimensions(outdev_t *dev, sysarg_t *cols,
    sysarg_t *rows)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;

	vt100_get_dimensions(state, cols, rows);
}

static console_caps_t serial_get_caps(outdev_t *dev)
{
	return (CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED);
}

static void serial_cursor_update(outdev_t *dev, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;

	vt100_goto(state, col, row);
	vt100_cursor_visibility(state, visible);
}

static void serial_char_update(outdev_t *dev, sysarg_t col, sysarg_t row)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	charfield_t *field =
	    chargrid_charfield_at(dev->backbuf, col, row);

	draw_char(state, field, col, row);
}

static void serial_flush(outdev_t *dev)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;

	vt100_flush(state);
}

static outdev_ops_t serial_ops = {
	.yield = serial_yield,
	.claim = serial_claim,
	.get_dimensions = serial_get_dimensions,
	.get_caps = serial_get_caps,
	.cursor_update = serial_cursor_update,
	.char_update = serial_char_update,
	.flush = serial_flush
};

errno_t serial_init(vt100_putchar_t putchar_fn,
    vt100_control_puts_t control_puts_fn, vt100_flush_t flush_fn)
{
	vt100_state_t *state =
	    vt100_state_create(SERIAL_COLS, SERIAL_ROWS, putchar_fn,
	    control_puts_fn, flush_fn);
	if (state == NULL)
		return ENOMEM;

	outdev_t *dev = outdev_register(&serial_ops, state);
	if (dev == NULL) {
		vt100_state_destroy(state);
		return ENOMEM;
	}

	return EOK;
}

/** @}
 */
