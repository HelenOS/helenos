/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup output
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <io/chargrid.h>
#include <vt/vt100.h>
#include "../output.h"
#include "serial.h"

#define SERIAL_COLS  80
#define SERIAL_ROWS  24

static serial_putuchar_t serial_putuchar_fn;
static serial_control_puts_t serial_control_puts_fn;
static serial_flush_t serial_flush_fn;

static void serial_vt_putuchar(void *, char32_t);
static void serial_vt_control_puts(void *, const char *);
static void serial_vt_flush(void *);

static vt100_cb_t serial_vt_cb = {
	.putuchar = serial_vt_putuchar,
	.control_puts = serial_vt_control_puts,
	.flush = serial_vt_flush
};

/** Draw the character at the specified position.
 *
 * @param state VT100 protocol state.
 * @param field Character field.
 * @param col   Horizontal screen position.
 * @param row   Vertical screen position.
 *
 */
static void draw_char(vt100_t *state, charfield_t *field,
    sysarg_t col, sysarg_t row)
{
	vt100_goto(state, col, row);
	vt100_set_attr(state, field->attrs);
	vt100_putuchar(state, field->ch);
}

static errno_t serial_yield(outdev_t *dev)
{
	vt100_t *state = (vt100_t *) dev->data;

	return vt100_yield(state);
}

static errno_t serial_claim(outdev_t *dev)
{
	vt100_t *state = (vt100_t *) dev->data;

	return vt100_claim(state);
}

static void serial_get_dimensions(outdev_t *dev, sysarg_t *cols,
    sysarg_t *rows)
{
	vt100_t *state = (vt100_t *) dev->data;

	vt100_get_dimensions(state, cols, rows);
}

static console_caps_t serial_get_caps(outdev_t *dev)
{
	return (CONSOLE_CAP_CURSORCTL | CONSOLE_CAP_STYLE |
	    CONSOLE_CAP_INDEXED | CONSOLE_CAP_RGB);
}

static void serial_cursor_update(outdev_t *dev, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	vt100_t *state = (vt100_t *) dev->data;

	vt100_goto(state, col, row);
	vt100_cursor_visibility(state, visible);
}

static void serial_char_update(outdev_t *dev, sysarg_t col, sysarg_t row)
{
	vt100_t *state = (vt100_t *) dev->data;
	charfield_t *field =
	    chargrid_charfield_at(dev->backbuf, col, row);

	draw_char(state, field, col, row);
}

static void serial_flush(outdev_t *dev)
{
	vt100_t *state = (vt100_t *) dev->data;

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

errno_t serial_init(serial_putuchar_t putuchar_fn,
    serial_control_puts_t control_puts_fn, serial_flush_t flush_fn)
{
	char_attrs_t attrs;
	vt100_t *vt100;

	serial_putuchar_fn = putuchar_fn;
	serial_control_puts_fn = control_puts_fn;
	serial_flush_fn = flush_fn;

	vt100 = vt100_create(NULL, SERIAL_COLS, SERIAL_ROWS, &serial_vt_cb);
	if (vt100 == NULL)
		return ENOMEM;
	vt100->enable_rgb = true;

	vt100_cursor_visibility(vt100, false);
	attrs.type = CHAR_ATTR_STYLE;
	attrs.val.style = STYLE_NORMAL;
	vt100_set_attr(vt100, attrs);
	vt100_cls(vt100);

	outdev_t *dev = outdev_register(&serial_ops, vt100);
	if (dev == NULL) {
		vt100_destroy(vt100);
		return ENOMEM;
	}

	return EOK;
}

static void serial_vt_putuchar(void *arg, char32_t c)
{
	(void)arg;
	serial_putuchar_fn(c);
}

static void serial_vt_control_puts(void *arg, const char *str)
{
	(void)arg;
	serial_control_puts_fn(str);
}

static void serial_vt_flush(void *arg)
{
	(void)arg;
	serial_flush_fn();
}

/** @}
 */
