/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
