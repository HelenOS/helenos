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

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu
 */

#ifndef _UI_MENU_H
#define _UI_MENU_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/common.h>
#include <types/ui/event.h>
#include <types/ui/menu.h>
#include <types/ui/menubar.h>
#include <types/ui/window.h>
#include <uchar.h>

extern errno_t ui_menu_create(ui_window_t *, ui_menu_t **);
extern void ui_menu_destroy(ui_menu_t *);
extern void ui_menu_set_cb(ui_menu_t *, ui_menu_cb_t *, void *);
extern errno_t ui_menu_open(ui_menu_t *, gfx_rect_t *, sysarg_t);
extern void ui_menu_close(ui_menu_t *);
extern bool ui_menu_is_open(ui_menu_t *);
extern errno_t ui_menu_paint(ui_menu_t *, gfx_coord2_t *);
extern ui_evclaim_t ui_menu_kbd_event(ui_menu_t *, kbd_event_t *);
extern ui_evclaim_t ui_menu_pos_event(ui_menu_t *, gfx_coord2_t *,
    pos_event_t *);
extern sysarg_t ui_menu_get_idev_id(ui_menu_t *);

#endif

/** @}
 */
