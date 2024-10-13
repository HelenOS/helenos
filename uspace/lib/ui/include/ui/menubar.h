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
 * @file Menu bar
 */

#ifndef _UI_MENUBAR_H
#define _UI_MENUBAR_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/common.h>
#include <types/ui/menubar.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <types/ui/ui.h>
#include <types/ui/window.h>
#include <uchar.h>

extern errno_t ui_menu_bar_create(ui_t *, ui_window_t *,
    ui_menu_bar_t **);
extern void ui_menu_bar_destroy(ui_menu_bar_t *);
extern void ui_menu_bar_set_cb(ui_menu_bar_t *, ui_menu_bar_cb_t *, void *);
extern ui_control_t *ui_menu_bar_ctl(ui_menu_bar_t *);
extern void ui_menu_bar_set_rect(ui_menu_bar_t *, gfx_rect_t *);
extern errno_t ui_menu_bar_paint(ui_menu_bar_t *);
extern ui_evclaim_t ui_menu_bar_kbd_event(ui_menu_bar_t *, kbd_event_t *);
extern ui_evclaim_t ui_menu_bar_pos_event(ui_menu_bar_t *, pos_event_t *);
extern void ui_menu_bar_press_accel(ui_menu_bar_t *, char32_t, sysarg_t);
extern void ui_menu_bar_unfocus(ui_menu_bar_t *);
extern void ui_menu_bar_activate(ui_menu_bar_t *);
extern void ui_menu_bar_deactivate(ui_menu_bar_t *);
extern void ui_menu_bar_select_first(ui_menu_bar_t *, bool, sysarg_t);
extern void ui_menu_bar_select_last(ui_menu_bar_t *, bool, sysarg_t);

#endif

/** @}
 */
