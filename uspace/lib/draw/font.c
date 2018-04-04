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
#include <str.h>

#include "font.h"
#include "font/embedded.h"
#include "drawctx.h"

font_t *font_create(font_backend_t *backend, void *backend_data)
{
	font_t *font = malloc(sizeof(font_t));
	if (font == NULL)
		return NULL;

	font->backend = backend;
	font->backend_data = backend_data;

	return font;
}

void font_release(font_t *font)
{
	font->backend->release(font->backend_data);
}

errno_t font_get_metrics(font_t *font, font_metrics_t *metrics)
{
	return font->backend->get_font_metrics(font->backend_data, metrics);
}

errno_t font_resolve_glyph(font_t *font, wchar_t c, glyph_id_t *glyph_id)
{
	return font->backend->resolve_glyph(font->backend_data, c, glyph_id);
}

errno_t font_get_glyph_metrics(font_t *font, glyph_id_t glyph_id,
    glyph_metrics_t *glyph_metrics)
{
	return font->backend->get_glyph_metrics(font->backend_data,
	    glyph_id, glyph_metrics);
}

errno_t font_render_glyph(font_t *font, drawctx_t *context, source_t *source,
    sysarg_t x, sysarg_t y, glyph_id_t glyph_id)
{
	return font->backend->render_glyph(font->backend_data, context, source,
	    x, y, glyph_id);
}

/* TODO this is bad interface */
errno_t font_get_box(font_t *font, char *text, sysarg_t *width, sysarg_t *height)
{
	font_metrics_t fm;
	errno_t rc = font_get_metrics(font, &fm);
	if (rc != EOK)
		return rc;

	native_t x = 0;

	size_t off = 0;
	while (true) {
		wchar_t c = str_decode(text, &off, STR_NO_LIMIT);
		if (c == 0)
			break;

		glyph_id_t glyph_id;
		rc = font_resolve_glyph(font, c, &glyph_id);
		if (rc != EOK) {
			errno_t rc2 = font_resolve_glyph(font, U_SPECIAL, &glyph_id);
			if (rc2 != EOK) {
				return rc;
			}
		}

		glyph_metrics_t glyph_metrics;
		rc = font_get_glyph_metrics(font, glyph_id, &glyph_metrics);
		if (rc != EOK)
			return rc;

		x += glyph_metrics_get_advancement(&glyph_metrics);
	}

	*width = x;
	*height = fm.ascender + fm.descender;
	return EOK;
}

/* TODO this is bad interface */
errno_t font_draw_text(font_t *font, drawctx_t *context, source_t *source,
    const char *text, sysarg_t sx, sysarg_t sy)
{
	drawctx_save(context);
	drawctx_set_compose(context, compose_over);

	font_metrics_t fm;
	errno_t rc = font_get_metrics(font, &fm);
	if (rc != EOK)
		return rc;

	native_t baseline = sy + fm.ascender;
	native_t x = sx;

	size_t off = 0;
	while (true) {
		wchar_t c = str_decode(text, &off, STR_NO_LIMIT);
		if (c == 0)
			break;

		glyph_id_t glyph_id;
		rc = font_resolve_glyph(font, c, &glyph_id);
		if (rc != EOK) {
			errno_t rc2 = font_resolve_glyph(font, U_SPECIAL, &glyph_id);
			if (rc2 != EOK) {
				return rc;
			}
		}

		glyph_metrics_t glyph_metrics;
		rc = font_get_glyph_metrics(font, glyph_id, &glyph_metrics);
		if (rc != EOK)
			return rc;

		rc = font_render_glyph(font, context, source, x, baseline,
		    glyph_id);
		if (rc != EOK)
			return rc;

		x += glyph_metrics_get_advancement(&glyph_metrics);

	}

	drawctx_restore(context);
	source_set_mask(source, NULL, false);

	return EOK;
}

/** @}
 */
