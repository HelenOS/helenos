/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Bitmap
 */

#include <gfx/bitmap.h>
#include <mem.h>
#include <stdint.h>
#include <stdlib.h>
#include "../private/bitmap.h"
#include "../private/context.h"

/** Initialize bitmap parameters structure.
 *
 * Bitmap parameters structure must always be initialized using this function
 * first.
 *
 * @param params Bitmap parameters structure
 */
void gfx_bitmap_params_init(gfx_bitmap_params_t *params)
{
	memset(params, 0, sizeof(gfx_bitmap_params_t));
}

/** Allocate bitmap in a graphics context.
 *
 * @param gc Graphic context
 * @param params Bitmap parameters
 * @param alloc Bitmap allocation info or @c NULL to let GC allocate
 *               pixel storage
 * @param rbitmap Place to store pointer to new bitmap
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_bitmap_create(gfx_context_t *gc, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, gfx_bitmap_t **rbitmap)
{
	void *bm_priv;
	gfx_bitmap_t *bitmap;
	errno_t rc;

	bitmap = calloc(1, sizeof(gfx_bitmap_t));
	if (bitmap == NULL)
		return ENOMEM;

	rc = gc->ops->bitmap_create(gc->arg, params, alloc, &bm_priv);
	if (rc != EOK) {
		free(bitmap);
		return rc;
	}

	bitmap->gc = gc;
	bitmap->gc_priv = bm_priv;
	*rbitmap = bitmap;
	return EOK;
}

/** Destroy bitmap from graphics context.
 *
 * @param bitmap Bitmap
 *
 * @return EOK on success, EIO if graphic device connection was lost
 */
errno_t gfx_bitmap_destroy(gfx_bitmap_t *bitmap)
{
	errno_t rc;

	rc = bitmap->gc->ops->bitmap_destroy(bitmap->gc_priv);
	if (rc != EOK)
		return rc;

	free(bitmap);
	return EOK;
}

/** Render bitmap in graphics context.
 *
 * @param bitmap Bitmap
 * @param srect Source rectangle or @c NULL to render entire bitmap
 * @param offs Bitmap offset or @c NULL for zero offset
 *
 * @return EOK on success, EIO if graphic device connection was lost
 */
errno_t gfx_bitmap_render(gfx_bitmap_t *bitmap, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	return bitmap->gc->ops->bitmap_render(bitmap->gc_priv, srect, offs);
}

/** Get bitmap allocation info.
 *
 * @param bitmap Bitmap
 * @param alloc Allocation info structure to fill in
 *
 * @return EOK on success, EIO if graphic device connection was lost
 */
errno_t gfx_bitmap_get_alloc(gfx_bitmap_t *bitmap, gfx_bitmap_alloc_t *alloc)
{
	return bitmap->gc->ops->bitmap_get_alloc(bitmap->gc_priv, alloc);
}

/** @}
 */
