/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Prompt dialog
 */

#ifndef _UI_PROMPTDIALOG_H
#define _UI_PROMPTDIALOG_H

#include <errno.h>
#include <types/ui/promptdialog.h>
#include <types/ui/ui.h>

extern void ui_prompt_dialog_params_init(ui_prompt_dialog_params_t *);
extern errno_t ui_prompt_dialog_create(ui_t *, ui_prompt_dialog_params_t *,
    ui_prompt_dialog_t **);
extern void ui_prompt_dialog_set_cb(ui_prompt_dialog_t *, ui_prompt_dialog_cb_t *,
    void *);
extern void ui_prompt_dialog_destroy(ui_prompt_dialog_t *);

#endif

/** @}
 */
