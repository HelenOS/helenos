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
 * @file Window decoration
 */

#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/paint.h>
#include <ui/wdecor.h>
#include "../private/resource.h"
#include "../private/wdecor.h"

/** Create new window decoration.
 *
 * @param resource UI resource
 * @param caption Window caption
 * @param rwdecor Place to store pointer to new window decoration
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_wdecor_create(ui_resource_t *resource, const char *caption,
    ui_wdecor_t **rwdecor)
{
	ui_wdecor_t *wdecor;

	wdecor = calloc(1, sizeof(ui_wdecor_t));
	if (wdecor == NULL)
		return ENOMEM;

	wdecor->caption = str_dup(caption);
	if (wdecor->caption == NULL) {
		free(wdecor);
		return ENOMEM;
	}

	wdecor->res = resource;
	wdecor->active = true;
	*rwdecor = wdecor;
	return EOK;
}

/** Destroy window decoration.
 *
 * @param wdecor Window decoration or @c NULL
 */
void ui_wdecor_destroy(ui_wdecor_t *wdecor)
{
	if (wdecor == NULL)
		return;

	free(wdecor);
}

/** Set window decoration callbacks.
 *
 * @param wdecor Window decoration
 * @param cb Window decoration callbacks
 * @param arg Callback argument
 */
void ui_wdecor_set_cb(ui_wdecor_t *wdecor, ui_wdecor_cb_t *cb, void *arg)
{
	wdecor->cb = cb;
	wdecor->arg = arg;
}

/** Set window decoration rectangle.
 *
 * @param wdecor Window decoration
 * @param rect New window decoration rectangle
 */
void ui_wdecor_set_rect(ui_wdecor_t *wdecor, gfx_rect_t *rect)
{
	wdecor->rect = *rect;
}

/** Set active flag.
 *
 * Active window is the one receiving keyboard events.
 *
 * @param wdecor Window decoration
 * @param active @c true iff window is active
 */
void ui_wdecor_set_active(ui_wdecor_t *wdecor, bool active)
{
	wdecor->active = active;
}

/** Paint window decoration.
 *
 * @param wdecor Window decoration
 * @return EOK on success or an error code
 */
errno_t ui_wdecor_paint(ui_wdecor_t *wdecor)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_rect_t trect;
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;

	rect = wdecor->rect;

	rc = ui_paint_bevel(wdecor->res->gc, &rect,
	    wdecor->res->wnd_frame_hi_color,
	    wdecor->res->wnd_frame_sh_color, 1, &rect);
	if (rc != EOK)
		return rc;

	rc = ui_paint_bevel(wdecor->res->gc, &rect,
	    wdecor->res->wnd_highlight_color,
	    wdecor->res->wnd_shadow_color, 1, &rect);
	if (rc != EOK)
		return rc;

	rc = ui_paint_bevel(wdecor->res->gc, &rect,
	    wdecor->res->wnd_face_color,
	    wdecor->res->wnd_face_color, 2, &rect);
	if (rc != EOK)
		return rc;

	trect.p0 = rect.p0;
	trect.p1.x = rect.p1.x;
	trect.p1.y = rect.p0.y + 22;

	rc = ui_paint_bevel(wdecor->res->gc, &trect,
	    wdecor->res->wnd_shadow_color,
	    wdecor->res->wnd_highlight_color, 1, &trect);
	if (rc != EOK)
		return rc;

	rc = gfx_set_color(wdecor->res->gc, wdecor->active ?
	    wdecor->res->tbar_act_bg_color :
	    wdecor->res->tbar_inact_bg_color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(wdecor->res->gc, &trect);
	if (rc != EOK)
		return rc;

	gfx_text_fmt_init(&fmt);
	fmt.halign = gfx_halign_center;
	fmt.valign = gfx_valign_center;

	pos.x = (trect.p0.x + trect.p1.x) / 2;
	pos.y = (trect.p0.y + trect.p1.y) / 2;

	rc = gfx_set_color(wdecor->res->gc, wdecor->active ?
	    wdecor->res->tbar_act_text_color :
	    wdecor->res->tbar_inact_text_color);
	if (rc != EOK)
		return rc;

	rc = gfx_puttext(wdecor->res->font, &pos, &fmt, wdecor->caption);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Send decoration move event.
 *
 * @param wdecor Window decoration
 * @param pos Position where the title bar was pressed
 */
void ui_wdecor_move(ui_wdecor_t *wdecor, gfx_coord2_t *pos)
{
	if (wdecor->cb != NULL && wdecor->cb->move != NULL)
		wdecor->cb->move(wdecor, wdecor->arg, pos);
}

/** Handle window decoration position event.
 *
 * @param wdecor Window decoration
 * @param pos_event Position event
 */
void ui_wdecor_pos_event(ui_wdecor_t *wdecor, pos_event_t *event)
{
	gfx_rect_t trect;
	gfx_coord2_t pos;

	trect.p0.x = wdecor->rect.p0.x + 3;
	trect.p0.y = wdecor->rect.p0.y + 3;
	trect.p1.x = wdecor->rect.p1.x - 3;
	trect.p1.y = trect.p0.y + 22;

	pos.x = event->hpos;
	pos.y = event->vpos;

	if (event->type == POS_PRESS && gfx_pix_inside_rect(&pos, &trect))
		ui_wdecor_move(wdecor, &pos);
}

/** @}
 */
