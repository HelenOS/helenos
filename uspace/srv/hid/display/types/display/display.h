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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display type
 */

#ifndef TYPES_DISPLAY_DISPLAY_H
#define TYPES_DISPLAY_DISPLAY_H

#include <adt/list.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <io/input.h>
#include <memgfx/memgc.h>
#include <types/display/cursor.h>
#include "cursor.h"
#include "clonegc.h"
#include "seat.h"
#include "window.h"

/** Display flags */
typedef enum {
	/** No flags enabled */
	df_none = 0,
	/** Use double buffer for display */
	df_disp_double_buf = 0x1
} ds_display_flags_t;

/** Display server display */
typedef struct ds_display {
	/** Synchronize access to display */
	fibril_mutex_t lock;
	/** Clients (of ds_client_t) */
	list_t clients;
	/** WM clients (of ds_wmclient_t) */
	list_t wmclients;
	/** CFG clients (of ds_cfgclient_t) */
	list_t cfgclients;

	/** Next ID to assign to a window.
	 *
	 * XXX Window IDs need to be unique per display just because
	 * we don't have a way to match GC connection to the proper
	 * client. Really this should be in ds_client_t and the ID
	 * space should be per client.
	 */
	ds_wnd_id_t next_wnd_id;
	/** Next ID to assign to a seat */
	ds_seat_id_t next_seat_id;
	/** Input service */
	input_t *input;

	/** Seats (of ds_seat_t) */
	list_t seats;

	/** Windows (of ds_window_t) in stacking order */
	list_t windows;

	/** Display devices (of ds_ddev_t) */
	list_t ddevs;

	/** Input device configuration entries (of ds_idevcfg_t) */
	list_t idevcfgs;

	/** Queue of input events */
	list_t ievents;

	/** Input event processing fibril ID */
	fid_t ievent_fid;
	/** Input event condition variable */
	fibril_condvar_t ievent_cv;
	/** Signal input event fibril to quit */
	bool ievent_quit;
	/** Input event fibril terminated */
	bool ievent_done;

	/** Background color */
	gfx_color_t *bg_color;

	/** Stock cursors */
	ds_cursor_t *cursor[dcurs_limit];

	/** List of all cursors */
	list_t cursors;

	/** Bounding rectangle */
	gfx_rect_t rect;

	/** Maximize rectangle */
	gfx_rect_t max_rect;

	/** Backbuffer bitmap or @c NULL if not double-buffering */
	gfx_bitmap_t *backbuf;

	/** Backbuffer GC or @c NULL if not double-buffering */
	mem_gc_t *bbgc;

	/** Frontbuffer (clone) GC */
	ds_clonegc_t *fbgc;

	/** Backbuffer dirty rectangle */
	gfx_rect_t dirty_rect;

	/** Display flags */
	ds_display_flags_t flags;
} ds_display_t;

#endif

/** @}
 */
