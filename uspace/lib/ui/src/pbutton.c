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
 * @file Push button
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/pbutton.h>
#include "../private/pbutton.h"
#include "../private/resource.h"

/** Caption movement when button is pressed down */
enum {
	ui_pb_press_dx = 2,
	ui_pb_press_dy = 2
};

/** Create new push button.
 *
 * @param resource UI resource
 * @param caption Caption
 * @param rpbutton Place to store pointer to new push button
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_pbutton_create(ui_resource_t *resource, const char *caption,
    ui_pbutton_t **rpbutton)
{
	ui_pbutton_t *pbutton;

	pbutton = calloc(1, sizeof(ui_pbutton_t));
	if (pbutton == NULL)
		return ENOMEM;

	pbutton->caption = str_dup(caption);
	if (pbutton->caption == NULL) {
		free(pbutton);
		return ENOMEM;
	}

	pbutton->res = resource;
	*rpbutton = pbutton;
	return EOK;
}

/** Destroy push button.
 *
 * @param pbutton Push button or @c NULL
 */
void ui_pbutton_destroy(ui_pbutton_t *pbutton)
{
	if (pbutton == NULL)
		return;

	free(pbutton);
}

/** Set button rectangle.
 *
 * @param pbutton Button
 * @param rect New button rectanle
 */
void ui_pbutton_set_rect(ui_pbutton_t *pbutton, gfx_rect_t *rect)
{
	pbutton->rect = *rect;
}

/** Paint push button.
 *
 * @param pbutton Push button
 * @return EOK on success or an error code
 */
errno_t ui_pbutton_paint(ui_pbutton_t *pbutton)
{
	gfx_color_t *color = NULL;
	gfx_coord2_t pos;
	gfx_text_fmt_t fmt;
	errno_t rc;

	if (pbutton->held) {
		rc = gfx_color_new_rgb_i16(0x8000, 0, 0, &color);
		if (rc != EOK)
			goto error;
	} else {
		rc = gfx_color_new_rgb_i16(0xc8c8, 0xc8c8, 0xc8c8, &color);
		if (rc != EOK)
			goto error;
	}

	rc = gfx_set_color(pbutton->res->gc, color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(pbutton->res->gc, &pbutton->rect);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	rc = gfx_color_new_rgb_i16(0, 0, 0, &color);
	if (rc != EOK)
		goto error;

	rc = gfx_set_color(pbutton->res->gc, color);
	if (rc != EOK)
		goto error;

	/* Center of button rectangle */
	pos.x = (pbutton->rect.p0.x + pbutton->rect.p1.x) / 2;
	pos.y = (pbutton->rect.p0.y + pbutton->rect.p1.y) / 2;

	if (pbutton->held) {
		pos.x += ui_pb_press_dx;
		pos.y += ui_pb_press_dy;
	}

	gfx_text_fmt_init(&fmt);
	fmt.halign = gfx_halign_center;
	fmt.valign = gfx_valign_center;

	rc = gfx_puttext(pbutton->res->font, &pos, &fmt, pbutton->caption);
	if (rc != EOK)
		goto error;

	gfx_color_delete(color);

	return EOK;
error:
	if (color != NULL)
		gfx_color_delete(color);
	return rc;
}

/** Press down button.
 *
 * This does not automatically repaint the button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_press(ui_pbutton_t *pbutton)
{
	pbutton->held = true;
}

/** Release button.
 *
 * This does not automatically repaint the button.
 *
 * @param pbutton Push button
 */
void ui_pbutton_release(ui_pbutton_t *pbutton)
{
	pbutton->held = false;
}

/** @}
 */
