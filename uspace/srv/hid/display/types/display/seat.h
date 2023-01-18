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

typedef sysarg_t ds_seat_id_t;

/** Display server seat */
typedef struct ds_seat {
	/** Containing display */
	struct ds_display *display;
	/** Link to display->seats */
	link_t lseats;
	/** Seat ID */
	ds_seat_id_t id;
	/** Seat name */
	char *name;
	/** Input device configurations mapping to this seat */
	list_t idevcfgs;
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
