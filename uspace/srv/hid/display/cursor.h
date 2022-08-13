/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server cursor
 */

#ifndef CURSOR_H
#define CURSOR_H

#include <errno.h>
#include <types/gfx/coord.h>
#include <stdint.h>
#include "types/display/display.h"
#include "types/display/cursor.h"

extern errno_t ds_cursor_create(ds_display_t *, gfx_rect_t *, const uint8_t *,
    ds_cursor_t **);
extern void ds_cursor_destroy(ds_cursor_t *);
extern errno_t ds_cursor_paint(ds_cursor_t *, gfx_coord2_t *, gfx_rect_t *);
extern void ds_cursor_get_rect(ds_cursor_t *, gfx_coord2_t *, gfx_rect_t *);

#endif

/** @}
 */
