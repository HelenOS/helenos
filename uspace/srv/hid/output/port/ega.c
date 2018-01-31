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

#include <errno.h>
#include <sysinfo.h>
#include <align.h>
#include <as.h>
#include <ddi.h>
#include <io/chargrid.h>
#include "../output.h"
#include "ega.h"

#define EGA_IO_BASE  ((ioport8_t *) 0x3d4)
#define EGA_IO_SIZE  2

#define FB_POS(x, y)  (((y) * ega.cols + (x)) << 1)

typedef struct {
	sysarg_t cols;
	sysarg_t rows;
	
	uint8_t style_normal;
	uint8_t style_inverted;
	
	size_t size;
	uint8_t *addr;
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

/** Draw the character at the specified position.
 *
 * @param field Character field.
 * @param col   Horizontal screen position.
 * @param row   Vertical screen position.
 *
 */
static void draw_char(charfield_t *field, sysarg_t col, sysarg_t row)
{
	uint8_t glyph;
	
	if (ascii_check(field->ch))
		glyph = field->ch;
	else
		glyph = '?';
	
	uint8_t attr = attrs_attr(field->attrs);
	
	ega.addr[FB_POS(col, row)] = glyph;
	ega.addr[FB_POS(col, row) + 1] = attr;
}

static errno_t ega_yield(outdev_t *dev)
{
	return EOK;
}

static errno_t ega_claim(outdev_t *dev)
{
	return EOK;
}

static void ega_get_dimensions(outdev_t *dev, sysarg_t *cols, sysarg_t *rows)
{
	*cols = ega.cols;
	*rows = ega.rows;
}

static console_caps_t ega_get_caps(outdev_t *dev)
{
	return (CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED);
}

static void ega_cursor_update(outdev_t *dev, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	/* Cursor position */
	uint16_t cursor = row * ega.cols + col;
	
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

static void ega_char_update(outdev_t *dev, sysarg_t col, sysarg_t row)
{
	charfield_t *field =
	    chargrid_charfield_at(dev->backbuf, col, row);
	
	draw_char(field, col, row);
}

static void ega_flush(outdev_t *dev)
{
}

static outdev_ops_t ega_ops = {
	.yield = ega_yield,
	.claim = ega_claim,
	.get_dimensions = ega_get_dimensions,
	.get_caps = ega_get_caps,
	.cursor_update = ega_cursor_update,
	.char_update = ega_char_update,
	.flush = ega_flush
};

errno_t ega_init(void)
{
	sysarg_t present;
	errno_t rc = sysinfo_get_value("fb", &present);
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
	
	rc = sysinfo_get_value("fb.width", &ega.cols);
	if (rc != EOK)
		return rc;
	
	rc = sysinfo_get_value("fb.height", &ega.rows);
	if (rc != EOK)
		return rc;
	
	rc = pio_enable((void*)EGA_IO_BASE, EGA_IO_SIZE, NULL);
	if (rc != EOK)
		return rc;
	
	ega.size = (ega.cols * ega.rows) << 1;
	ega.addr = AS_AREA_ANY;
	
	rc = physmem_map(paddr,
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
	
	outdev_t *dev = outdev_register(&ega_ops, (void *) &ega);
	if (dev == NULL) {
		as_area_destroy(ega.addr);
		return EINVAL;
	}
	
	return EOK;
}

/** @}
 */
