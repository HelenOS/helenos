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

#ifndef _GFX_RENDER_H
#define _GFX_RENDER_H

#include <errno.h>
#include <types/gfx/color.h>
#include <types/gfx/coord.h>
#include <types/gfx/context.h>

extern errno_t gfx_set_clip_rect(gfx_context_t *, gfx_rect_t *);
extern errno_t gfx_set_color(gfx_context_t *, gfx_color_t *);
extern errno_t gfx_fill_rect(gfx_context_t *, gfx_rect_t *);
extern errno_t gfx_update(gfx_context_t *);

#endif

/** @}
 */
