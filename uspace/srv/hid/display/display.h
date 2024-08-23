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
 * @file Display server display
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <display/info.h>
#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include "types/display/cfgclient.h"
#include "types/display/client.h"
#include "types/display/cursor.h"
#include "types/display/ddev.h"
#include "types/display/display.h"
#include "types/display/idev.h"
#include "types/display/idevcfg.h"
#include "types/display/ptd_event.h"
#include "types/display/seat.h"
#include "types/display/wmclient.h"

extern errno_t ds_display_create(gfx_context_t *, ds_display_flags_t,
    ds_display_t **);
extern void ds_display_destroy(ds_display_t *);
errno_t ds_display_load_cfg(ds_display_t *, const char *);
errno_t ds_display_save_cfg(ds_display_t *, const char *);
extern void ds_display_lock(ds_display_t *);
extern void ds_display_unlock(ds_display_t *);
extern void ds_display_get_info(ds_display_t *, display_info_t *);
extern void ds_display_add_client(ds_display_t *, ds_client_t *);
extern void ds_display_remove_client(ds_client_t *);
extern ds_client_t *ds_display_first_client(ds_display_t *);
extern ds_client_t *ds_display_next_client(ds_client_t *);
extern void ds_display_add_wmclient(ds_display_t *, ds_wmclient_t *);
extern void ds_display_remove_wmclient(ds_wmclient_t *);
extern void ds_display_add_cfgclient(ds_display_t *, ds_cfgclient_t *);
extern void ds_display_remove_cfgclient(ds_cfgclient_t *);
extern ds_wmclient_t *ds_display_first_wmclient(ds_display_t *);
extern ds_wmclient_t *ds_display_next_wmclient(ds_wmclient_t *);
extern ds_window_t *ds_display_find_window(ds_display_t *, ds_wnd_id_t);
extern ds_window_t *ds_display_window_by_pos(ds_display_t *, gfx_coord2_t *);
extern void ds_display_enlist_window(ds_display_t *, ds_window_t *);
extern void ds_display_add_window(ds_display_t *, ds_window_t *);
extern void ds_display_remove_window(ds_window_t *);
extern void ds_display_window_to_top(ds_window_t *);
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
extern ds_seat_t *ds_display_default_seat(ds_display_t *);
extern ds_seat_t *ds_display_find_seat(ds_display_t *, ds_seat_id_t);
extern ds_seat_t *ds_display_seat_by_idev(ds_display_t *, ds_idev_id_t);
extern errno_t ds_display_add_ddev(ds_display_t *, ds_ddev_t *);
extern void ds_display_remove_ddev(ds_ddev_t *);
extern ds_ddev_t *ds_display_first_ddev(ds_display_t *);
extern ds_ddev_t *ds_display_next_ddev(ds_ddev_t *);
extern void ds_display_add_idevcfg(ds_display_t *, ds_idevcfg_t *);
extern void ds_display_remove_idevcfg(ds_idevcfg_t *);
extern ds_idevcfg_t *ds_display_first_idevcfg(ds_display_t *);
extern ds_idevcfg_t *ds_display_next_idevcfg(ds_idevcfg_t *);
extern void ds_display_add_cursor(ds_display_t *, ds_cursor_t *);
extern void ds_display_remove_cursor(ds_cursor_t *);
extern void ds_display_update_max_rect(ds_display_t *);
extern void ds_display_crop_max_rect(gfx_rect_t *, gfx_rect_t *);
extern gfx_context_t *ds_display_get_gc(ds_display_t *);
extern errno_t ds_display_paint_bg(ds_display_t *, gfx_rect_t *);
extern errno_t ds_display_paint(ds_display_t *, gfx_rect_t *);

#endif

/** @}
 */
