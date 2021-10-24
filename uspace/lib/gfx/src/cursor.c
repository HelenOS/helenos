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
