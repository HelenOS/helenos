/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator panel
 */

#ifndef PANEL_H
#define PANEL_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include "types/panel.h"

extern errno_t panel_create(ui_window_t *, bool, panel_t **);
extern void panel_destroy(panel_t *);
extern void panel_set_cb(panel_t *, panel_cb_t *, void *);
extern errno_t panel_paint(panel_t *);
extern ui_evclaim_t panel_kbd_event(panel_t *, kbd_event_t *);
extern ui_evclaim_t panel_pos_event(panel_t *, pos_event_t *);
extern ui_control_t *panel_ctl(panel_t *);
extern void panel_set_rect(panel_t *, gfx_rect_t *);
extern bool panel_is_active(panel_t *);
extern errno_t panel_activate(panel_t *);
extern void panel_deactivate(panel_t *);
extern errno_t panel_read_dir(panel_t *, const char *);
extern void panel_activate_req(panel_t *);

#endif

/** @}
 */
