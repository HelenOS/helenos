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
 * @file Text entry
 */

#ifndef _UI_ENTRY_H
#define _UI_ENTRY_H

#include <errno.h>
#include <gfx/coord.h>
#include <gfx/text.h>
#include <types/ui/control.h>
#include <types/ui/entry.h>
#include <types/ui/window.h>

extern errno_t ui_entry_create(ui_window_t *, const char *,
    ui_entry_t **);
extern void ui_entry_destroy(ui_entry_t *);
extern ui_control_t *ui_entry_ctl(ui_entry_t *);
extern void ui_entry_set_rect(ui_entry_t *, gfx_rect_t *);
extern void ui_entry_set_halign(ui_entry_t *, gfx_halign_t);
extern void ui_entry_set_read_only(ui_entry_t *, bool);
extern errno_t ui_entry_set_text(ui_entry_t *, const char *);
extern const char *ui_entry_get_text(ui_entry_t *);
extern errno_t ui_entry_paint(ui_entry_t *);
extern errno_t ui_entry_insert_str(ui_entry_t *, const char *);
extern void ui_entry_activate(ui_entry_t *);
extern void ui_entry_deactivate(ui_entry_t *);
extern void ui_entry_backspace(ui_entry_t *);
extern void ui_entry_delete(ui_entry_t *);
extern void ui_entry_copy(ui_entry_t *);
extern void ui_entry_cut(ui_entry_t *);
extern void ui_entry_paste(ui_entry_t *);
extern void ui_entry_seek_start(ui_entry_t *, bool);
extern void ui_entry_seek_end(ui_entry_t *, bool);
extern void ui_entry_seek_prev_char(ui_entry_t *, bool);
extern void ui_entry_seek_next_char(ui_entry_t *, bool);
extern ui_evclaim_t ui_entry_kbd_event(ui_entry_t *, kbd_event_t *);
extern ui_evclaim_t ui_entry_pos_event(ui_entry_t *, pos_event_t *);

#endif

/** @}
 */
