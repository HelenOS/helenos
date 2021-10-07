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
 * @file Navigator panel
 */

#ifndef PANEL_H
#define PANEL_H

#include <adt/list.h>
#include <errno.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdint.h>
#include "nav.h"
#include "panel.h"

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

	/** Panel entries (list of panel_entry_t) */
	list_t entries;

	/** First entry of current page */
	panel_entry_t *page;

	/** Cursor position */
	panel_entry_t *cursor;
} panel_t;

extern errno_t panel_create(ui_window_t *, panel_t **);
extern void panel_destroy(panel_t *);
extern errno_t panel_paint(panel_t *);
extern ui_evclaim_t panel_pos_event(panel_t *, pos_event_t *);
extern ui_control_t *panel_ctl(panel_t *);
extern void panel_set_rect(panel_t *, gfx_rect_t *);
extern errno_t panel_entry_append(panel_t *, const char *, uint64_t);
extern void panel_entry_delete(panel_entry_t *);
extern void panel_clear_entries(panel_t *);
extern errno_t panel_read_dir(panel_t *, const char *);
extern panel_entry_t *panel_first(panel_t *);
extern panel_entry_t *panel_next(panel_entry_t *);

#endif

/** @}
 */
