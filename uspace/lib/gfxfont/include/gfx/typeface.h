/*
 * Copyright (c) 2020 Jiri Svoboda
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
