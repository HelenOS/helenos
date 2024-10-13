/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup terminal
 * @{
 */
/**
 * @file
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <adt/prodcons.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/con_srv.h>
#include <io/cons_event.h>
#include <loc.h>
#include <stdatomic.h>
#include <str.h>
#include <task.h>
#include <termui.h>
#include <ui/ui.h>
#include <ui/window.h>

#define UTF8_CHAR_BUFFER_SIZE  (STR_BOUNDS(1) + 1)

typedef enum {
	tf_topleft = 1
} terminal_flags_t;

typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_resource_t *ui_res;
	gfx_context_t *gc;

	gfx_bitmap_t *bmp;
	sysarg_t w;
	sysarg_t h;
	gfx_rect_t update;
	gfx_coord2_t off;
	bool is_focused;

	fibril_mutex_t mtx;
	link_t link;
	atomic_flag refcnt;

	prodcons_t input_pc;
	char char_remains[UTF8_CHAR_BUFFER_SIZE];
	size_t char_remains_len;

	termui_t *termui;

	termui_color_t default_bgcolor;
	termui_color_t default_fgcolor;
	termui_color_t emphasis_bgcolor;
	termui_color_t emphasis_fgcolor;
	termui_color_t selection_bgcolor;
	termui_color_t selection_fgcolor;

	sysarg_t ucols;
	sysarg_t urows;
	charfield_t *ubuf;

	loc_srv_t *srv;
	service_id_t dsid;
	con_srvs_t srvs;

	task_wait_t wait;
	fid_t wfid;
} terminal_t;

/** Terminal event */
typedef struct {
	/** Link to list of events */
	link_t link;
	/** Console event */
	cons_event_t ev;
} terminal_event_t;

extern errno_t terminal_create(const char *, sysarg_t, sysarg_t,
    terminal_flags_t, const char *, terminal_t **);
extern void terminal_destroy(terminal_t *);

#endif

/** @}
 */
