/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File dialog
 */

#ifndef _UI_FILEDIALOG_H
#define _UI_FILEDIALOG_H

#include <errno.h>
#include <types/ui/filedialog.h>
#include <types/ui/ui.h>

extern void ui_file_dialog_params_init(ui_file_dialog_params_t *);
extern errno_t ui_file_dialog_create(ui_t *, ui_file_dialog_params_t *,
    ui_file_dialog_t **);
extern void ui_file_dialog_set_cb(ui_file_dialog_t *, ui_file_dialog_cb_t *,
    void *);
extern void ui_file_dialog_destroy(ui_file_dialog_t *);

#endif

/** @}
 */
