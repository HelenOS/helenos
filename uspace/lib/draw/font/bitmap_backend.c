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

#include <errno.h>
#include <stdlib.h>

#include "../font.h"
#include "../drawctx.h"
#include "bitmap_backend.h"

typedef struct {
	surface_t *surface;
	glyph_metrics_t metrics;
	bool metrics_loaded;
} glyph_cache_item_t;

typedef struct {
	uint16_t points;
	uint32_t glyph_count;
	font_metrics_t font_metrics;
	glyph_cache_item_t *glyph_cache;
	bitmap_font_decoder_t *decoder;
	void *decoder_data;
	bool scale;
	double scale_ratio;
} bitmap_backend_data_t;

static errno_t bb_get_font_metrics(void *backend_data, font_metrics_t *font_metrics)
{
	bitmap_backend_data_t *data = (bitmap_backend_data_t *) backend_data;

	*font_metrics = data->font_metrics;

	return EOK;
}

static errno_t bb_resolve_glyph(void *backend_data, wchar_t c, glyph_id_t *glyph_id)
{
	bitmap_backend_data_t *data = (bitmap_backend_data_t *) backend_data;
	return data->decoder->resolve_glyph(data->decoder_data, c, glyph_id);
}

static errno_t bb_get_glyph_metrics(void *backend_data, glyph_id_t glyph_id,
    glyph_metrics_t *glyph_metrics)
{
	bitmap_backend_data_t *data = (bitmap_backend_data_t *) backend_data;

	if (glyph_id >= data->glyph_count)
		return ENOENT;

	if (data->glyph_cache[glyph_id].metrics_loaded) {
		*glyph_metrics = data->glyph_cache[glyph_id].metrics;
		return EOK;
	}

	glyph_metrics_t gm;

	errno_t rc = data->decoder->load_glyph_metrics(data->decoder_data, glyph_id,
	    &gm);
	if (rc != EOK)
		return rc;

	if (data->scale) {
		gm.left_side_bearing = (metric_t)
		    (data->scale_ratio * gm.left_side_bearing + 0.5);
		gm.width = (metric_t)
		    (data->scale_ratio * gm.width + 0.5);
		gm.right_side_bearing = (metric_t)
		    (data->scale_ratio * gm.right_side_bearing + 0.5);
		gm.ascender = (metric_t)
		    (data->scale_ratio * gm.ascender + 0.5);
		gm.height = (metric_t)
		    (data->scale_ratio * gm.height + 0.5);
	}



	data->glyph_cache[glyph_id].metrics = gm;
	data->glyph_cache[glyph_id].metrics_loaded = true;
	*glyph_metrics = gm;
	return EOK;
}

static errno_t get_glyph_surface(bitmap_backend_data_t *data, glyph_id_t glyph_id,
    surface_t **result)
{
	if (glyph_id >= data->glyph_count)
		return ENOENT;

	if (data->glyph_cache[glyph_id].surface != NULL) {
		*result = data->glyph_cache[glyph_id].surface;
		return EOK;
	}

	surface_t *raw_surface;
	errno_t rc = data->decoder->load_glyph_surface(data->decoder_data, glyph_id,
	    &raw_surface);
	if (rc != EOK)
		return rc;

	sysarg_t w;
	sysarg_t h;
	surface_get_resolution(raw_surface, &w, &h);

	if (!data->scale) {
		*result = raw_surface;
		return EOK;
	}

	source_t source;
	source_init(&source);
	source_set_texture(&source, raw_surface, PIXELMAP_EXTEND_TRANSPARENT_BLACK);

	transform_t transform;
	transform_identity(&transform);
	transform_translate(&transform, 0.5, 0.5);
	transform_scale(&transform, data->scale_ratio, data->scale_ratio);
	source_set_transform(&source, transform);

	surface_coord_t scaled_width = (data->scale_ratio * ((double) w) + 0.5);
	surface_coord_t scaled_height = (data->scale_ratio * ((double) h) + 0.5);

	surface_t *scaled_surface = surface_create(scaled_width, scaled_height,
	    NULL, 0);
	if (!scaled_surface) {
		surface_destroy(raw_surface);
		return ENOMEM;
	}

	drawctx_t context;
	drawctx_init(&context, scaled_surface);
	drawctx_set_source(&context, &source);
	drawctx_transfer(&context, 0, 0, scaled_width, scaled_height);

	surface_destroy(raw_surface);

	data->glyph_cache[glyph_id].surface = scaled_surface;
	*result = scaled_surface;
	return EOK;
}

static errno_t bb_render_glyph(void *backend_data, drawctx_t *context,
    source_t *source, sysarg_t ox, sysarg_t oy, glyph_id_t glyph_id)
{
	bitmap_backend_data_t *data = (bitmap_backend_data_t *) backend_data;

	glyph_metrics_t glyph_metrics;
	errno_t rc = bb_get_glyph_metrics(backend_data, glyph_id, &glyph_metrics);
	if (rc != EOK)
		return rc;

	surface_t *glyph_surface;
	rc = get_glyph_surface(data, glyph_id, &glyph_surface);
	if (rc != EOK)
		return rc;

	native_t x = ox + glyph_metrics.left_side_bearing;
	native_t y = oy - glyph_metrics.ascender;

	transform_t transform;
	transform_identity(&transform);
	transform_translate(&transform, x, y);
	source_set_transform(source, transform);
	source_set_mask(source, glyph_surface, false);
	drawctx_transfer(context, x, y, glyph_metrics.width,
	    glyph_metrics.height);

	return EOK;
}

static void bb_release(void *backend_data)
{
	bitmap_backend_data_t *data = (bitmap_backend_data_t *) backend_data;

	for (size_t i = 0; i < data->glyph_count; ++i) {
		if (data->glyph_cache[i].surface) {
			surface_destroy(data->glyph_cache[i].surface);
		}
	}
	free(data->glyph_cache);

	data->decoder->release(data->decoder_data);
}

font_backend_t bitmap_backend = {
	.get_font_metrics = bb_get_font_metrics,
	.resolve_glyph = bb_resolve_glyph,
	.get_glyph_metrics = bb_get_glyph_metrics,
	.render_glyph = bb_render_glyph,
	.release = bb_release
};

errno_t bitmap_font_create(bitmap_font_decoder_t *decoder, void *decoder_data,
    uint32_t glyph_count, font_metrics_t font_metrics, uint16_t points,
    font_t **out_font)
{
	if (glyph_count == 0)
		return EINVAL;

	bitmap_backend_data_t *data = malloc(sizeof(bitmap_backend_data_t));
	if (data == NULL)
		return ENOMEM;

	data->glyph_count = glyph_count;
	data->points = points;
	data->decoder = decoder;
	data->decoder_data = decoder_data;
	data->font_metrics = font_metrics;
	metric_t line_height = (font_metrics.ascender + font_metrics.descender);
	if (points == line_height) {
		data->scale = false;
		data->scale_ratio = 1.0;
	}
	else {
		data->scale = true;
		data->scale_ratio = ((double) points) / ((double) line_height);
		line_height = (data->scale_ratio * ((double) line_height));
		data->font_metrics.ascender = (metric_t)
		    (data->scale_ratio * data->font_metrics.ascender + 0.5);
		data->font_metrics.descender =
		    line_height - data->font_metrics.ascender;
		data->font_metrics.leading = (metric_t)
		    (data->scale_ratio * data->font_metrics.leading + 0.5);
	}

	data->glyph_cache = calloc(data->glyph_count,
	    sizeof(glyph_cache_item_t));
	if (data->glyph_cache == NULL) {
		free(data);
		return ENOMEM;
	}

	for (size_t i = 0; i < data->glyph_count; ++i) {
		data->glyph_cache[i].surface = NULL;
		data->glyph_cache[i].metrics_loaded = false;
	}

	font_t *font = font_create(&bitmap_backend, data);
	if (font == NULL) {
		free(data->glyph_cache);
		free(data);
		return ENOMEM;
	}

	*out_font = font;
	return EOK;
}

/** @}
 */
