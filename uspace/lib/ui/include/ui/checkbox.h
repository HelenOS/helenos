/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Check box
 */

#ifndef _UI_CHECKBOX_H
#define _UI_CHECKBOX_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/checkbox.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_checkbox_create(ui_resource_t *, const char *,
    ui_checkbox_t **);
extern void ui_checkbox_destroy(ui_checkbox_t *);
extern ui_control_t *ui_checkbox_ctl(ui_checkbox_t *);
extern void ui_checkbox_set_cb(ui_checkbox_t *, ui_checkbox_cb_t *, void *);
extern void ui_checkbox_set_rect(ui_checkbox_t *, gfx_rect_t *);
extern errno_t ui_checkbox_paint(ui_checkbox_t *);
extern void ui_checkbox_press(ui_checkbox_t *);
extern void ui_checkbox_release(ui_checkbox_t *);
extern void ui_checkbox_enter(ui_checkbox_t *);
extern void ui_checkbox_leave(ui_checkbox_t *);
extern void ui_checkbox_switched(ui_checkbox_t *);
extern ui_evclaim_t ui_checkbox_pos_event(ui_checkbox_t *, pos_event_t *);

#endif

/** @}
 */
