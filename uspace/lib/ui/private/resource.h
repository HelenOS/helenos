/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Resource structure
 *
 */

#ifndef _UI_PRIVATE_RESOURCE_H
#define _UI_PRIVATE_RESOURCE_H

#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/font.h>
#include <gfx/typeface.h>
#include <stdbool.h>
#include <types/ui/resource.h>

/** Actual structure of UI resources.
 *
 * Contains resources common accross the entire UI. This is private to libui.
 */
struct ui_resource {
	/** Graphic context */
	gfx_context_t *gc;
	/** Typeface */
	gfx_typeface_t *tface;
	/** Font */
	gfx_font_t *font;
	/** Text mode */
	bool textmode;

	/** UI background color */
	gfx_color_t *ui_bg_color;

	/** Button frame color */
	gfx_color_t *btn_frame_color;
	/** Button face color */
	gfx_color_t *btn_face_color;
	/** Button text color */
	gfx_color_t *btn_text_color;
	/** Button highlight color */
	gfx_color_t *btn_highlight_color;
	/** Button shadow color */
	gfx_color_t *btn_shadow_color;

	/** Window face color */
	gfx_color_t *wnd_face_color;
	/** Window text color */
	gfx_color_t *wnd_text_color;
	/** Window text highlight color */
	gfx_color_t *wnd_text_hgl_color;
	/** Window selected text color */
	gfx_color_t *wnd_sel_text_color;
	/** Window selected text highlight color */
	gfx_color_t *wnd_sel_text_hgl_color;
	/** Window selected text background color */
	gfx_color_t *wnd_sel_text_bg_color;
	/** Window frame hightlight color */
	gfx_color_t *wnd_frame_hi_color;
	/** Window frame shadow color */
	gfx_color_t *wnd_frame_sh_color;
	/** Window highlight color */
	gfx_color_t *wnd_highlight_color;
	/** Window shadow color */
	gfx_color_t *wnd_shadow_color;

	/** Active titlebar background color */
	gfx_color_t *tbar_act_bg_color;
	/** Active titlebar text color */
	gfx_color_t *tbar_act_text_color;
	/** Inactive titlebar background color */
	gfx_color_t *tbar_inact_bg_color;
	/** Inactive titlebar text color */
	gfx_color_t *tbar_inact_text_color;

	/** Entry (text entry, checkbox, radio button) foreground color */
	gfx_color_t *entry_fg_color;
	/** Entry (text entry, checkbox, raido button) background color */
	gfx_color_t *entry_bg_color;
	/** Entry (text entry, checkbox, raido button) active background color */
	gfx_color_t *entry_act_bg_color;
	/** Entry selected text foreground color */
	gfx_color_t *entry_sel_text_fg_color;
	/** Entry selected text background color */
	gfx_color_t *entry_sel_text_bg_color;

	/** Scrollbar through color */
	gfx_color_t *sbar_through_color;
	/** Scrollbar active through color */
	gfx_color_t *sbar_act_through_color;

	/** Expose callback or @c NULL */
	ui_expose_cb_t expose_cb;
	/** Expose callback argument */
	void *expose_arg;
};

#endif

/** @}
 */
