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

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator panel types
 */

#ifndef TYPES_PANEL_H
#define TYPES_PANEL_H

#include <adt/list.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <ipc/loc.h>
#include <ui/window.h>
#include <stdint.h>

/** Panel entry attributes */
typedef struct {
	/** File name */
	const char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
} panel_entry_attr_t;

/** Panel entry */
typedef struct {
	/** Containing panel */
	struct panel *panel;
	/** Link to @c panel->entries */
	link_t lentries;
	/** File name */
	char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
} panel_entry_t;

/** Navigator panel
 *
 * This is a custom UI control.
 */
typedef struct panel {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Panel rectangle */
	gfx_rect_t rect;

	/** Panel color */
	gfx_color_t *color;

	/** Panel cursor color */
	gfx_color_t *curs_color;

	/** Active border color */
	gfx_color_t *act_border_color;

	/** Directory-type entry color */
	gfx_color_t *dir_color;

	/** Service-type entry color */
	gfx_color_t *svc_color;

	/** Panel entries (list of panel_entry_t) */
	list_t entries;

	/** Number of entries */
	size_t entries_cnt;

	/** First entry of current page */
	panel_entry_t *page;

	/** Index of first entry of current page */
	size_t page_idx;

	/** Cursor position */
	panel_entry_t *cursor;

	/** Index of entry under cursor */
	size_t cursor_idx;

	/** @c true iff the panel is active */
	bool active;

	/** Directory */
	char *dir;
} panel_t;

#endif

/** @}
 */
