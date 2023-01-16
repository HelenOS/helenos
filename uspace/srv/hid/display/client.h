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
extern errno_t ds_client_post_focus_event(ds_client_t *, ds_window_t *,
    display_wnd_focus_ev_t *);
extern errno_t ds_client_post_kbd_event(ds_client_t *, ds_window_t *,
    kbd_event_t *);
extern errno_t ds_client_post_pos_event(ds_client_t *, ds_window_t *,
    pos_event_t *);
extern errno_t ds_client_post_resize_event(ds_client_t *, ds_window_t *,
    gfx_rect_t *);
extern errno_t ds_client_post_unfocus_event(ds_client_t *, ds_window_t *,
    display_wnd_unfocus_ev_t *);

#endif

/** @}
 */
