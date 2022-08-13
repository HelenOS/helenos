/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Clickmatic
 */

#ifndef _UI_CLICKMATIC_H
#define _UI_CLICKMATIC_H

#include <errno.h>
#include <gfx/coord.h>
#include <types/ui/clickmatic.h>
#include <types/ui/ui.h>

extern errno_t ui_clickmatic_create(ui_t *, ui_clickmatic_t **);
extern void ui_clickmatic_set_cb(ui_clickmatic_t *, ui_clickmatic_cb_t *,
    void *);
extern void ui_clickmatic_destroy(ui_clickmatic_t *);
extern void ui_clickmatic_press(ui_clickmatic_t *);
extern void ui_clickmatic_release(ui_clickmatic_t *);

#endif

/** @}
 */
