/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** Create new UI resource in graphics mode.
 *
 * @param gc Graphic context
 * @param rresource Place to store pointer to new UI resource
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t ui_resource_create_gfx(gfx_context_t *gc,
    ui_resource_t **rresource)
{
	ui_resource_t *resource;
	gfx_typeface_t *tface = NULL;
	gfx_font_t *font = NULL;
	gfx_font_info_t *finfo;
	gfx_color_t *btn_frame_color = NULL;
	gfx_color_t *btn_face_color = NULL;
	gfx_color_t *btn_face_lit_color = NULL;
	gfx_color_t *btn_text_color = NULL;
	gfx_color_t *btn_highlight_color = NULL;
	gfx_color_t *btn_shadow_color = NULL;
	gfx_color_t *wnd_face_color = NULL;
	gfx_color_t *wnd_text_color = NULL;
	gfx_color_t *wnd_dis_text_color = NULL;
	gfx_color_t *wnd_text_hgl_color = NULL;
	gfx_color_t *wnd_sel_text_color = NULL;
	gfx_color_t *wnd_sel_text_hgl_color = NULL;
	gfx_color_t *wnd_sel_text_bg_color = NULL;
	gfx_color_t *wnd_frame_hi_color = NULL;
	gfx_color_t *wnd_frame_sh_color = NULL;
	gfx_color_t *wnd_highlight_color = NULL;
	gfx_color_t *wnd_shadow_color = NULL;
	gfx_color_t *tbar_act_bg_color = NULL;
	gfx_color_t *tbar_inact_bg_color = NULL;
	gfx_color_t *tbar_act_text_color = NULL;
	gfx_color_t *tbar_inact_text_color = NULL;
	gfx_color_t *entry_fg_color = NULL;
	gfx_color_t *entry_bg_color = NULL;
	gfx_color_t *entry_act_bg_color = NULL;
	gfx_color_t *entry_sel_text_fg_color = NULL;
	gfx_color_t *entry_sel_text_bg_color = NULL;
	gfx_color_t *sbar_trough_color = NULL;
	gfx_color_t *sbar_act_trough_color = NULL;
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

	rc = gfx_color_new_rgb_i16(0, 0, 0, &btn_frame_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xc8c8, 0xc8c8, 0xc8c8, &btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xe8e8, 0xe8e8, 0xe8e8, &btn_face_lit_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0, 0, 0, &btn_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff,
	    &btn_highlight_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x8888, 0x8888, 0x8888, &btn_shadow_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xc8c8, 0xc8c8, 0xc8c8, &wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0, 0, 0, &wnd_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x9696, 0x9696, 0x9696, &wnd_dis_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0, 0, 0, &wnd_text_hgl_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &wnd_sel_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff,
	    &wnd_sel_text_hgl_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x5858, 0x6a6a, 0xc4c4,
	    &wnd_sel_text_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x8888, 0x8888, 0x8888, &wnd_frame_hi_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x4444, 0x4444, 0x4444, &wnd_frame_sh_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff,
	    &wnd_highlight_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x8888, 0x8888, 0x8888, &wnd_shadow_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x5858, 0x6a6a, 0xc4c4, &tbar_act_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff,
	    &tbar_act_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xdddd, 0xdddd, 0xdddd,
	    &tbar_inact_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x5858, 0x5858, 0x5858,
	    &tbar_inact_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0, 0, 0, &entry_fg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &entry_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xc8c8, 0xc8c8, 0xc8c8, &entry_act_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff,
	    &entry_sel_text_fg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0, 0, 0xffff, &entry_sel_text_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0xe4e4, 0xe4e4, 0xe4e4,
	    &sbar_trough_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_rgb_i16(0x5858, 0x5858, 0x5858,
	    &sbar_act_trough_color);
	if (rc != EOK)
		goto error;

	resource->gc = gc;
	resource->tface = tface;
	resource->font = font;
	resource->textmode = false;

	resource->btn_frame_color = btn_frame_color;
	resource->btn_face_color = btn_face_color;
	resource->btn_face_lit_color = btn_face_lit_color;
	resource->btn_text_color = btn_text_color;
	resource->btn_highlight_color = btn_highlight_color;
	resource->btn_shadow_color = btn_shadow_color;

	resource->wnd_face_color = wnd_face_color;
	resource->wnd_text_color = wnd_text_color;
	resource->wnd_dis_text_color = wnd_dis_text_color;
	resource->wnd_text_hgl_color = wnd_text_hgl_color;
	resource->wnd_sel_text_color = wnd_sel_text_color;
	resource->wnd_sel_text_hgl_color = wnd_sel_text_hgl_color;
	resource->wnd_sel_text_bg_color = wnd_sel_text_bg_color;
	resource->wnd_frame_hi_color = wnd_frame_hi_color;
	resource->wnd_frame_sh_color = wnd_frame_sh_color;
	resource->wnd_highlight_color = wnd_highlight_color;
	resource->wnd_shadow_color = wnd_shadow_color;

	resource->tbar_act_bg_color = tbar_act_bg_color;
	resource->tbar_act_text_color = tbar_act_text_color;
	resource->tbar_inact_bg_color = tbar_inact_bg_color;
	resource->tbar_inact_text_color = tbar_inact_text_color;

	resource->entry_fg_color = entry_fg_color;
	resource->entry_bg_color = entry_bg_color;
	resource->entry_act_bg_color = entry_act_bg_color;
	resource->entry_sel_text_fg_color = entry_sel_text_fg_color;
	resource->entry_sel_text_bg_color = entry_sel_text_bg_color;

	resource->sbar_trough_color = sbar_trough_color;
	resource->sbar_act_trough_color = sbar_act_trough_color;

	*rresource = resource;
	return EOK;
error:
	if (btn_frame_color != NULL)
		gfx_color_delete(btn_frame_color);
	if (btn_face_color != NULL)
		gfx_color_delete(btn_face_color);
	if (btn_face_lit_color != NULL)
		gfx_color_delete(btn_face_lit_color);
	if (btn_text_color != NULL)
		gfx_color_delete(btn_text_color);
	if (btn_highlight_color != NULL)
		gfx_color_delete(btn_highlight_color);
	if (btn_shadow_color != NULL)
		gfx_color_delete(btn_shadow_color);

	if (wnd_face_color != NULL)
		gfx_color_delete(wnd_face_color);
	if (wnd_text_color != NULL)
		gfx_color_delete(wnd_text_color);
	if (wnd_dis_text_color != NULL)
		gfx_color_delete(wnd_dis_text_color);
	if (wnd_text_hgl_color != NULL)
		gfx_color_delete(wnd_text_hgl_color);
	if (wnd_sel_text_color != NULL)
		gfx_color_delete(wnd_sel_text_color);
	if (wnd_sel_text_hgl_color != NULL)
		gfx_color_delete(wnd_sel_text_hgl_color);
	if (wnd_sel_text_bg_color != NULL)
		gfx_color_delete(wnd_sel_text_bg_color);
	if (wnd_frame_hi_color != NULL)
		gfx_color_delete(wnd_frame_hi_color);
	if (wnd_frame_sh_color != NULL)
		gfx_color_delete(wnd_frame_sh_color);
	if (wnd_highlight_color != NULL)
		gfx_color_delete(wnd_highlight_color);
	if (wnd_shadow_color != NULL)
		gfx_color_delete(wnd_shadow_color);

	if (tbar_act_bg_color != NULL)
		gfx_color_delete(tbar_act_bg_color);
	if (tbar_act_text_color != NULL)
		gfx_color_delete(tbar_act_text_color);
	if (tbar_inact_bg_color != NULL)
		gfx_color_delete(tbar_inact_bg_color);
	if (tbar_inact_text_color != NULL)
		gfx_color_delete(tbar_inact_text_color);

	if (entry_fg_color != NULL)
		gfx_color_delete(entry_fg_color);
	if (entry_bg_color != NULL)
		gfx_color_delete(entry_bg_color);
	if (entry_sel_text_fg_color != NULL)
		gfx_color_delete(entry_sel_text_fg_color);
	if (entry_sel_text_bg_color != NULL)
		gfx_color_delete(entry_sel_text_bg_color);
	if (entry_act_bg_color != NULL)
		gfx_color_delete(entry_act_bg_color);

	if (sbar_trough_color != NULL)
		gfx_color_delete(sbar_trough_color);
	if (sbar_act_trough_color != NULL)
		gfx_color_delete(sbar_act_trough_color);

	if (tface != NULL)
		gfx_typeface_destroy(tface);
	free(resource);
	return rc;
}

/** Create new UI resource in text mode.
 *
 * @param gc Graphic context
 * @param rresource Place to store pointer to new UI resource
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t ui_resource_create_text(gfx_context_t *gc,
    ui_resource_t **rresource)
{
	ui_resource_t *resource;
	gfx_typeface_t *tface = NULL;
	gfx_font_t *font = NULL;
	gfx_color_t *btn_frame_color = NULL;
	gfx_color_t *btn_face_color = NULL;
	gfx_color_t *btn_face_lit_color = NULL;
	gfx_color_t *btn_text_color = NULL;
	gfx_color_t *btn_highlight_color = NULL;
	gfx_color_t *btn_shadow_color = NULL;
	gfx_color_t *wnd_face_color = NULL;
	gfx_color_t *wnd_text_color = NULL;
	gfx_color_t *wnd_dis_text_color = NULL;
	gfx_color_t *wnd_text_hgl_color = NULL;
	gfx_color_t *wnd_sel_text_color = NULL;
	gfx_color_t *wnd_sel_text_hgl_color = NULL;
	gfx_color_t *wnd_sel_text_bg_color = NULL;
	gfx_color_t *wnd_frame_hi_color = NULL;
	gfx_color_t *wnd_frame_sh_color = NULL;
	gfx_color_t *wnd_highlight_color = NULL;
	gfx_color_t *wnd_shadow_color = NULL;
	gfx_color_t *tbar_act_bg_color = NULL;
	gfx_color_t *tbar_inact_bg_color = NULL;
	gfx_color_t *tbar_act_text_color = NULL;
	gfx_color_t *tbar_inact_text_color = NULL;
	gfx_color_t *entry_fg_color = NULL;
	gfx_color_t *entry_bg_color = NULL;
	gfx_color_t *entry_sel_text_fg_color = NULL;
	gfx_color_t *entry_sel_text_bg_color = NULL;
	gfx_color_t *entry_act_bg_color = NULL;
	gfx_color_t *sbar_trough_color = NULL;
	gfx_color_t *sbar_act_trough_color = NULL;
	errno_t rc;

	resource = calloc(1, sizeof(ui_resource_t));
	if (resource == NULL)
		return ENOMEM;

	/* Create dummy font for text mode */
	rc = gfx_typeface_create(gc, &tface);
	if (rc != EOK)
		goto error;

	rc = gfx_font_create_textmode(tface, &font);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &btn_frame_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x20, &btn_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x30, &btn_face_lit_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x20, &btn_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x20, &btn_highlight_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x01, &btn_shadow_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &wnd_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x78, &wnd_dis_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x74, &wnd_text_hgl_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &wnd_sel_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x04, &wnd_sel_text_hgl_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &wnd_sel_text_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &wnd_frame_hi_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x01, &wnd_frame_sh_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &wnd_highlight_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x01, &wnd_shadow_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &tbar_act_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x70, &tbar_act_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &tbar_inact_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &tbar_inact_text_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &entry_fg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &entry_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x1e, &entry_sel_text_fg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x1e, &entry_sel_text_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x37, &entry_act_bg_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &sbar_trough_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &sbar_act_trough_color);
	if (rc != EOK)
		goto error;

	resource->gc = gc;
	resource->tface = tface;
	resource->font = font;
	resource->textmode = true;

	resource->btn_frame_color = btn_frame_color;
	resource->btn_face_color = btn_face_color;
	resource->btn_face_lit_color = btn_face_lit_color;
	resource->btn_text_color = btn_text_color;
	resource->btn_highlight_color = btn_highlight_color;
	resource->btn_shadow_color = btn_shadow_color;

	resource->wnd_face_color = wnd_face_color;
	resource->wnd_text_color = wnd_text_color;
	resource->wnd_dis_text_color = wnd_dis_text_color;
	resource->wnd_text_hgl_color = wnd_text_hgl_color;
	resource->wnd_sel_text_color = wnd_sel_text_color;
	resource->wnd_sel_text_hgl_color = wnd_sel_text_hgl_color;
	resource->wnd_sel_text_bg_color = wnd_sel_text_bg_color;
	resource->wnd_frame_hi_color = wnd_frame_hi_color;
	resource->wnd_frame_sh_color = wnd_frame_sh_color;
	resource->wnd_highlight_color = wnd_highlight_color;
	resource->wnd_shadow_color = wnd_shadow_color;

	resource->tbar_act_bg_color = tbar_act_bg_color;
	resource->tbar_act_text_color = tbar_act_text_color;
	resource->tbar_inact_bg_color = tbar_inact_bg_color;
	resource->tbar_inact_text_color = tbar_inact_text_color;

	resource->entry_fg_color = entry_fg_color;
	resource->entry_bg_color = entry_bg_color;
	resource->entry_act_bg_color = entry_act_bg_color;
	resource->entry_sel_text_fg_color = entry_sel_text_fg_color;
	resource->entry_sel_text_bg_color = entry_sel_text_bg_color;

	resource->sbar_trough_color = sbar_trough_color;
	resource->sbar_act_trough_color = sbar_act_trough_color;

	*rresource = resource;
	return EOK;
error:
	if (btn_frame_color != NULL)
		gfx_color_delete(btn_frame_color);
	if (btn_face_color != NULL)
		gfx_color_delete(btn_face_color);
	if (btn_face_lit_color != NULL)
		gfx_color_delete(btn_face_lit_color);
	if (btn_text_color != NULL)
		gfx_color_delete(btn_text_color);
	if (btn_highlight_color != NULL)
		gfx_color_delete(btn_highlight_color);
	if (btn_shadow_color != NULL)
		gfx_color_delete(btn_shadow_color);

	if (wnd_face_color != NULL)
		gfx_color_delete(wnd_face_color);
	if (wnd_text_color != NULL)
		gfx_color_delete(wnd_text_color);
	if (wnd_dis_text_color != NULL)
		gfx_color_delete(wnd_dis_text_color);
	if (wnd_text_hgl_color != NULL)
		gfx_color_delete(wnd_text_hgl_color);
	if (wnd_sel_text_color != NULL)
		gfx_color_delete(wnd_sel_text_color);
	if (wnd_sel_text_hgl_color != NULL)
		gfx_color_delete(wnd_sel_text_hgl_color);
	if (wnd_sel_text_bg_color != NULL)
		gfx_color_delete(wnd_sel_text_bg_color);
	if (wnd_frame_hi_color != NULL)
		gfx_color_delete(wnd_frame_hi_color);
	if (wnd_frame_sh_color != NULL)
		gfx_color_delete(wnd_frame_sh_color);
	if (wnd_highlight_color != NULL)
		gfx_color_delete(wnd_highlight_color);
	if (wnd_shadow_color != NULL)
		gfx_color_delete(wnd_shadow_color);

	if (tbar_act_bg_color != NULL)
		gfx_color_delete(tbar_act_bg_color);
	if (tbar_act_text_color != NULL)
		gfx_color_delete(tbar_act_text_color);
	if (tbar_inact_bg_color != NULL)
		gfx_color_delete(tbar_inact_bg_color);
	if (tbar_inact_text_color != NULL)
		gfx_color_delete(tbar_inact_text_color);

	if (entry_fg_color != NULL)
		gfx_color_delete(entry_fg_color);
	if (entry_bg_color != NULL)
		gfx_color_delete(entry_bg_color);
	if (entry_act_bg_color != NULL)
		gfx_color_delete(entry_act_bg_color);
	if (entry_sel_text_fg_color != NULL)
		gfx_color_delete(entry_sel_text_fg_color);
	if (entry_sel_text_bg_color != NULL)
		gfx_color_delete(entry_sel_text_bg_color);
	if (sbar_trough_color != NULL)
		gfx_color_delete(sbar_trough_color);
	if (sbar_act_trough_color != NULL)
		gfx_color_delete(sbar_act_trough_color);

	if (tface != NULL)
		gfx_typeface_destroy(tface);
	free(resource);
	return rc;
}

/** Create new UI resource.
 *
 * @param gc Graphic context
 * @param textmode @c true if running in text mode
 * @param rresource Place to store pointer to new UI resource
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_resource_create(gfx_context_t *gc, bool textmode,
    ui_resource_t **rresource)
{
	if (textmode)
		return ui_resource_create_text(gc, rresource);
	else
		return ui_resource_create_gfx(gc, rresource);
}

/** Destroy UI resource.
 *
 * @param resource UI resource or @c NULL
 */
void ui_resource_destroy(ui_resource_t *resource)
{
	if (resource == NULL)
		return;

	gfx_color_delete(resource->btn_frame_color);
	gfx_color_delete(resource->btn_face_color);
	gfx_color_delete(resource->btn_face_lit_color);
	gfx_color_delete(resource->btn_text_color);
	gfx_color_delete(resource->btn_highlight_color);
	gfx_color_delete(resource->btn_shadow_color);

	gfx_color_delete(resource->wnd_face_color);
	gfx_color_delete(resource->wnd_text_color);
	gfx_color_delete(resource->wnd_dis_text_color);
	gfx_color_delete(resource->wnd_sel_text_color);
	gfx_color_delete(resource->wnd_sel_text_bg_color);
	gfx_color_delete(resource->wnd_frame_hi_color);
	gfx_color_delete(resource->wnd_frame_sh_color);
	gfx_color_delete(resource->wnd_highlight_color);
	gfx_color_delete(resource->wnd_shadow_color);

	gfx_color_delete(resource->tbar_act_bg_color);
	gfx_color_delete(resource->tbar_act_text_color);
	gfx_color_delete(resource->tbar_inact_bg_color);
	gfx_color_delete(resource->tbar_inact_text_color);

	gfx_color_delete(resource->entry_fg_color);
	gfx_color_delete(resource->entry_bg_color);
	gfx_color_delete(resource->entry_act_bg_color);
	gfx_color_delete(resource->entry_sel_text_fg_color);
	gfx_color_delete(resource->entry_sel_text_bg_color);

	gfx_color_delete(resource->sbar_trough_color);
	gfx_color_delete(resource->sbar_act_trough_color);

	gfx_font_close(resource->font);
	gfx_typeface_destroy(resource->tface);
	free(resource);
}

/** Set UI resource expose callback.
 *
 * @param resource Resource
 * @param cb Callback
 * @param arg Callback argument
 */
void ui_resource_set_expose_cb(ui_resource_t *resource,
    ui_expose_cb_t cb, void *arg)
{
	resource->expose_cb = cb;
	resource->expose_arg = arg;
}

/** Force UI repaint after an area has been exposed.
 *
 * This is called when a popup disappears, which could have exposed some
 * other UI elements. It causes complete repaint of the UI.
 *
 * NOTE Ideally we could specify the exposed rectangle and then limit
 * the repaint to just that. That would, however, require means of
 * actually clipping the repaint operation.
 */
void ui_resource_expose(ui_resource_t *resource)
{
	if (resource->expose_cb != NULL)
		resource->expose_cb(resource->expose_arg);
}

/** Get the UI font.
 *
 * @param resource UI resource
 * @return UI font
 */
gfx_font_t *ui_resource_get_font(ui_resource_t *resource)
{
	return resource->font;
}

/** Determine if resource is textmode.
 *
 * @param resource UI resource
 * @return @c true iff resource is textmode
 */
bool ui_resource_is_textmode(ui_resource_t *resource)
{
	return resource->textmode;
}

/** Get the UI window face color.
 *
 * @param resource UI resource
 * @return UI window face color
 */
gfx_color_t *ui_resource_get_wnd_face_color(ui_resource_t *resource)
{
	return resource->wnd_face_color;
}

/** Get the UI window text color.
 *
 * @param resource UI resource
 * @return UI window text color
 */
gfx_color_t *ui_resource_get_wnd_text_color(ui_resource_t *resource)
{
	return resource->wnd_text_color;
}

/** @}
 */
