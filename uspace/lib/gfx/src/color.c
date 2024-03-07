/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Color operations
 */

#include <gfx/color.h>
#include <stdint.h>
#include <stdlib.h>
#include "../private/color.h"

/** Create new 16-bit per channel RGB color.
 *
 * Create a new RGB color where the R, G, B components have 16 bits
 * of precision each.
 *
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param rcolor Place to store pointer to new color
 *
 * @return EOK on success or an error code, ENOMEM if out of resources,
 *         EIO if the graphic device connection was lost
 */
errno_t gfx_color_new_rgb_i16(uint16_t r, uint16_t g, uint16_t b,
    gfx_color_t **rcolor)
{
	gfx_color_t *color;

	color = calloc(1, sizeof(gfx_color_t));
	if (color == NULL)
		return ENOMEM;

	color->r = r;
	color->g = g;
	color->b = b;
	color->attr = 0xff;

	*rcolor = color;
	return EOK;
}

/** Create new EGA color.
 *
 * @param attr EGA attributes
 * @param rcolor Place to store pointer to new color
 *
 * @return EOK on success or an error code, ENOMEM if out of resources,
 *         EIO if the graphic device connection was lost
 */
errno_t gfx_color_new_ega(uint8_t attr, gfx_color_t **rcolor)
{
	gfx_color_t *color;

	color = calloc(1, sizeof(gfx_color_t));
	if (color == NULL)
		return ENOMEM;

	color->attr = attr;

	*rcolor = color;
	return EOK;
}

/** Delete color.
 *
 * @param color Color
 */
void gfx_color_delete(gfx_color_t *color)
{
	free(color);
}

/** Convert color to 16-bit RGB coordinates.
 *
 * @param color Color
 * @param r Place to store red coordinate
 * @param g Place to store green coordinate
 * @param b Place to store blue coordinate
 */
void gfx_color_get_rgb_i16(gfx_color_t *color, uint16_t *r, uint16_t *g,
    uint16_t *b)
{
	*r = color->r;
	*g = color->g;
	*b = color->b;
}

/** Convert color to EGA attributes.
 *
 * @param color Color
 * @param attr Place to store EGA attributes
 */
void gfx_color_get_ega(gfx_color_t *color, uint8_t *attr)
{
	*attr = color->attr;
}

/** @}
 */
