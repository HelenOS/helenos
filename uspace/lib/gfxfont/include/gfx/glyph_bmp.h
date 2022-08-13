/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph bitmap
 */

#ifndef _GFX_GLYPH_BMP_H
#define _GFX_GLYPH_BMP_H

#include <errno.h>
#include <gfx/coord.h>
#include <types/gfx/glyph.h>
#include <types/gfx/glyph_bmp.h>

extern errno_t gfx_glyph_bmp_open(gfx_glyph_t *, gfx_glyph_bmp_t **);
extern errno_t gfx_glyph_bmp_save(gfx_glyph_bmp_t *);
extern void gfx_glyph_bmp_close(gfx_glyph_bmp_t *);
extern void gfx_glyph_bmp_get_rect(gfx_glyph_bmp_t *, gfx_rect_t *);
extern int gfx_glyph_bmp_getpix(gfx_glyph_bmp_t *, gfx_coord_t, gfx_coord_t);
extern errno_t gfx_glyph_bmp_setpix(gfx_glyph_bmp_t *, gfx_coord_t,
    gfx_coord_t, int);
extern errno_t gfx_glyph_bmp_clear(gfx_glyph_bmp_t *);

#endif

/** @}
 */
