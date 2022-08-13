/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server seat type
 */

#ifndef TYPES_DISPLAY_SEAT_H
#define TYPES_DISPLAY_SEAT_H

#include <adt/list.h>
#include <gfx/coord.h>

/** Display server seat */
typedef struct ds_seat {
	/** Containing display */
	struct ds_display *display;
	/** Link to display->seats */
	link_t lseats;
	/** Window this seat is focused on */
	struct ds_window *focus;
	/** This seat's popup window */
	struct ds_window *popup;
	/** Cursor selected by client */
	struct ds_cursor *client_cursor;
	/** Cursor override for window management or @c NULL */
	struct ds_cursor *wm_cursor;
	/** Pointer position */
	gfx_coord2_t pntpos;
} ds_seat_t;

#endif

/** @}
 */
