/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file Menu entry
 */

#ifndef _UI_MENUENTRY_H
#define _UI_MENUENTRY_H

#include <errno.h>
#include <gfx/coord.h>
#include <types/ui/menu.h>
#include <types/ui/menuentry.h>
#include <types/ui/event.h>

extern errno_t ui_menu_entry_create(ui_menu_t *, const char *,
    ui_menu_entry_t **);
extern void ui_menu_entry_destroy(ui_menu_entry_t *);
extern void ui_menu_entry_set_cb(ui_menu_entry_t *, ui_menu_entry_cb_t,
    void *);
extern ui_menu_entry_t *ui_menu_entry_first(ui_menu_t *);
extern ui_menu_entry_t *ui_menu_entry_next(ui_menu_entry_t *);
extern gfx_coord_t ui_menu_entry_width(ui_menu_entry_t *);
extern gfx_coord_t ui_menu_entry_height(ui_menu_entry_t *);
extern errno_t ui_menu_entry_paint(ui_menu_entry_t *, gfx_coord2_t *);
extern void ui_menu_entry_press(ui_menu_entry_t *, gfx_coord2_t *,
    gfx_coord2_t *);

#endif

/** @}
 */
