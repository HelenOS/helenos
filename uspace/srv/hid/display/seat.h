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
 * @file Display server seat
 */

#ifndef SEAT_H
#define SEAT_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <sif.h>
#include "types/display/display.h"
#include "types/display/seat.h"
#include "types/display/ptd_event.h"
#include "types/display/window.h"

extern errno_t ds_seat_create(ds_display_t *, const char *, ds_seat_t **);
extern void ds_seat_destroy(ds_seat_t *);
extern errno_t ds_seat_load(ds_display_t *, sif_node_t *, ds_seat_t **);
extern errno_t ds_seat_save(ds_seat_t *, sif_node_t *);
extern void ds_seat_set_focus(ds_seat_t *, ds_window_t *);
extern void ds_seat_set_popup(ds_seat_t *, ds_window_t *);
extern void ds_seat_evac_wnd_refs(ds_seat_t *, ds_window_t *);
extern void ds_seat_unfocus_wnd(ds_seat_t *, ds_window_t *);
extern void ds_seat_switch_focus(ds_seat_t *);
extern errno_t ds_seat_post_kbd_event(ds_seat_t *, kbd_event_t *);
extern errno_t ds_seat_post_ptd_event(ds_seat_t *, ptd_event_t *);
extern errno_t ds_seat_post_pos_event(ds_seat_t *, pos_event_t *);
extern void ds_seat_set_wm_cursor(ds_seat_t *, ds_cursor_t *);
extern errno_t ds_seat_paint_pointer(ds_seat_t *, gfx_rect_t *);
extern void ds_seat_add_idevcfg(ds_seat_t *, ds_idevcfg_t *);
extern void ds_seat_remove_idevcfg(ds_idevcfg_t *);
extern ds_idevcfg_t *ds_seat_first_idevcfg(ds_seat_t *);
extern ds_idevcfg_t *ds_seat_next_idevcfg(ds_idevcfg_t *);

#endif

/** @}
 */
