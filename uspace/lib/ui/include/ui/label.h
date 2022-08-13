/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Label
 */

#ifndef _UI_LABEL_H
#define _UI_LABEL_H

#include <errno.h>
#include <gfx/coord.h>
#include <gfx/text.h>
#include <types/ui/control.h>
#include <types/ui/label.h>
#include <types/ui/resource.h>

extern errno_t ui_label_create(ui_resource_t *, const char *,
    ui_label_t **);
extern void ui_label_destroy(ui_label_t *);
extern ui_control_t *ui_label_ctl(ui_label_t *);
extern void ui_label_set_rect(ui_label_t *, gfx_rect_t *);
extern void ui_label_set_halign(ui_label_t *, gfx_halign_t);
extern void ui_label_set_valign(ui_label_t *, gfx_valign_t);
extern errno_t ui_label_set_text(ui_label_t *, const char *);
extern errno_t ui_label_paint(ui_label_t *);

#endif

/** @}
 */
