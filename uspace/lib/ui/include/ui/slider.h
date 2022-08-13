/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Slider
 */

#ifndef _UI_SLIDER_H
#define _UI_SLIDER_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <types/ui/slider.h>

extern errno_t ui_slider_create(ui_resource_t *, ui_slider_t **);
extern void ui_slider_destroy(ui_slider_t *);
extern ui_control_t *ui_slider_ctl(ui_slider_t *);
extern void ui_slider_set_cb(ui_slider_t *, ui_slider_cb_t *, void *);
extern void ui_slider_set_rect(ui_slider_t *, gfx_rect_t *);
extern errno_t ui_slider_paint(ui_slider_t *);
extern errno_t ui_slider_btn_clear(ui_slider_t *);
extern gfx_coord_t ui_slider_length(ui_slider_t *);
extern void ui_slider_press(ui_slider_t *, gfx_coord2_t *);
extern void ui_slider_release(ui_slider_t *, gfx_coord2_t *);
extern void ui_slider_update(ui_slider_t *, gfx_coord2_t *);
extern void ui_slider_moved(ui_slider_t *, gfx_coord_t);
extern ui_evclaim_t ui_slider_pos_event(ui_slider_t *, pos_event_t *);

#endif

/** @}
 */
