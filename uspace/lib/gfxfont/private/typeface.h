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
 * @file Typeface structure
 *
 */

#ifndef _GFX_PRIVATE_TYPEFACE_H
#define _GFX_PRIVATE_TYPEFACE_H

#include <adt/list.h>
#include <errno.h>
#include <riff/chunk.h>
#include <types/gfx/bitmap.h>
#include <types/gfx/context.h>
#include <types/gfx/typeface.h>

/** Typeface
 *
 * A typeface can contain several fonts of varying size and attributes
 * (such as bold, italic). Fonts present in a typeface can be enumerated
 * without actually loading them into memory.
 *
 * This is private to libgfxfont.
 */
struct gfx_typeface {
	/** Graphics context of the typeface */
	gfx_context_t *gc;
	/** Fonts (of gfx_font_info_t) */
	list_t fonts;
	/** RIFF reader of the open typeface file or @c NULL */
	riffr_t *riffr;
};

#endif

/** @}
 */
