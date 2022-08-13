/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Push button
 */

#ifndef _UI_PBUTTON_H
#define _UI_PBUTTON_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/pbutton.h>
#include <types/ui/resource.h>
#include <stdbool.h>

extern errno_t ui_pbutton_create(ui_resource_t *, const char *,
    ui_pbutton_t **);
extern void ui_pbutton_destroy(ui_pbutton_t *);
extern ui_control_t *ui_pbutton_ctl(ui_pbutton_t *);
extern void ui_pbutton_set_cb(ui_pbutton_t *, ui_pbutton_cb_t *, void *);
extern void ui_pbutton_set_decor_ops(ui_pbutton_t *, ui_pbutton_decor_ops_t *,
    void *);
extern void ui_pbutton_set_flags(ui_pbutton_t *, ui_pbutton_flags_t);
extern void ui_pbutton_set_rect(ui_pbutton_t *, gfx_rect_t *);
extern void ui_pbutton_set_default(ui_pbutton_t *, bool);
extern errno_t ui_pbutton_paint(ui_pbutton_t *);
extern void ui_pbutton_press(ui_pbutton_t *);
extern void ui_pbutton_release(ui_pbutton_t *);
extern void ui_pbutton_enter(ui_pbutton_t *);
extern void ui_pbutton_leave(ui_pbutton_t *);
extern void ui_pbutton_clicked(ui_pbutton_t *);
extern void ui_pbutton_down(ui_pbutton_t *);
extern void ui_pbutton_up(ui_pbutton_t *);
extern ui_evclaim_t ui_pbutton_pos_event(ui_pbutton_t *, pos_event_t *);

#endif

/** @}
 */
