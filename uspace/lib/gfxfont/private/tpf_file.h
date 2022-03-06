/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file TPF file definitions
 *
 */

#ifndef _GFX_PRIVATE_TPF_FILE_H
#define _GFX_PRIVATE_TPF_FILE_H

#include <stdint.h>

enum {
	/** Typeface RIFF format ID */
	FORM_TPFC = 0x43465054,

	/** Font list type */
	LTYPE_font = 0x746e6f66,

	/** Font properties chunk ID */
	CKID_fprp = 0x70727066,
	/** Font metrics chunk ID */
	CKID_fmtr = 0x72746d66,
	/** Font bitmap chunk ID */
	CKID_fbmp = 0x706d6266,

	/** Glyph list type */
	LTYPE_glph = 0x68706c67,

	/** Glyph metrics chunk ID */
	CKID_gmtr = 0x72746d67,
	/** Glyph patterns chunk ID */
	CKID_gpat = 0x74617067,
	/** Glyph rectangle/origin chunk ID */
	CKID_gror = 0x726f7267
};

/** TPF font properties */
typedef struct {
	uint16_t size;
	uint16_t flags;
} tpf_font_props_t;

/** TPF font metrics */
typedef struct {
	uint16_t ascent;
	uint16_t descent;
	uint16_t leading;
	int16_t underline_y0;
	int16_t underline_y1;
} tpf_font_metrics_t;

/** TPF glyph metrics */
typedef struct {
	uint16_t advance;
} tpf_glyph_metrics_t;

/** TPF glyph rectangle/origin */
typedef struct {
	/** Rectangle p0.x */
	uint32_t p0x;
	/** Rectangle p0.y */
	uint32_t p0y;
	/** Rectangle p1.x */
	uint32_t p1x;
	/** Rectangle p1.y */
	uint32_t p1y;
	/** Origin X */
	uint32_t orig_x;
	/** Origin Y */
	uint32_t orig_y;
} tpf_glyph_ror_t;

/** TPF font bitmap header */
typedef struct {
	/** Width in pixels */
	uint32_t width;
	/** Height in pixels */
	uint32_t height;
	/** Format (0) */
	uint16_t fmt;
	/** Depth (bits/pixel) */
	uint16_t depth;
} tpf_font_bmp_hdr_t;

#endif

/** @}
 */
