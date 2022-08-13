/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Color operations
 */

#ifndef _GFX_COLOR_H
#define _GFX_COLOR_H

#include <errno.h>
#include <stdint.h>
#include <types/gfx/color.h>

extern errno_t gfx_color_new_rgb_i16(uint16_t, uint16_t,
    uint16_t, gfx_color_t **);
extern errno_t gfx_color_new_ega(uint8_t, gfx_color_t **);
extern void gfx_color_delete(gfx_color_t *);
extern void gfx_color_get_rgb_i16(gfx_color_t *, uint16_t *, uint16_t *,
    uint16_t *);
extern void gfx_color_get_ega(gfx_color_t *, uint8_t *);

#endif

/** @}
 */
