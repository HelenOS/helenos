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
 * @file Glyph
 */

#include <assert.h>
#include <errno.h>
#include <gfx/glyph.h>
#include <mem.h>
#include <stdlib.h>
#include "../private/glyph.h"

/** Initialize glyph metrics structure.
 *
 * Glyph metrics structure must always be initialized using this function
 * first.
 *
 * @param metrics Glyph metrics structure
 */
void gfx_glyph_metrics_init(gfx_glyph_metrics_t *metrics)
{
	memset(metrics, 0, sizeof(gfx_glyph_metrics_t));
}

/** Create glyph.
 *
 * @param font Containing font
 * @param metrics Glyph metrics
 * @param rglyph Place to store pointer to new glyph
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_glyph_create(gfx_font_t *font, gfx_glyph_metrics_t *metrics,
    gfx_glyph_t **rglyph)
{
	gfx_glyph_t *glyph;
	errno_t rc;

	glyph = calloc(1, sizeof(gfx_glyph_t));
	if (glyph == NULL)
		return ENOMEM;

	glyph->font = font;

	rc = gfx_glyph_set_metrics(glyph, metrics);
	if (rc != EOK) {
		assert(rc == EINVAL);
		free(glyph);
		return rc;
	}

	glyph->metrics = *metrics;
	*rglyph = glyph;
	return EOK;
}

/** Destroy glyph.
 *
 * @param glyph Glyph
 */
void gfx_glyph_destroy(gfx_glyph_t *glyph)
{
	free(glyph);
}

/** Get glyph metrics.
 *
 * @param glyph Glyph
 * @param metrics Place to store metrics
 */
void gfx_glyph_get_metrics(gfx_glyph_t *glyph, gfx_glyph_metrics_t *metrics)
{
	*metrics = glyph->metrics;
}

/** Set glyph metrics.
 *
 * @param glyph Glyph
 * @param metrics Place to store metrics
 * @return EOK on success, EINVAL if supplied metrics are invalid
 */
errno_t gfx_glyph_set_metrics(gfx_glyph_t *glyph, gfx_glyph_metrics_t *metrics)
{
	glyph->metrics = *metrics;
	return EOK;
}

/** Set a pattern that the glyph will match.
 *
 * A glyph can match any number of patterns. Setting the same pattern
 * again has no effect. The pattern is a simple (sub)string. Matching
 * is done using maximum munch rule.
 *
 * @param glyph Glyph
 * @param pattern Pattern
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_set_pattern(gfx_glyph_t *glyph, const char *pattern)
{
	return EOK;
}

/** Clear a matching pattern from a glyph.
 *
 * Clearing a pattern that is not set has no effect.
 *
 * @param Glyph
 * @param pattern Pattern
 */
void gfx_glyph_clear_pattern(gfx_glyph_t *glyph, const char *pattern)
{
}

/** Open glyph bitmap for editing.
 *
 * @param glyph Glyph
 * @param rbmp Place to store glyph bitmap
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_open_bmp(gfx_glyph_t *glyph, gfx_glyph_bmp_t **rbmp)
{
	return EOK;
}

/** @}
 */
