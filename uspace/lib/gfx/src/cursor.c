/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Hardware cursor control
 */

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <gfx/cursor.h>
#include <gfx/render.h>
#include <stdbool.h>
#include "../private/context.h"

/** Get hardware cursor position.
 *
 * @param gc Graphic context
 * @param pos Place to store cursor position
 *
 * @return EOK on success, ENOTSUP if not supported,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_cursor_get_pos(gfx_context_t *gc, gfx_coord2_t *pos)
{
	if (gc->ops->cursor_get_pos != NULL)
		return gc->ops->cursor_get_pos(gc->arg, pos);
	else
		return ENOTSUP;
}

/** Set hardware cursor position.
 *
 * @param gc Graphic context
 * @param pos New cursor position
 *
 * @return EOK on success, ENOTSUP if not supported,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_cursor_set_pos(gfx_context_t *gc, gfx_coord2_t *pos)
{
	if (gc->ops->cursor_set_pos != NULL)
		return gc->ops->cursor_set_pos(gc->arg, pos);
	else
		return ENOTSUP;
}

/** Set hardware cursor visibility.
 *
 * @param gc Graphic context
 * @param visible @c true iff cursor should be made visible
 *
 * @return EOK on success, ENOTSUP if not supported,
 *         EIO if grahic device connection was lost
 */
errno_t gfx_cursor_set_visible(gfx_context_t *gc, bool visible)
{
	if (gc->ops->cursor_set_visible != NULL)
		return gc->ops->cursor_set_visible(gc->arg, visible);
	else
		return ENOTSUP;
}

/** @}
 */
