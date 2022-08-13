/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
