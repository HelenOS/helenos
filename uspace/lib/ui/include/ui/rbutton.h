/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Radio button
 */

#ifndef _UI_RBUTTON_H
#define _UI_RBUTTON_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/rbutton.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_rbutton_group_create(ui_resource_t *,
    ui_rbutton_group_t **);
extern void ui_rbutton_group_destroy(ui_rbutton_group_t *);
extern errno_t ui_rbutton_create(ui_rbutton_group_t *, const char *, void *,
    ui_rbutton_t **);
extern void ui_rbutton_destroy(ui_rbutton_t *);
extern ui_control_t *ui_rbutton_ctl(ui_rbutton_t *);
extern void ui_rbutton_group_set_cb(ui_rbutton_group_t *, ui_rbutton_group_cb_t *,
    void *);
extern void ui_rbutton_set_rect(ui_rbutton_t *, gfx_rect_t *);
extern errno_t ui_rbutton_paint(ui_rbutton_t *);
extern void ui_rbutton_press(ui_rbutton_t *);
extern void ui_rbutton_release(ui_rbutton_t *);
extern void ui_rbutton_enter(ui_rbutton_t *);
extern void ui_rbutton_leave(ui_rbutton_t *);
extern void ui_rbutton_select(ui_rbutton_t *);
extern void ui_rbutton_selected(ui_rbutton_t *);
extern ui_evclaim_t ui_rbutton_pos_event(ui_rbutton_t *, pos_event_t *);

#endif

/** @}
 */
