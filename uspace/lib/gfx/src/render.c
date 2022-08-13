/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
