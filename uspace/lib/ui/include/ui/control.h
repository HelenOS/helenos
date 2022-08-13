/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file UI control
 */

#ifndef _UI_CONTROL_H
#define _UI_CONTROL_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/event.h>

extern errno_t ui_control_new(ui_control_ops_t *, void *, ui_control_t **);
extern void ui_control_delete(ui_control_t *);
extern void ui_control_destroy(ui_control_t *);
extern errno_t ui_control_paint(ui_control_t *);
extern ui_evclaim_t ui_control_kbd_event(ui_control_t *, kbd_event_t *);
extern ui_evclaim_t ui_control_pos_event(ui_control_t *, pos_event_t *);
extern void ui_control_unfocus(ui_control_t *);

#endif

/** @}
 */
