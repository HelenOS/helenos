/*
 * Copyright (c) 2012 Petr Koupy
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

#ifndef DRAW_SOURCE_H_
#define DRAW_SOURCE_H_

#include <sys/types.h>
#include <stdbool.h>

#include <transform.h>
#include <filter.h>
#include <io/pixelmap.h>

#include "surface.h"

typedef struct source {
	transform_t transform;
	filter_t filter;

	pixel_t color;
	surface_t *texture;
	pixelmap_extend_t texture_extend;

	pixel_t alpha;
	surface_t *mask;
	pixelmap_extend_t mask_extend;
} source_t;

extern void source_init(source_t *);

extern void source_set_transform(source_t *, transform_t);
extern void source_reset_transform(source_t *);

extern void source_set_filter(source_t *, filter_t);

extern void source_set_color(source_t *, pixel_t);
extern void source_set_texture(source_t *, surface_t *, pixelmap_extend_t);

extern void source_set_alpha(source_t *, pixel_t);
extern void source_set_mask(source_t *, surface_t *, pixelmap_extend_t);

extern bool source_is_fast(source_t *);
extern pixel_t *source_direct_access(source_t *, double, double);
extern pixel_t source_determine_pixel(source_t *, double, double);

#endif

/** @}
 */
