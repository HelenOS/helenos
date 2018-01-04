/*
 * Copyright (c) 2012 Petr Koupy
 * Copyright (c) 2014 Martin Sucha
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#ifndef DRAW_FONT_H_
#define DRAW_FONT_H_

#include <stdint.h>

#include "surface.h"
#include "source.h"

struct drawctx;
typedef struct drawctx drawctx_t;

typedef int metric_t;

typedef struct {
	/* Horizontal distance between origin and left side of the glyph */ 
	metric_t left_side_bearing;
	
	/* Width of the actual glyph drawn */
	metric_t width;
	
	/* Horizontal distance between right side of the glyph and origin
	   of the next glyph */
	metric_t right_side_bearing;
	
	/* Vertical distance between baseline and top of the glyph
	   (positive to top) */
	metric_t ascender;
	
	/* Height of the actual glyph drawn */
	metric_t height;
} glyph_metrics_t;

static inline metric_t glyph_metrics_get_descender(glyph_metrics_t *gm)
{
	return gm->height - gm->ascender;
}

static inline metric_t glyph_metrics_get_advancement(glyph_metrics_t *gm)
{
	return gm->left_side_bearing + gm->width + gm->right_side_bearing;
}

typedef struct {
	/* Distance between top of the line and baseline */
	metric_t ascender;
	
	/* Distance between baseline and bottom of the line */
	metric_t descender;
	
	/* Distance between bottom of the line and top of the next line */
	metric_t leading;
} font_metrics_t;

typedef uint32_t glyph_id_t;

typedef struct {
	errno_t (*get_font_metrics)(void *, font_metrics_t *);
	errno_t (*resolve_glyph)(void *, wchar_t, glyph_id_t *);
	errno_t (*get_glyph_metrics)(void *, glyph_id_t, glyph_metrics_t *);
	errno_t (*render_glyph)(void *, drawctx_t *, source_t *, sysarg_t,
	    sysarg_t, glyph_id_t);
	void (*release)(void *);
} font_backend_t;

typedef struct {
	font_backend_t *backend;
	void *backend_data;
} font_t;

extern font_t *font_create(font_backend_t *, void *);
extern errno_t font_get_metrics(font_t *, font_metrics_t *);
extern errno_t font_resolve_glyph(font_t *, wchar_t, glyph_id_t *);
extern errno_t font_get_glyph_metrics(font_t *, glyph_id_t, glyph_metrics_t *);
extern errno_t font_render_glyph(font_t *, drawctx_t *, source_t *,
    sysarg_t, sysarg_t, glyph_id_t);
extern void font_release(font_t *);

extern errno_t font_get_box(font_t *, char *, sysarg_t *, sysarg_t *);
extern errno_t font_draw_text(font_t *, drawctx_t *, source_t *, const char *,
    sysarg_t, sysarg_t);

#endif

/** @}
 */
