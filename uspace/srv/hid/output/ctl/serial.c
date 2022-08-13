/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 * SPDX-FileCopyrightText: 2008 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup output
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
	vt100_putuchar(state, field->ch);
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

errno_t serial_init(vt100_putuchar_t putuchar_fn,
    vt100_control_puts_t control_puts_fn, vt100_flush_t flush_fn)
{
	vt100_state_t *state =
	    vt100_state_create(SERIAL_COLS, SERIAL_ROWS, putuchar_fn,
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
