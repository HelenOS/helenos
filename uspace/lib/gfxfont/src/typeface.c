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

#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/glyph.h>
#include <gfx/typeface.h>
#include <mem.h>
#include <stdlib.h>
#include "../private/font.h"
#include "../private/glyph.h"
#include "../private/typeface.h"

/** Create typeface in graphics context.
 *
 * @param gc Graphic context
 * @param rtface Place to store pointer to new typeface
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_typeface_create(gfx_context_t *gc, gfx_typeface_t **rtface)
{
	gfx_typeface_t *tface;

	tface = calloc(1, sizeof(gfx_typeface_t));
	if (tface == NULL)
		return ENOMEM;

	tface->gc = gc;
	list_initialize(&tface->fonts);

	*rtface = tface;
	return EOK;
}

/** Destroy typeface.
 *
 * @param tface Typeface
 */
void gfx_typeface_destroy(gfx_typeface_t *tface)
{
	free(tface);
}

/** Get info on first font in typeface.
 *
 * @param tface Typeface
 * @return First font info or @c NULL if there are none
 */
gfx_font_info_t *gfx_typeface_first_font(gfx_typeface_t *tface)
{
	link_t *link;

	link = list_first(&tface->fonts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_font_info_t, lfonts);
}

/** Get info on next font in typeface.
 *
 * @param cur Current font info
 * @return Next font info in font or @c NULL if @a cur was the last one
 */
gfx_font_info_t *gfx_typeface_next_font(gfx_font_info_t *cur)
{
	link_t *link;

	link = list_next(&cur->lfonts, &cur->typeface->fonts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_font_info_t, lfonts);
}

/** @}
 */
