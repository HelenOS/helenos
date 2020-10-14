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

/** @addtogroup libui
 * @{
 */
/**
 * @file UI resources
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/font.h>
#include <gfx/render.h>
#include <gfx/typeface.h>
#include <stdlib.h>
#include <str.h>
#include <ui/resource.h>
#include "../private/resource.h"

static const char *ui_typeface_path = "/data/font/helena.tpf";

/** Create new UI resource.
 *
 * @param gc Graphic context
 * @param rresource Place to store pointer to new push button
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_resource_create(gfx_context_t *gc, ui_resource_t **rresource)
{
	ui_resource_t *resource;
	gfx_typeface_t *tface = NULL;
	gfx_font_t *font = NULL;
	gfx_font_info_t *finfo;
	errno_t rc;

	resource = calloc(1, sizeof(ui_resource_t));
	if (resource == NULL)
		return ENOMEM;

	rc = gfx_typeface_open(gc, ui_typeface_path, &tface);
	if (rc != EOK)
		goto error;

	finfo = gfx_typeface_first_font(tface);
	if (finfo == NULL) {
		rc = EIO;
		goto error;
	}

	rc = gfx_font_open(finfo, &font);
	if (rc != EOK)
		goto error;

	resource->gc = gc;
	resource->tface = tface;
	resource->font = font;
	*rresource = resource;
	return EOK;
error:
	if (tface != NULL)
		gfx_typeface_destroy(tface);
	free(resource);
	return rc;
}

/** Destroy UI resource.
 *
 * @param resource UI resource or @c NULL
 */
void ui_resource_destroy(ui_resource_t *resource)
{
	if (resource == NULL)
		return;

	gfx_font_close(resource->font);
	gfx_typeface_destroy(resource->tface);
	free(resource);
}

/** @}
 */
