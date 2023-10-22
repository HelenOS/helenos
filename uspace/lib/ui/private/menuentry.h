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
 * @file Menu entry structure
 *
 */

#ifndef _UI_PRIVATE_MENUENTRY_H
#define _UI_PRIVATE_MENUENTRY_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <types/ui/menuentry.h>

/** Actual structure of menu entry.
 *
 * This is private to libui.
 */
struct ui_menu_entry {
	/** Containing menu */
	struct ui_menu *menu;
	/** Link to @c menu->entries */
	link_t lentries;
	/** Callbacks */
	ui_menu_entry_cb_t cb;
	/** This entry is disabled */
	bool disabled;
	/** This entry is a separator entry */
	bool separator;
	/** Menu entry is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
	/** Callback argument */
	void *arg;
	/** Caption */
	char *caption;
	/** Shortcut key(s) */
	char *shortcut;
};

/** Menu entry geometry.
 *
 * Computed positions of menu entry elements.
 */
typedef struct {
	/** Outer rectangle */
	gfx_rect_t outer_rect;
	/** Caption position */
	gfx_coord2_t caption_pos;
	/** Shortcut position */
	gfx_coord2_t shortcut_pos;
} ui_menu_entry_geom_t;

extern void ui_menu_entry_get_geom(ui_menu_entry_t *, gfx_coord2_t *,
    ui_menu_entry_geom_t *);
extern void ui_menu_entry_cb(ui_menu_entry_t *);

#endif

/** @}
 */
