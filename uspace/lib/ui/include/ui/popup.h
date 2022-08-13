/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Popup window
 */

#ifndef _UI_POPUP_H
#define _UI_POPUP_H

#include <errno.h>
#include <gfx/context.h>
#include <types/ui/control.h>
#include <types/ui/ui.h>
#include <types/ui/popup.h>
#include <types/ui/resource.h>
#include <types/ui/window.h>

extern void ui_popup_params_init(ui_popup_params_t *);
extern errno_t ui_popup_create(ui_t *, ui_window_t *, ui_popup_params_t *,
    ui_popup_t **);
extern void ui_popup_set_cb(ui_popup_t *, ui_popup_cb_t *, void *);
extern void ui_popup_destroy(ui_popup_t *);
extern void ui_popup_add(ui_popup_t *, ui_control_t *);
extern void ui_popup_remove(ui_popup_t *, ui_control_t *);
extern ui_resource_t *ui_popup_get_res(ui_popup_t *);
extern gfx_context_t *ui_popup_get_gc(ui_popup_t *);

#endif

/** @}
 */
