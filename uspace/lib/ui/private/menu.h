/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * @file Menu structure
 *
 */

#ifndef _UI_PRIVATE_MENU_H
#define _UI_PRIVATE_MENU_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/common.h>
#include <types/ui/menu.h>
#include <types/ui/resource.h>

/** Actual structure of menu.
 *
 * This is private to libui.
 */
struct ui_menu {
	/** Parent window */
	struct ui_window *parent;
	/** Caption */
	char *caption;
	/** Popup window or @c NULL if menu is not currently open */
	struct ui_popup *popup;
	/** Selected menu entry or @c NULL */
	struct ui_menu_entry *selected;
	/** Maximum caption width */
	gfx_coord_t max_caption_w;
	/** Maximum shortcut width */
	gfx_coord_t max_shortcut_w;
	/** Total entry height */
	gfx_coord_t total_h;
	/** Menu entries (ui_menu_entry_t) */
	list_t entries;
	/** Callbacks */
	struct ui_menu_cb *cb;
	/** Callback argument */
	void *arg;
	/** ID of device that activated entry */
	sysarg_t idev_id;
};

/** Menu geometry.
 *
 * Computed rectangles of menu elements.
 */
typedef struct {
	/** Outer rectangle */
	gfx_rect_t outer_rect;
	/** Entries rectangle */
	gfx_rect_t entries_rect;
} ui_menu_geom_t;

extern void ui_menu_get_geom(ui_menu_t *, gfx_coord2_t *, ui_menu_geom_t *);
extern ui_resource_t *ui_menu_get_res(ui_menu_t *);
extern errno_t ui_menu_paint_bg_gfx(ui_menu_t *, gfx_coord2_t *);
extern errno_t ui_menu_paint_bg_text(ui_menu_t *, gfx_coord2_t *);
extern void ui_menu_up(ui_menu_t *);
extern void ui_menu_down(ui_menu_t *);
extern void ui_menu_left(ui_menu_t *, sysarg_t);
extern void ui_menu_right(ui_menu_t *, sysarg_t);
extern void ui_menu_close_req(ui_menu_t *);
extern void ui_menu_press_accel(ui_menu_t *, char32_t, sysarg_t);

#endif

/** @}
 */
