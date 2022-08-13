/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Message dialog
 */

#ifndef _UI_MSGDIALOG_H
#define _UI_MSGDIALOG_H

#include <errno.h>
#include <types/ui/msgdialog.h>
#include <types/ui/ui.h>

extern void ui_msg_dialog_params_init(ui_msg_dialog_params_t *);
extern errno_t ui_msg_dialog_create(ui_t *, ui_msg_dialog_params_t *,
    ui_msg_dialog_t **);
extern void ui_msg_dialog_set_cb(ui_msg_dialog_t *, ui_msg_dialog_cb_t *,
    void *);
extern void ui_msg_dialog_destroy(ui_msg_dialog_t *);

#endif

/** @}
 */
