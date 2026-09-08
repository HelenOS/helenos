/*
 * Copyright (c) 2026 Jiri Svoboda
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

/** @addtogroup uidemo
 * @{
 */
/**
 * @file User interface demo
 */

#ifndef UIDEMO_H
#define UIDEMO_H

#include <display.h>
#include <fibril_synch.h>
#include <gfx/bitmap.h>
#include <gfx/coord.h>
#include <ui/checkbox.h>
#include <ui/entry.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/pbutton.h>
#include <ui/progress.h>
#include <ui/rbutton.h>
#include <ui/scrollbar.h>
#include <ui/tab.h>
#include <ui/ui.h>
#include <ui/window.h>

/** User interface demo */
typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_menu_bar_t *mbar;
	ui_menu_t *mfile;
	ui_menu_t *medit;
	ui_menu_t *mpreferences;
	ui_menu_t *mhelp;
	ui_tab_set_t *tabset;
	ui_tab_t *tbasic;
	ui_tab_t *tlists;
	ui_tab_t *tbars;
	ui_entry_t *entry;
	gfx_bitmap_t *bitmap;
	ui_image_t *image;
	ui_label_t *label;
	ui_pbutton_t *pb1;
	ui_pbutton_t *pb2;
	ui_checkbox_t *checkbox;
	ui_rbutton_group_t *rbgroup;
	ui_rbutton_t *rbleft;
	ui_rbutton_t *rbcenter;
	ui_rbutton_t *rbright;
	ui_slider_t *slider;
	ui_scrollbar_t *hscrollbar;
	ui_scrollbar_t *vscrollbar;
	ui_list_t *list;
	ui_progress_t *progress;
	unsigned progress_value;
	fibril_timer_t *timer;
} ui_demo_t;

/** UI demo geometry */
typedef struct {
	gfx_rect_t mbar_rect;
	gfx_rect_t entry_rect;
	gfx_rect_t label_rect;
	gfx_rect_t pb1_rect;
	gfx_rect_t pb2_rect;
	gfx_rect_t image_rect;
	gfx_rect_t checkbox_rect;
	gfx_rect_t rbleft_rect;
	gfx_rect_t rbcenter_rect;
	gfx_rect_t rbright_rect;
	gfx_rect_t slider_rect;
	gfx_rect_t hscrollbar_rect;
	gfx_rect_t vscrollbar_rect;
	gfx_rect_t tabset_rect;
	gfx_rect_t list_rect;
	gfx_rect_t progress_rect;
} ui_demo_geom_t;

#endif

/** @}
 */
