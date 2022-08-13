/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Typeface
 */

#ifndef _GFX_TYPEFACE_H
#define _GFX_TYPEFACE_H

#include <errno.h>
#include <stddef.h>
#include <types/gfx/context.h>
#include <types/gfx/font.h>
#include <types/gfx/glyph.h>
#include <types/gfx/typeface.h>

extern errno_t gfx_typeface_create(gfx_context_t *, gfx_typeface_t **);
extern void gfx_typeface_destroy(gfx_typeface_t *);
extern gfx_font_info_t *gfx_typeface_first_font(gfx_typeface_t *);
extern gfx_font_info_t *gfx_typeface_next_font(gfx_font_info_t *);
extern errno_t gfx_typeface_open(gfx_context_t *, const char *,
    gfx_typeface_t **);
extern errno_t gfx_typeface_save(gfx_typeface_t *, const char *);

#endif

/** @}
 */
