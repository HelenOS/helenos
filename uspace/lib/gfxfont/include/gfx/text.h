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
