/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include "types/display/display.h"
#include "types/display/seat.h"
#include "types/display/ptd_event.h"
#include "types/display/window.h"

extern errno_t ds_seat_create(ds_display_t *, ds_seat_t **);
extern void ds_seat_destroy(ds_seat_t *);
extern void ds_seat_set_focus(ds_seat_t *, ds_window_t *);
extern void ds_seat_set_popup(ds_seat_t *, ds_window_t *);
extern void ds_seat_evac_wnd_refs(ds_seat_t *, ds_window_t *);
extern void ds_seat_switch_focus(ds_seat_t *);
extern errno_t ds_seat_post_kbd_event(ds_seat_t *, kbd_event_t *);
extern errno_t ds_seat_post_ptd_event(ds_seat_t *, ptd_event_t *);
extern errno_t ds_seat_post_pos_event(ds_seat_t *, pos_event_t *);
extern void ds_seat_set_wm_cursor(ds_seat_t *, ds_cursor_t *);
extern errno_t ds_seat_paint_pointer(ds_seat_t *, gfx_rect_t *);

#endif

/** @}
 */
