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

/** @addtogroup taskbar
 * @{
 */
/**
 * @file Taskbar window list
 */

#ifndef TYPES_WNDLIST_H
#define TYPES_WNDLIST_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <ui/pbutton.h>
#include <ui/fixed.h>
#include <ui/window.h>
#include <wndmgt.h>

/** Taskbar window list entry */
typedef struct {
	/** Containing window list */
	struct wndlist *wndlist;
	/** Link to wndlist->entries */
	link_t lentries;
	/** Window ID */
	sysarg_t wnd_id;
	/** Entry is visible */
	bool visible;
	/** Window button */
	ui_pbutton_t *button;
	/** Window button rectangle */
	gfx_rect_t rect;
} wndlist_entry_t;

/** Taskbar window list */
typedef struct wndlist {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Layout to which we add window buttons */
	ui_fixed_t *fixed;

	/** Window list rectangle */
	gfx_rect_t rect;

	/** Window list entries (of wndlist_entry_t) */
	list_t entries;

	/** Current button pitch */
	gfx_coord_t pitch;

	/** Window management service */
	wndmgt_t *wndmgt;

	/** Device ID of last input event */
	sysarg_t ev_idev_id;
} wndlist_t;

#endif

/** @}
 */
