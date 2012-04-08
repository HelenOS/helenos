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

/** @file
 */

#include <sys/types.h>
#include <errno.h>
#include <sysinfo.h>
#include <task.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <as.h>
#include <malloc.h>
#include <align.h>
#include <screenbuffer.h>
#include "../fb.h"
#include "ega.h"

#define EGA_IO_BASE  ((ioport8_t *) 0x3d4)
#define EGA_IO_SIZE  2

#define FB_POS(x, y)  (((y) * ega.width + (x)) << 1)

typedef struct {
	sysarg_t width;
	sysarg_t height;
	
	size_t size;
	uint8_t *addr;
	
	uint8_t style_normal;
	uint8_t style_inverted;
	
	uint8_t *backbuf;
} ega_t;

static ega_t ega;

static uint8_t attrs_attr(char_attrs_t attrs)
{
	uint8_t attr = 0;
	
	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			attr = ega.style_normal;
			break;
		case STYLE_EMPHASIS:
			attr = (ega.style_normal | 0x04);
			break;
		case STYLE_INVERTED:
			attr = ega.style_inverted;
			break;
		case STYLE_SELECTED:
			attr = (ega.style_inverted | 0x40);
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		attr = ((attrs.val.index.bgcolor & 7) << 4) |
		    (attrs.val.index.fgcolor & 7);
		
		if (attrs.val.index.attr & CATTR_BRIGHT)
			attr |= 0x08;
		
		break;
	case CHAR_ATTR_RGB:
		attr = (attrs.val.rgb.bgcolor < attrs.val.rgb.fgcolor) ?
		    ega.style_inverted : ega.style_normal;
		break;
	}
	
	return attr;
}

/** Draw the character at the specified position in viewport.
 *
 * @param vp     Viewport.
 * @param col    Screen position relative to viewport.
 * @param row    Screen position relative to viewport.
 *
 */
static void draw_vp_char(fbvp_t *vp, sysarg_t col, sysarg_t row)
{
	sysarg_t x = vp->x + col;
	sysarg_t y = vp->y + row;
	
	charfield_t *field = screenbuffer_field_at(vp->backbuf, col, row);
	
	uint8_t glyph;
	
	if (ascii_check(field->ch))
		glyph = field->ch;
	else
		glyph = '?';
	
	uint8_t attr = attrs_attr(field->attrs);
	
	ega.addr[FB_POS(x, y)] = glyph;
	ega.addr[FB_POS(x, y) + 1] = attr;
}

static int ega_yield(fbdev_t *dev)
{
	if (ega.backbuf == NULL) {
		ega.backbuf = malloc(ega.size);
		if (ega.backbuf == NULL)
			return ENOMEM;
	}
	
	memcpy(ega.backbuf, ega.addr, ega.size);
	return EOK;
}

static int ega_claim(fbdev_t *dev)
{
	if (ega.backbuf == NULL)
		return ENOENT;
	
	memcpy(ega.addr, ega.backbuf, ega.size);
	return EOK;
}

static int ega_get_resolution(fbdev_t *dev, sysarg_t *width, sysarg_t *height)
{
	*width = ega.width;
	*height = ega.height;
	return EOK;
}

static void ega_font_metrics(fbdev_t *dev, sysarg_t width, sysarg_t height,
    sysarg_t *cols, sysarg_t *rows)
{
	*cols = width;
	*rows = height;
}

static int ega_vp_create(fbdev_t *dev, fbvp_t *vp)
{
	vp->attrs.type = CHAR_ATTR_STYLE;
	vp->attrs.val.style = STYLE_NORMAL;
	vp->data = NULL;
	
	return EOK;
}

static void ega_vp_destroy(fbdev_t *dev, fbvp_t *vp)
{
	/* No-op */
}

static void ega_vp_clear(fbdev_t *dev, fbvp_t *vp)
{
	for (sysarg_t row = 0; row < vp->rows; row++) {
		for (sysarg_t col = 0; col < vp->cols; col++) {
			charfield_t *field =
			    screenbuffer_field_at(vp->backbuf, col, row);
			
			field->ch = 0;
			field->attrs = vp->attrs;
			
			draw_vp_char(vp, col, row);
		}
	}
}

static console_caps_t ega_vp_get_caps(fbdev_t *dev, fbvp_t *vp)
{
	return (CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED);
}

static void ega_vp_cursor_update(fbdev_t *dev, fbvp_t *vp, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	/* Cursor position */
	uint16_t cursor = row * ega.width + col;
	
	pio_write_8(EGA_IO_BASE, 0x0e);
	pio_write_8(EGA_IO_BASE + 1, (cursor >> 8) & 0xff);
	pio_write_8(EGA_IO_BASE, 0x0f);
	pio_write_8(EGA_IO_BASE + 1, cursor & 0xff);
	
	/* Cursor visibility */
	pio_write_8(EGA_IO_BASE, 0x0a);
	uint8_t stat = pio_read_8(EGA_IO_BASE + 1);
	
	pio_write_8(EGA_IO_BASE, 0x0a);
	
	if (visible)
		pio_write_8(EGA_IO_BASE + 1, stat & (~(1 << 5)));
	else
		pio_write_8(EGA_IO_BASE + 1, stat | (1 << 5));
}

static void ega_vp_char_update(fbdev_t *dev, fbvp_t *vp, sysarg_t col,
    sysarg_t row)
{
	draw_vp_char(vp, col, row);
}

static fbdev_ops_t ega_ops = {
	.yield = ega_yield,
	.claim = ega_claim,
	.get_resolution = ega_get_resolution,
	.font_metrics = ega_font_metrics,
	.vp_create = ega_vp_create,
	.vp_destroy = ega_vp_destroy,
	.vp_clear = ega_vp_clear,
	.vp_get_caps = ega_vp_get_caps,
	.vp_cursor_update = ega_vp_cursor_update,
	.vp_char_update = ega_vp_char_update
};

int ega_init(void)
{
	sysarg_t present;
	int rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;
	
	if (!present)
		return ENOENT;
	
	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;
	
	if (kind != 2)
		return EINVAL;
	
	sysarg_t paddr;
	rc = sysinfo_get_value("fb.address.physical", &paddr);
	if (rc != EOK)
		return rc;
	
	sysarg_t width;
	rc = sysinfo_get_value("fb.width", &width);
	if (rc != EOK)
		return rc;
	
	sysarg_t height;
	rc = sysinfo_get_value("fb.height", &height);
	if (rc != EOK)
		return rc;
	
	rc = iospace_enable(task_get_id(), (void *) EGA_IO_BASE, EGA_IO_SIZE);
	if (rc != EOK)
		return rc;
	
	ega.width = width;
	ega.height = height;
	
	ega.size = (width * height) << 1;
	
	rc = physmem_map((void *) paddr,
	    ALIGN_UP(ega.size, PAGE_SIZE) >> PAGE_WIDTH,
	    AS_AREA_READ | AS_AREA_WRITE, (void *) &ega.addr);
	if (rc != EOK)
		return rc;
	
	sysarg_t blinking;
	rc = sysinfo_get_value("fb.blinking", &blinking);
	if (rc != EOK)
		blinking = false;
	
	ega.style_normal = 0xf0;
	ega.style_inverted = 0x0f;
	
	if (blinking) {
		ega.style_normal &= 0x77;
		ega.style_inverted &= 0x77;
	}
	
	ega.backbuf = NULL;
	
	fbdev_t *dev = fbdev_register(&ega_ops, (void *) &ega);
	if (dev == NULL) {
		as_area_destroy(ega.addr);
		return EINVAL;
	}
	
	return EOK;
}

/**
 * @}
 */
