/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window decoration
 */

#ifndef _UI_WDECOR_H
#define _UI_WDECOR_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <types/ui/wdecor.h>

extern errno_t ui_wdecor_create(ui_resource_t *, const char *,
    ui_wdecor_style_t, ui_wdecor_t **);
extern void ui_wdecor_destroy(ui_wdecor_t *);
extern void ui_wdecor_set_cb(ui_wdecor_t *, ui_wdecor_cb_t *, void *);
extern void ui_wdecor_set_rect(ui_wdecor_t *, gfx_rect_t *);
extern void ui_wdecor_set_active(ui_wdecor_t *, bool);
extern void ui_wdecor_set_maximized(ui_wdecor_t *, bool);
extern errno_t ui_wdecor_set_caption(ui_wdecor_t *, const char *);
extern errno_t ui_wdecor_paint(ui_wdecor_t *);
extern ui_evclaim_t ui_wdecor_pos_event(ui_wdecor_t *, pos_event_t *);
extern void ui_wdecor_rect_from_app(ui_wdecor_style_t, gfx_rect_t *,
    gfx_rect_t *);
extern void ui_wdecor_app_from_rect(ui_wdecor_style_t, gfx_rect_t *,
    gfx_rect_t *);

#endif

/** @}
 */
