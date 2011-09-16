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

/** @file
 */

#include <sys/types.h>
#include <errno.h>
#include <malloc.h>
#include <screenbuffer.h>
#include "../proto/vt100.h"
#include "../fb.h"
#include "serial.h"

/** Draw the character at the specified position in viewport.
 *
 * @param state VT100 protocol state.
 * @param vp    Viewport.
 * @param col   Screen position relative to viewport.
 * @param row   Screen position relative to viewport.
 *
 */
static void draw_vp_char(vt100_state_t *state, fbvp_t *vp, sysarg_t col,
    sysarg_t row)
{
	sysarg_t x = vp->x + col;
	sysarg_t y = vp->y + row;
	
	charfield_t *field = screenbuffer_field_at(vp->backbuf, col, row);
	
	vt100_goto(state, x, y);
	vt100_set_attr(state, field->attrs);
	vt100_putchar(state, field->ch);
}

static int serial_yield(fbdev_t *dev)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	return vt100_yield(state);
}

static int serial_claim(fbdev_t *dev)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	return vt100_claim(state);
}

static int serial_get_resolution(fbdev_t *dev, sysarg_t *width, sysarg_t *height)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	vt100_get_resolution(state, width, height);
	return EOK;
}

static void serial_font_metrics(fbdev_t *dev, sysarg_t width, sysarg_t height,
    sysarg_t *cols, sysarg_t *rows)
{
	*cols = width;
	*rows = height;
}

static int serial_vp_create(fbdev_t *dev, fbvp_t *vp)
{
	vp->attrs.type = CHAR_ATTR_STYLE;
	vp->attrs.val.style = STYLE_NORMAL;
	vp->data = NULL;
	
	return EOK;
}

static void serial_vp_destroy(fbdev_t *dev, fbvp_t *vp)
{
	/* No-op */
}

static void serial_vp_clear(fbdev_t *dev, fbvp_t *vp)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	
	for (sysarg_t row = 0; row < vp->rows; row++) {
		for (sysarg_t col = 0; col < vp->cols; col++) {
			charfield_t *field =
			    screenbuffer_field_at(vp->backbuf, col, row);
			
			field->ch = 0;
			field->attrs = vp->attrs;
			
			draw_vp_char(state, vp, col, row);
		}
	}
}

static console_caps_t serial_vp_get_caps(fbdev_t *dev, fbvp_t *vp)
{
	return (CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED);
}

static void serial_vp_cursor_update(fbdev_t *dev, fbvp_t *vp,
    sysarg_t prev_col, sysarg_t prev_row, sysarg_t col, sysarg_t row,
    bool visible)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	
	vt100_goto(state, vp->x + col, vp->y + row);
	vt100_cursor_visibility(state, visible);
}

static void serial_vp_char_update(fbdev_t *dev, fbvp_t *vp, sysarg_t col,
    sysarg_t row)
{
	vt100_state_t *state = (vt100_state_t *) dev->data;
	draw_vp_char(state, vp, col, row);
}

static fbdev_ops_t serial_ops = {
	.yield = serial_yield,
	.claim = serial_claim,
	.get_resolution = serial_get_resolution,
	.font_metrics = serial_font_metrics,
	.vp_create = serial_vp_create,
	.vp_destroy = serial_vp_destroy,
	.vp_clear = serial_vp_clear,
	.vp_get_caps = serial_vp_get_caps,
	.vp_cursor_update = serial_vp_cursor_update,
	.vp_char_update = serial_vp_char_update
};

int serial_init(vt100_putchar_t putchar_fn,
    vt100_control_puts_t control_puts_fn)
{
	vt100_state_t *state =
	    vt100_state_create(80, 24, putchar_fn, control_puts_fn);
	if (state == NULL)
		return ENOMEM;
	
	fbdev_t *dev = fbdev_register(&serial_ops, state);
	if (dev == NULL)
		return EINVAL;
	
	return EOK;
}

/** @}
 */
