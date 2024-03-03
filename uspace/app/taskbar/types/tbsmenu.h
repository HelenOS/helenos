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

/** @addtogroup taskbar
 * @{
 */
/**
 * @file Taskbar start menu
 */

#ifndef TYPES_TBSMENU_H
#define TYPES_TBSMENU_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <tbarcfg/tbarcfg.h>
#include <ui/pbutton.h>
#include <ui/fixed.h>
#include <ui/menu.h>
#include <ui/menuentry.h>
#include <ui/window.h>

/** Taskbar start menu entry */
typedef struct {
	/** Containing start menu */
	struct tbsmenu *tbsmenu;
	/** Link to tbsmenu->entries */
	link_t lentries;
	/** Menu entry */
	ui_menu_entry_t *mentry;
	/** Caption */
	char *caption;
	/** Command to run */
	char *cmd;
	/** Start in terminal */
	bool terminal;
} tbsmenu_entry_t;

/** Taskbar start menu */
typedef struct tbsmenu {
	/** Containing window */
	ui_window_t *window;

	/** Layout to which we add start button */
	ui_fixed_t *fixed;

	/** Start button rectangle */
	gfx_rect_t rect;

	/** Start button */
	ui_pbutton_t *sbutton;

	/** Start menu */
	ui_menu_t *smenu;

	/** Start menu entries (of tbsmenu_entry_t) */
	list_t entries;

	/** Device ID of last input event */
	sysarg_t ev_idev_id;

	/** Repository path name */
	char *repopath;

	/** Need to reload menu when possible */
	bool needs_reload;

	/** Display specification */
	char *display_spec;
} tbsmenu_t;

/** Command split into individual parts */
typedef struct {
	/** NULL-terminated array of string pointers */
	char **argv;
} tbsmenu_cmd_t;

#endif

/** @}
 */
