/*
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IO_WINDOW_H_
#define LIBC_IO_WINDOW_H_

#include <stdbool.h>
#include <async.h>
#include <loc.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>

typedef enum {
	WINDOW_MAIN = 1,
	WINDOW_DECORATED = 2,
	WINDOW_RESIZEABLE = 4
} window_flags_t;

typedef enum {
	GF_EMPTY = 0,
	GF_MOVE_X = 1,
	GF_MOVE_Y = 2,
	GF_RESIZE_X = 4,
	GF_RESIZE_Y = 8,
	GF_SCALE_X = 16,
	GF_SCALE_Y = 32
} window_grab_flags_t;

typedef enum {
	WINDOW_PLACEMENT_ANY = 0,
	WINDOW_PLACEMENT_CENTER_X = 1,
	WINDOW_PLACEMENT_CENTER_Y = 2,
	WINDOW_PLACEMENT_CENTER =
	    WINDOW_PLACEMENT_CENTER_X | WINDOW_PLACEMENT_CENTER_Y,
	WINDOW_PLACEMENT_LEFT = 4,
	WINDOW_PLACEMENT_RIGHT = 8,
	WINDOW_PLACEMENT_TOP = 16,
	WINDOW_PLACEMENT_BOTTOM = 32,
	WINDOW_PLACEMENT_ABSOLUTE_X = 64,
	WINDOW_PLACEMENT_ABSOLUTE_Y = 128,
	WINDOW_PLACEMENT_ABSOLUTE =
	    WINDOW_PLACEMENT_ABSOLUTE_X | WINDOW_PLACEMENT_ABSOLUTE_Y
} window_placement_flags_t;

typedef struct {
	sysarg_t object;
	sysarg_t slot;
	sysarg_t argument;
} signal_event_t;

typedef struct {
	sysarg_t offset_x;
	sysarg_t offset_y;
	sysarg_t width;
	sysarg_t height;
	window_placement_flags_t placement_flags;
} resize_event_t;

typedef enum {
	ET_KEYBOARD_EVENT,
	ET_POSITION_EVENT,
	ET_SIGNAL_EVENT,
	ET_WINDOW_FOCUS,
	ET_WINDOW_UNFOCUS,
	ET_WINDOW_RESIZE,
	ET_WINDOW_REFRESH,
	ET_WINDOW_DAMAGE,
	ET_WINDOW_CLOSE
} window_event_type_t;

typedef union {
	kbd_event_t kbd;
	pos_event_t pos;
	signal_event_t signal;
	resize_event_t resize;
} window_event_data_t;

typedef struct {
	link_t link;
	window_event_type_t type;
	window_event_data_t data;
} window_event_t;

extern errno_t win_register(async_sess_t *, window_flags_t, service_id_t *,
    service_id_t *);

extern errno_t win_get_event(async_sess_t *, window_event_t *);

extern errno_t win_damage(async_sess_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern errno_t win_grab(async_sess_t *, sysarg_t, sysarg_t);
extern errno_t win_resize(async_sess_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    window_placement_flags_t, void *);
extern errno_t win_close(async_sess_t *);
extern errno_t win_close_request(async_sess_t *);

#endif

/** @}
 */
