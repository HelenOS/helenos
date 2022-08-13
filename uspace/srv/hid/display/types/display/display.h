/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <fibril_synch.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <io/input.h>
#include <memgfx/memgc.h>
#include <types/display/cursor.h>
#include "cursor.h"
#include "clonegc.h"
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

	/** Next ID to assign to a window.
	 *
	 * XXX Window IDs need to be unique per display just because
	 * we don't have a way to match GC connection to the proper
	 * client. Really this should be in ds_client_t and the ID
	 * space should be per client.
	 */
	ds_wnd_id_t next_wnd_id;
	/** Input service */
	input_t *input;

	/** Seats (of ds_seat_t) */
	list_t seats;

	/** Windows (of ds_window_t) in stacking order */
	list_t windows;

	/** Display devices (of ds_ddev_t) */
	list_t ddevs;

	/** Background color */
	gfx_color_t *bg_color;

	/** Stock cursors */
	ds_cursor_t *cursor[dcurs_limit];

	/** List of all cursors */
	list_t cursors;

	/** Bounding rectangle */
	gfx_rect_t rect;

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
