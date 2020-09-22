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
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/typeface.h>
#include <mem.h>
#include <riff/chunk.h>
#include <stdlib.h>
#include "../private/font.h"
#include "../private/glyph.h"
#include "../private/tpf_file.h"
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
	gfx_font_info_t *finfo;

	if (tface->riffr != NULL)
		(void) riff_rclose(tface->riffr);

	finfo = gfx_typeface_first_font(tface);
	while (finfo != NULL) {
		if (finfo->font != NULL)
			gfx_font_close(finfo->font);
		list_remove(&finfo->lfonts);
		free(finfo);

		finfo = gfx_typeface_first_font(tface);
	}

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

/** Open typeface from a TPF file.
 *
 * @param gc Graphic context
 * @param fname File name
 * @param rtface Place to store pointer to open typeface
 * @return EOK on success or an error code
 */
errno_t gfx_typeface_open(gfx_context_t *gc, const char *fname,
    gfx_typeface_t **rtface)
{
	riffr_t *riffr = NULL;
	gfx_typeface_t *tface = NULL;
	errno_t rc;
	riff_rchunk_t riffck;
	uint32_t format;

	rc = gfx_typeface_create(gc, &tface);
	if (rc != EOK)
		goto error;

	rc = riff_ropen(fname, &riffck, &riffr);
	if (rc != EOK)
		goto error;

	rc = riff_read_uint32(&riffck, &format);
	if (rc != EOK)
		goto error;

	if (format != FORM_TPFC) {
		rc = ENOTSUP;
		goto error;
	}

	while (true) {
		rc = gfx_font_info_load(tface, &riffck);
		if (rc == ENOENT)
			break;

		if (rc != EOK)
			goto error;
	}

	rc = riff_rchunk_end(&riffck);
	if (rc != EOK)
		goto error;

	tface->riffr = riffr;
	*rtface = tface;
	return EOK;
error:
	if (riffr != NULL)
		riff_rclose(riffr);
	if (tface != NULL)
		gfx_typeface_destroy(tface);
	return rc;
}

/** Make sure all typeface fonts are loaded.
 *
 * @param tface Typeface
 * @return EOK on success or an error code
 */
static errno_t gfx_typeface_loadin(gfx_typeface_t *tface)
{
	gfx_font_t *font;
	gfx_font_info_t *finfo;
	errno_t rc;

	finfo = gfx_typeface_first_font(tface);
	while (finfo != NULL) {
		/* Open font to make sure it is loaded */
		rc = gfx_font_open(finfo, &font);
		if (rc != EOK)
			return rc;

		/* Don't need this anymore */
		(void)font;

		finfo = gfx_typeface_next_font(finfo);
	}

	return EOK;
}

/** Save typeface into a TPF file.
 *
 * @param tface Typeface
 * @param fname Destination file name
 * @return EOK on success or an error code
 */
errno_t gfx_typeface_save(gfx_typeface_t *tface, const char *fname)
{
	riffw_t *riffw = NULL;
	errno_t rc;
	gfx_font_info_t *finfo;
	riff_wchunk_t riffck;

	/*
	 * Make sure all fonts are loaded before writing (in case
	 * we are writing into our original backing file).
	 */
	rc = gfx_typeface_loadin(tface);
	if (rc != EOK)
		return rc;

	rc = riff_wopen(fname, &riffw);
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_start(riffw, CKID_RIFF, &riffck);
	if (rc != EOK)
		goto error;

	rc = riff_write_uint32(riffw, FORM_TPFC);
	if (rc != EOK)
		goto error;

	finfo = gfx_typeface_first_font(tface);
	while (finfo != NULL) {
		/* Save font */
		rc = gfx_font_save(finfo, riffw);
		if (rc != EOK)
			goto error;

		finfo = gfx_typeface_next_font(finfo);
	}

	rc = riff_wchunk_end(riffw, &riffck);
	if (rc != EOK)
		goto error;

	rc = riff_wclose(riffw);
	if (rc != EOK)
		return rc;

	return EOK;
error:
	if (riffw != NULL)
		riff_wclose(riffw);
	return rc;
}

/** @}
 */
