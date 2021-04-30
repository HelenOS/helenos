/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file Rendering operations
 */

#include <gfx/render.h>
#include "../private/context.h"

/** Set clipping rectangle.
 *
 * @param gc Graphic context
 * @param rect Rectangle or @c NULL (no extra clipping)
 *
 * @return EOK on success, ENOMEM if insufficient resources,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_set_clip_rect(gfx_context_t *gc, gfx_rect_t *rect)
{
	return gc->ops->set_clip_rect(gc->arg, rect);
}

/** Set drawing color.
 *
 * @param gc Graphic context
 * @param color Color
 *
 * @return EOK on success, ENOMEM if insufficient resources,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_set_color(gfx_context_t *gc, gfx_color_t *color)
{
	return gc->ops->set_color(gc->arg, color);
}

/** Fill rectangle using the current drawing color.
 *
 * @param gc Graphic context
 * @param rect Rectangle
 *
 * @return EOK on success, ENOMEM if insufficient resources,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_fill_rect(gfx_context_t *gc, gfx_rect_t *rect)
{
	return gc->ops->fill_rect(gc->arg, rect);
}

/** Update display.
 *
 * Finish any deferred rendering.
 *
 * @param gc Graphic context
 *
 * @return EOK on success, ENOMEM if insufficient resources,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_update(gfx_context_t *gc)
{
	return gc->ops->update(gc->arg);
}

/** @}
 */
