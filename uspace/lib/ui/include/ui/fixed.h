/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Fixed layout
 */

#ifndef _UI_FIXED_H
#define _UI_FIXED_H

#include <errno.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/fixed.h>

extern errno_t ui_fixed_create(ui_fixed_t **);
extern void ui_fixed_destroy(ui_fixed_t *);
extern ui_control_t *ui_fixed_ctl(ui_fixed_t *);
extern errno_t ui_fixed_add(ui_fixed_t *, ui_control_t *);
extern void ui_fixed_remove(ui_fixed_t *, ui_control_t *);
extern errno_t ui_fixed_paint(ui_fixed_t *);
extern ui_evclaim_t ui_fixed_kbd_event(ui_fixed_t *, kbd_event_t *);
extern ui_evclaim_t ui_fixed_pos_event(ui_fixed_t *, pos_event_t *);
extern void ui_fixed_unfocus(ui_fixed_t *);

#endif

/** @}
 */
