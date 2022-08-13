/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphic coordinates
 */

#ifndef _GFX_TYPES_COORD_H
#define _GFX_TYPES_COORD_H

#include <errno.h>
#include <stdint.h>

typedef int gfx_coord_t;

typedef struct {
	gfx_coord_t x;
	gfx_coord_t y;
} gfx_coord2_t;

typedef struct {
	gfx_coord2_t p0;
	gfx_coord2_t p1;
} gfx_rect_t;

#endif

/** @}
 */
