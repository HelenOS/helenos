/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup gui
 * @{
 */
/**
 * @file
 */

#include <stddef.h>
#include <stdint.h>
#include <drawctx.h>
#include "common.h"

#define CROSS_WIDTH   14
#define CROSS_HEIGHT  14

static uint8_t cross_texture[] = {
	0x00, 0x00, 0x02, 0x08, 0x04, 0x04, 0x08, 0x02, 0x10, 0x01, 0xa0, 0x00,
	0x40, 0x00, 0xa0, 0x00, 0x10, 0x01, 0x08, 0x02, 0x04, 0x04, 0x02, 0x08,
	0x01, 0x10, 0x00, 0x00
};

static uint8_t cross_mask[] = {
	0x00, 0x00, 0x02, 0x18, 0x06, 0x0c, 0x0c, 0x06, 0x18, 0x03, 0xb0, 0x01,
	0xe0, 0x00, 0xe0, 0x00, 0xb0, 0x01, 0x18, 0x03, 0x0c, 0x06, 0x06, 0x0c,
	0x03, 0x18, 0x00, 0x00
};

void draw_icon_cross(surface_t *surface, sysarg_t hpos, sysarg_t vpos,
    pixel_t highlight, pixel_t shadow)
{
	for (unsigned int y = 0; y < CROSS_HEIGHT; y++) {
		for (unsigned int x = 0; x < CROSS_WIDTH; x++) {
			size_t offset = y * ((CROSS_WIDTH - 1) / 8 + 1) + x / 8;
			bool visible = cross_mask[offset] & (1 << (x % 8));
			pixel_t pixel = (cross_texture[offset] & (1 << (x % 8))) ?
			    highlight : shadow;

			if (visible)
				surface_put_pixel(surface, hpos + x, vpos + y, pixel);
		}
	}
}

void draw_bevel(drawctx_t *drawctx, source_t *source, sysarg_t hpos,
    sysarg_t vpos, sysarg_t width, sysarg_t height, pixel_t highlight,
    pixel_t shadow)
{
	source_set_color(source, highlight);
	drawctx_transfer(drawctx, hpos, vpos, width - 1, 1);
	drawctx_transfer(drawctx, hpos, vpos + 1, 1, height - 2);

	source_set_color(source, shadow);
	drawctx_transfer(drawctx, hpos, vpos + height - 1, width, 1);
	drawctx_transfer(drawctx, hpos + width - 1, vpos, 1, height);
}

/** @}
 */
