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
 * @file Menu drop-down
 */

#ifndef _UI_MENUDD_H
#define _UI_MENUDD_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/common.h>
#include <types/ui/menu.h>
#include <types/ui/menudd.h>
#include <types/ui/menubar.h>
#include <types/ui/event.h>
#include <uchar.h>

extern errno_t ui_menu_dd_create(ui_menu_bar_t *, const char *, ui_menu_dd_t **,
    ui_menu_t **);
extern void ui_menu_dd_destroy(ui_menu_dd_t *);
extern ui_menu_dd_t *ui_menu_dd_first(ui_menu_bar_t *);
extern ui_menu_dd_t *ui_menu_dd_next(ui_menu_dd_t *);
extern ui_menu_dd_t *ui_menu_dd_last(ui_menu_bar_t *);
extern ui_menu_dd_t *ui_menu_dd_prev(ui_menu_dd_t *);
extern const char *ui_menu_dd_caption(ui_menu_dd_t *);
extern void ui_menu_dd_get_rect(ui_menu_dd_t *, gfx_coord2_t *, gfx_rect_t *);
extern char32_t ui_menu_dd_get_accel(ui_menu_dd_t *);
extern errno_t ui_menu_dd_open(ui_menu_dd_t *, gfx_rect_t *, sysarg_t);
extern void ui_menu_dd_close(ui_menu_dd_t *);
extern bool ui_menu_dd_is_open(ui_menu_dd_t *);

#endif

/** @}
 */
