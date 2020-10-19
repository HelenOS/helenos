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
 * @file Label
 */

#include <errno.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/paint.h>
#include <ui/label.h>
#include "../private/label.h"
#include "../private/resource.h"

/** Create new label.
 *
 * @param resource UI resource
 * @param text Text
 * @param rlabel Place to store pointer to new label
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_label_create(ui_resource_t *resource, const char *text,
    ui_label_t **rlabel)
{
	ui_label_t *label;

	label = calloc(1, sizeof(ui_label_t));
	if (label == NULL)
		return ENOMEM;

	label->text = str_dup(text);
	if (label->text == NULL) {
		free(label);
		return ENOMEM;
	}

	label->res = resource;
	*rlabel = label;
	return EOK;
}

/** Destroy label.
 *
 * @param label Label or @c NULL
 */
void ui_label_destroy(ui_label_t *label)
{
	if (label == NULL)
		return;

	free(label);
}

/** Set label rectangle.
 *
 * @param label Label
 * @param rect New label rectangle
 */
void ui_label_set_rect(ui_label_t *label, gfx_rect_t *rect)
{
	label->rect = *rect;
}

/** Set label text.
 *
 * @param label Label
 * @param text New label text
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_label_set_text(ui_label_t *label, const char *text)
{
	char *tcopy;

	tcopy = str_dup(text);
	if (tcopy == NULL)
		return ENOMEM;

	free(label->text);
	label->text = tcopy;

	return EOK;
}

/** Paint label.
 *
 * @param label Label
 * @return EOK on success or an error code
 */
errno_t ui_label_paint(ui_label_t *label)
{
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	errno_t rc;

	/* Paint label background */

	rc = gfx_set_color(label->res->gc, label->res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(label->res->gc, &label->rect);
	if (rc != EOK)
		goto error;

	pos = label->rect.p0;

	gfx_text_fmt_init(&fmt);
	fmt.halign = gfx_halign_left;
	fmt.valign = gfx_valign_top;

	rc = gfx_set_color(label->res->gc, label->res->wnd_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_puttext(label->res->font, &pos, &fmt, label->text);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** @}
 */
