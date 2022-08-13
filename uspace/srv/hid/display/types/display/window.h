/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server window type
 */

#ifndef TYPES_DISPLAY_WINDOW_H
#define TYPES_DISPLAY_WINDOW_H

#include <adt/list.h>
#include <display/event.h>
#include <display/wndparams.h>
#include <display/wndresize.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/pixel.h>
#include <io/pixelmap.h>
#include <memgfx/memgc.h>

typedef sysarg_t ds_wnd_id_t;

/** Window state */
typedef enum {
	/** Idle */
	dsw_idle,
	/** Moving by mouse drag */
	dsw_moving,
	/** Resizing by mouse drag */
	dsw_resizing
} ds_window_state_t;

/** Display server window */
typedef struct ds_window {
	/** Parent client */
	struct ds_client *client;
	/** Link to @c client->windows */
	link_t lcwindows;
	/** Containing display */
	struct ds_display *display;
	/** Link to @c display->windows */
	link_t ldwindows;
	/** Bounding rectangle */
	gfx_rect_t rect;
	/** Display position */
	gfx_coord2_t dpos;
	/** Preview position (when moving) */
	gfx_coord2_t preview_pos;
	/** Preview rectangle (when resizing) */
	gfx_rect_t preview_rect;
	/** Minimum size */
	gfx_coord2_t min_size;
	/** Normal rectangle (when not maximized or minimized) */
	gfx_rect_t normal_rect;
	/** Normal display position (when not maximized or minimized) */
	gfx_coord2_t normal_dpos;
	/** Window ID */
	ds_wnd_id_t id;
	/** Memory GC */
	mem_gc_t *mgc;
	/** Graphic context */
	gfx_context_t *gc;
	/** Bitmap in the display device */
	gfx_bitmap_t *bitmap;
	/** Pixel map for accessing the window bitmap */
	pixelmap_t pixelmap;
	/** Current drawing color */
	pixel_t color;
	/** Cursor set by client */
	struct ds_cursor *cursor;
	/** Window flags */
	display_wnd_flags_t flags;
	/** State */
	ds_window_state_t state;
	/** Original position before started to move or resize the window */
	gfx_coord2_t orig_pos;
	/** Window resize type (if state is dsw_resizing) */
	display_wnd_rsztype_t rsztype;
} ds_window_t;

/** Window event queue entry */
typedef struct {
	/** Link to event queue */
	link_t levents;
	/** Window to which the event is delivered */
	ds_window_t *window;
	/** Event */
	display_wnd_ev_t event;
} ds_window_ev_t;

/** Bitmap in display server window GC */
typedef struct {
	/** Containing window */
	ds_window_t *wnd;
	/** Display bitmap */
	gfx_bitmap_t *bitmap;
	/** Bounding rectangle */
	gfx_rect_t rect;
	/** Bitmap flags */
	gfx_bitmap_flags_t flags;
	/** Key color */
	pixel_t key_color;
} ds_window_bitmap_t;

#endif

/** @}
 */
