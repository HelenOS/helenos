/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File list
 */

#ifndef _UI_FILELIST_H
#define _UI_FILELIST_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include <types/ui/filelist.h>

extern errno_t ui_file_list_create(ui_window_t *, bool, ui_file_list_t **);
extern void ui_file_list_destroy(ui_file_list_t *);
extern ui_control_t *ui_file_list_ctl(ui_file_list_t *);
extern void ui_file_list_set_cb(ui_file_list_t *, ui_file_list_cb_t *, void *);
extern void ui_file_list_set_rect(ui_file_list_t *, gfx_rect_t *);
extern errno_t ui_file_list_read_dir(ui_file_list_t *, const char *);
extern errno_t ui_file_list_activate(ui_file_list_t *);
extern void ui_file_list_deactivate(ui_file_list_t *);
extern errno_t ui_file_list_open(ui_file_list_t *, ui_file_list_entry_t *);
extern ui_file_list_entry_t *ui_file_list_get_cursor(ui_file_list_t *);

#endif

/** @}
 */
