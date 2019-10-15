/*
 * Copyright (c) 2019 Jiri Svoboda
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
 * @file Bitmap
 */

#include <gfx/bitmap.h>
#include <stdint.h>
#include <stdlib.h>
#include "../private/bitmap.h"
#include "../private/context.h"

/** Allocate bitmap in a graphics context.
 *
 * @param gc Graphic context
 * @param params Bitmap parameters
 * @param alloc Bitmap allocation info or @c NULL to let GC allocate
 *               pixel storage
 * @param rbitmap Place to store pointer to new bitmap
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if grahic device connection
 *         was lost
 */
errno_t gfx_bitmap_create(gfx_context_t *gc, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, gfx_bitmap_t **rbitmap)
{
	return gc->ops->bitmap_create(gc->arg, params, alloc, rbitmap);
}

/** Delete bitmap from graphics context.
 *
 * @param bitmap Bitmap
 *
 * @return EOK on success, EIO if grahic device connection was lost
 */
errno_t gfx_bitmap_delete(gfx_bitmap_t *bitmap)
{
	return bitmap->gc->ops->bitmap_delete(bitmap->gc->arg, bitmap);
}

/** Render bitmap in graphics context.
 *
 * @param bitmap Bitmap
 * @param srect Source rectangle or @c NULL to render entire bitmap
 * @param offs Bitmap offset or @c NULL for zero offset
 *
 * @return EOK on success, EIO if grahic device connection was lost
 */
errno_t gfx_bitmap_render(gfx_bitmap_t *bitmap, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	return bitmap->gc->ops->bitmap_render(bitmap->gc->arg, bitmap,
	    srect, offs);
}

/** Get bitmap allocation info.
 *
 * @param bitmap Bitmap
 * @param alloc Allocation info structure to fill in
 *
 * @return EOK on success, EIO if grahic device connection was lost
 */
errno_t gfx_bitmap_get_alloc(gfx_bitmap_t *bitmap, gfx_bitmap_alloc_t *alloc)
{
	return bitmap->gc->ops->bitmap_get_alloc(bitmap->gc->arg, bitmap,
	    alloc);
}

/** @}
 */
