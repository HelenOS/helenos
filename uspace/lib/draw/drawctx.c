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

#include <assert.h>
#include <adt/list.h>
#include <stdlib.h>

#include "drawctx.h"

void drawctx_init(drawctx_t *context, surface_t *surface)
{
	assert(surface);
	list_initialize(&context->list);
	context->surface = surface;
	context->compose = compose_src;
	context->mask = NULL;
	context->source = NULL;
	context->shall_clip = false;
	context->clip_x = 0;
	context->clip_y = 0;
	surface_get_resolution(surface, &context->clip_width, &context->clip_height);
}

void drawctx_save(drawctx_t *context)
{
	drawctx_t *to_save = (drawctx_t *) malloc(sizeof(drawctx_t));
	if (to_save) {
		link_initialize(&to_save->link);
		to_save->compose = context->compose;
		to_save->mask = context->mask;
		to_save->source = context->source;
		to_save->shall_clip = context->shall_clip;
		to_save->clip_x = context->clip_x;
		to_save->clip_y = context->clip_y;
		to_save->clip_width = context->clip_width;
		to_save->clip_height = context->clip_height;
		list_append(&to_save->link, &context->list);
	}
}

void drawctx_restore(drawctx_t *context)
{
	drawctx_t *to_load = (drawctx_t *) list_last(&context->list);
	if (to_load) {
		list_remove(&to_load->link);
		context->compose = to_load->compose;
		context->mask = to_load->mask;
		context->source = to_load->source;
		context->shall_clip = to_load->shall_clip;
		context->clip_x = to_load->clip_x;
		context->clip_y = to_load->clip_y;
		context->clip_width = to_load->clip_width;
		context->clip_height = to_load->clip_height;
		free(to_load);
	}
}

void drawctx_set_compose(drawctx_t *context, compose_t compose)
{
	context->compose = compose;
}

void drawctx_set_clip(drawctx_t *context,
    sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height)
{
	surface_get_resolution(context->surface,
	    &context->clip_width, &context->clip_height);
	context->shall_clip = (x > 0) || (y > 0) ||
	    (width < context->clip_width) || (height < context->clip_height);

	context->clip_x = x;
	context->clip_y = y;
	context->clip_width = width;
	context->clip_height = height;
}

void drawctx_set_mask(drawctx_t *context, surface_t *mask)
{
	context->mask = mask;
}

void drawctx_set_source(drawctx_t *context, source_t *source)
{
	context->source = source;
}

void drawctx_set_font(drawctx_t *context, font_t *font)
{
	context->font = font;
}

void drawctx_transfer(drawctx_t *context,
    sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height)
{
	if (!context->source) {
		return;
	}

	bool transfer_fast = source_is_fast(context->source) &&
	    (context->shall_clip == false) &&
	    (context->mask == NULL) &&
	    (context->compose == compose_src || context->compose == compose_over);

	if (transfer_fast) {

		for (sysarg_t _y = y; _y < y + height; ++_y) {
			pixel_t *src = source_direct_access(context->source, x, _y);
			pixel_t *dst = pixelmap_pixel_at(surface_pixmap_access(context->surface), x, _y);
			if (src && dst) {
				sysarg_t count = width;
				while (count-- != 0) {
					*dst++ = *src++;
				}
			}
		}
		surface_add_damaged_region(context->surface, x, y, width, height);

	} else {

		bool clipped = false;
		bool masked = false;
		for (sysarg_t _y = y; _y < y + height; ++_y) {
			for (sysarg_t _x = x; _x < x + width; ++_x) {
				if (context->shall_clip) {
					clipped = _x < context->clip_x && _x >= context->clip_width &&
					    _y < context->clip_y && _y >= context->clip_height;
				}

				if (context->mask) {
					pixel_t p = surface_get_pixel(context->mask, _x, _y);
					masked = p > 0 ? false : true;
				}

				if (!clipped && !masked) {
					pixel_t p_src = source_determine_pixel(context->source, _x, _y);
					pixel_t p_dst = surface_get_pixel(context->surface, _x, _y);
					pixel_t p_res = context->compose(p_src, p_dst);
					surface_put_pixel(context->surface, _x, _y, p_res);
				}
			}
		}

	}
}

void drawctx_stroke(drawctx_t *context, path_t *path)
{
	if (!context->source || !path) {
		return;
	}

	/* Note:
	 * Antialiasing could be achieved by up-scaling path coordinates and
	 * rendering into temporary higher-resolution surface. Then, the temporary
	 * surface would be set as a source and its damaged region would be
	 * transferred to the original surface. */

	list_foreach(*((list_t *) path), link, path_step_t, step) {
		switch (step->type) {
		case PATH_STEP_MOVETO:
			// TODO
			break;
		case PATH_STEP_LINETO:
			// TODO
			break;
		default:
			break;
		}
	}
}

void drawctx_fill(drawctx_t *context, path_t *path)
{
	if (!context->source || !path) {
		return;
	}

	// TODO
}

void drawctx_print(drawctx_t *context, const char *text, sysarg_t x, sysarg_t y)
{
	if (context->font && context->source) {
		font_draw_text(context->font, context, context->source, text, x, y);
	}
}

/** @}
 */
