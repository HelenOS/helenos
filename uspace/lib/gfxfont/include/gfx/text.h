/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Text formatting
 */

#ifndef _GFX_TEXT_H
#define _GFX_TEXT_H

#include <errno.h>
#include <stddef.h>
#include <types/gfx/coord.h>
#include <types/gfx/font.h>
#include <types/gfx/text.h>

extern void gfx_text_fmt_init(gfx_text_fmt_t *);
extern gfx_coord_t gfx_text_width(gfx_font_t *, const char *);
extern errno_t gfx_puttext(gfx_coord2_t *, gfx_text_fmt_t *, const char *);
extern void gfx_text_start_pos(gfx_coord2_t *, gfx_text_fmt_t *, const char *,
    gfx_coord2_t *);
extern size_t gfx_text_find_pos(gfx_coord2_t *, gfx_text_fmt_t *, const char *,
    gfx_coord2_t *);
extern void gfx_text_cont(gfx_coord2_t *, gfx_text_fmt_t *, const char *,
    gfx_coord2_t *, gfx_text_fmt_t *);
extern void gfx_text_rect(gfx_coord2_t *, gfx_text_fmt_t *, const char *,
    gfx_rect_t *);

#endif

/** @}
 */
