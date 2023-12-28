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

/** @addtogroup libui
 * @{
 */
/**
 * @file Scrollbar
 */

#ifndef _UI_SCROLLBAR_H
#define _UI_SCROLLBAR_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/scrollbar.h>
#include <types/ui/ui.h>
#include <types/ui/window.h>

extern errno_t ui_scrollbar_create(ui_t *, ui_window_t *, ui_scrollbar_dir_t,
    ui_scrollbar_t **);
extern void ui_scrollbar_destroy(ui_scrollbar_t *);
extern ui_control_t *ui_scrollbar_ctl(ui_scrollbar_t *);
extern void ui_scrollbar_set_cb(ui_scrollbar_t *, ui_scrollbar_cb_t *, void *);
extern void ui_scrollbar_set_rect(ui_scrollbar_t *, gfx_rect_t *);
extern errno_t ui_scrollbar_paint(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_trough_length(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_move_length(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_get_pos(ui_scrollbar_t *);
extern void ui_scrollbar_set_thumb_length(ui_scrollbar_t *, gfx_coord_t);
extern void ui_scrollbar_set_pos(ui_scrollbar_t *, gfx_coord_t);
extern void ui_scrollbar_thumb_press(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_upper_trough_press(ui_scrollbar_t *);
extern void ui_scrollbar_lower_trough_press(ui_scrollbar_t *);
extern void ui_scrollbar_release(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_update(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_troughs_update(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_up(ui_scrollbar_t *);
extern void ui_scrollbar_down(ui_scrollbar_t *);
extern void ui_scrollbar_page_up(ui_scrollbar_t *);
extern void ui_scrollbar_page_down(ui_scrollbar_t *);
extern void ui_scrollbar_moved(ui_scrollbar_t *, gfx_coord_t);
extern ui_evclaim_t ui_scrollbar_pos_event(ui_scrollbar_t *, pos_event_t *);

#endif

/** @}
 */
