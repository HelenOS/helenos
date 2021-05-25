/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file Menu bar structure
 *
 */

#ifndef _UI_PRIVATE_MENUBAR_H
#define _UI_PRIVATE_MENUBAR_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <types/ui/menu.h>
#include <types/ui/menubar.h>

/** Actual structure of menu bar.
 *
 * This is private to libui.
 */
struct ui_menu_bar {
	/** Base control object */
	struct ui_control *control;
	/** UI */
	struct ui *ui;
	/** UI window containing menu bar */
	struct ui_window *window;
	/** Menu bar rectangle */
	gfx_rect_t rect;
	/** Selected menu or @c NULL */
	struct ui_menu *selected;
	/** List of menus (ui_menu_t) */
	list_t menus;
};

extern void ui_menu_bar_select(ui_menu_bar_t *, gfx_rect_t *, ui_menu_t *);

#endif

/** @}
 */
