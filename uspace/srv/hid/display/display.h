/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <display/info.h>
#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include "types/display/client.h"
#include "types/display/cursor.h"
#include "types/display/ddev.h"
#include "types/display/display.h"
#include "types/display/ptd_event.h"
#include "types/display/seat.h"

extern errno_t ds_display_create(gfx_context_t *, ds_display_flags_t,
    ds_display_t **);
extern void ds_display_destroy(ds_display_t *);
extern void ds_display_lock(ds_display_t *);
extern void ds_display_unlock(ds_display_t *);
extern void ds_display_get_info(ds_display_t *, display_info_t *);
extern void ds_display_add_client(ds_display_t *, ds_client_t *);
extern void ds_display_remove_client(ds_client_t *);
extern ds_client_t *ds_display_first_client(ds_display_t *);
extern ds_client_t *ds_display_next_client(ds_client_t *);
extern ds_window_t *ds_display_find_window(ds_display_t *, ds_wnd_id_t);
extern ds_window_t *ds_display_window_by_pos(ds_display_t *, gfx_coord2_t *);
extern void ds_display_add_window(ds_display_t *, ds_window_t *);
extern void ds_display_remove_window(ds_window_t *);
extern ds_window_t *ds_display_first_window(ds_display_t *);
extern ds_window_t *ds_display_last_window(ds_display_t *);
extern ds_window_t *ds_display_next_window(ds_window_t *);
extern ds_window_t *ds_display_prev_window(ds_window_t *);
extern errno_t ds_display_post_kbd_event(ds_display_t *, kbd_event_t *);
extern errno_t ds_display_post_ptd_event(ds_display_t *, ptd_event_t *);
extern void ds_display_add_seat(ds_display_t *, ds_seat_t *);
extern void ds_display_remove_seat(ds_seat_t *);
extern ds_seat_t *ds_display_first_seat(ds_display_t *);
extern ds_seat_t *ds_display_next_seat(ds_seat_t *);
extern errno_t ds_display_add_ddev(ds_display_t *, ds_ddev_t *);
extern void ds_display_remove_ddev(ds_ddev_t *);
extern ds_ddev_t *ds_display_first_ddev(ds_display_t *);
extern ds_ddev_t *ds_display_next_ddev(ds_ddev_t *);
extern void ds_display_add_cursor(ds_display_t *, ds_cursor_t *);
extern void ds_display_remove_cursor(ds_cursor_t *);
extern gfx_context_t *ds_display_get_gc(ds_display_t *);
extern errno_t ds_display_paint_bg(ds_display_t *, gfx_rect_t *);
extern errno_t ds_display_paint(ds_display_t *, gfx_rect_t *);

#endif

/** @}
 */
