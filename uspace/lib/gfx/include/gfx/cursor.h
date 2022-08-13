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

#ifndef _GFX_CURSOR_H
#define _GFX_CURSOR_H

#include <errno.h>
#include <types/gfx/coord.h>
#include <types/gfx/context.h>
#include <stdbool.h>

extern errno_t gfx_cursor_get_pos(gfx_context_t *, gfx_coord2_t *);
extern errno_t gfx_cursor_set_pos(gfx_context_t *, gfx_coord2_t *);
extern errno_t gfx_cursor_set_visible(gfx_context_t *, bool);

#endif

/** @}
 */
