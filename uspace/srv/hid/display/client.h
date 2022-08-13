/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server client
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include "types/display/client.h"
#include "types/display/display.h"

extern errno_t ds_client_create(ds_display_t *, ds_client_cb_t *, void *,
    ds_client_t **);
extern void ds_client_destroy(ds_client_t *);
extern void ds_client_add_window(ds_client_t *, ds_window_t *);
extern void ds_client_remove_window(ds_window_t *);
extern ds_window_t *ds_client_find_window(ds_client_t *, ds_wnd_id_t);
extern ds_window_t *ds_client_first_window(ds_client_t *);
extern ds_window_t *ds_client_next_window(ds_window_t *);
extern errno_t ds_client_get_event(ds_client_t *, ds_window_t **,
    display_wnd_ev_t *);
extern void ds_client_purge_window_events(ds_client_t *, ds_window_t *);
extern errno_t ds_client_post_close_event(ds_client_t *, ds_window_t *);
extern errno_t ds_client_post_focus_event(ds_client_t *, ds_window_t *);
extern errno_t ds_client_post_kbd_event(ds_client_t *, ds_window_t *,
    kbd_event_t *);
extern errno_t ds_client_post_pos_event(ds_client_t *, ds_window_t *,
    pos_event_t *);
extern errno_t ds_client_post_resize_event(ds_client_t *, ds_window_t *,
    gfx_rect_t *);
extern errno_t ds_client_post_unfocus_event(ds_client_t *, ds_window_t *);

#endif

/** @}
 */
