/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
