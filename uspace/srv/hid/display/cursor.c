/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server cursor
 */

#include <gfx/bitmap.h>
#include <gfx/coord.h>
#include <stdlib.h>
#include "cursor.h"
#include "display.h"

/** Create cursor.
 *
 * @param disp Display
 * @param rect Rectangle bounding cursor graphic
 * @param image Cursor image (one entire byte per pixel)
 * @param rcursor Place to store pointer to new cursor.
 *
 * @return EOK on success or an error code
 */
errno_t ds_cursor_create(ds_display_t *disp, gfx_rect_t *rect,
    const uint8_t *image, ds_cursor_t **rcursor)
{
	ds_cursor_t *cursor = NULL;
	errno_t rc;

	cursor = calloc(1, sizeof(ds_cursor_t));
	if (cursor == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ds_display_add_cursor(disp, cursor);

	cursor->rect = *rect;
	cursor->image = image;
	*rcursor = cursor;
	return EOK;
error:
	return rc;
}

/** Destroy cusror.
 *
 * @param cursor Cursor
 */
void ds_cursor_destroy(ds_cursor_t *cursor)
{
	list_remove(&cursor->ldisplay);

	if (cursor->bitmap != NULL)
		gfx_bitmap_destroy(cursor->bitmap);

	free(cursor);
}

/** Paint cusor.
 *
 * @param cursor Cusor to paint
 * @param pos Position to paint at
 * @param clip Clipping rectangle or @c NULL
 * @return EOK on success or an error code
 */
errno_t ds_cursor_paint(ds_cursor_t *cursor, gfx_coord2_t *pos,
    gfx_rect_t *clip)
{
	gfx_context_t *dgc;
	gfx_coord_t x, y;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_alloc_t alloc;
	gfx_coord2_t dims;
	gfx_rect_t sclip;
	gfx_rect_t srect;
	pixelmap_t pixelmap;
	pixel_t pixel;
	const uint8_t *pp;
	errno_t rc;

	dgc = ds_display_get_gc(cursor->display);
	if (dgc == NULL)
		return EOK;

	gfx_bitmap_params_init(&bparams);
	bparams.rect = cursor->rect;
	bparams.flags = bmpf_color_key;
	bparams.key_color = PIXEL(0, 0, 255, 255);

	if (cursor->bitmap == NULL) {
		rc = gfx_bitmap_create(dgc, &bparams, NULL, &cursor->bitmap);
		if (rc != EOK)
			goto error;

		rc = gfx_bitmap_get_alloc(cursor->bitmap, &alloc);
		if (rc != EOK) {
			gfx_bitmap_destroy(cursor->bitmap);
			cursor->bitmap = NULL;
			goto error;
		}

		gfx_rect_dims(&cursor->rect, &dims);
		pixelmap.width = dims.x;
		pixelmap.height = dims.y;
		pixelmap.data = alloc.pixels;

		pp = cursor->image;
		for (y = 0; y < dims.y; y++) {
			for (x = 0; x < dims.x; x++) {
				switch (*pp) {
				case 1:
					pixel = PIXEL(0, 0, 0, 0);
					break;
				case 2:
					pixel = PIXEL(0, 255, 255, 255);
					break;
				default:
					pixel = PIXEL(0, 0, 255, 255);
					break;
				}

				pixelmap_put_pixel(&pixelmap, x, y, pixel);
				++pp;
			}
		}
	}

	/* Determine source rectangle */
	if (clip == NULL) {
		srect = cursor->rect;
	} else {
		gfx_rect_rtranslate(pos, clip, &sclip);
		gfx_rect_clip(&cursor->rect, &sclip, &srect);
	}

	if (!gfx_rect_is_empty(&srect)) {
		rc = gfx_bitmap_render(cursor->bitmap, &srect, pos);
		if (rc != EOK)
			return rc;
	}

	return EOK;
error:
	return rc;
}

/** Get rectangle covered by cursor when drawn at a specified position.
 *
 * @param cursor Cursor
 * @param pos Position where cursor is situated
 * @param rect Place to store rectangle covered by cursor
 */
void ds_cursor_get_rect(ds_cursor_t *cursor, gfx_coord2_t *pos,
    gfx_rect_t *drect)
{
	gfx_rect_translate(pos, &cursor->rect, drect);
}

/** @}
 */
