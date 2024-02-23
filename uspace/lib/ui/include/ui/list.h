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
 * @file List
 */

#ifndef _UI_LIST_H
#define _UI_LIST_H

#include <errno.h>
#include <gfx/coord.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include <types/ui/list.h>

extern errno_t ui_list_create(ui_window_t *, bool, ui_list_t **);
extern void ui_list_destroy(ui_list_t *);
extern ui_control_t *ui_list_ctl(ui_list_t *);
extern void ui_list_set_cb(ui_list_t *, ui_list_cb_t *, void *);
extern void *ui_list_get_cb_arg(ui_list_t *);
extern void ui_list_set_rect(ui_list_t *, gfx_rect_t *);
extern errno_t ui_list_activate(ui_list_t *);
extern void ui_list_deactivate(ui_list_t *);
extern ui_list_entry_t *ui_list_get_cursor(ui_list_t *);
extern void ui_list_set_cursor(ui_list_t *, ui_list_entry_t *);
extern void ui_list_entry_attr_init(ui_list_entry_attr_t *);
extern errno_t ui_list_entry_append(ui_list_t *,
    ui_list_entry_attr_t *, ui_list_entry_t **);
extern void ui_list_entry_move_up(ui_list_entry_t *);
extern void ui_list_entry_move_down(ui_list_entry_t *);
extern void ui_list_entry_delete(ui_list_entry_t *);
extern void *ui_list_entry_get_arg(ui_list_entry_t *);
extern ui_list_t *ui_list_entry_get_list(ui_list_entry_t *);
extern errno_t ui_list_entry_set_caption(ui_list_entry_t *, const char *);
extern size_t ui_list_entries_cnt(ui_list_t *);
extern errno_t ui_list_sort(ui_list_t *);
extern void ui_list_cursor_center(ui_list_t *, ui_list_entry_t *);
extern ui_list_entry_t *ui_list_first(ui_list_t *);
extern ui_list_entry_t *ui_list_last(ui_list_t *);
extern ui_list_entry_t *ui_list_next(ui_list_entry_t *);
extern ui_list_entry_t *ui_list_prev(ui_list_entry_t *);
extern bool ui_list_is_active(ui_list_t *);

#endif

/** @}
 */
