/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file UI resources
 */

#ifndef _UI_RESOURCE_H
#define _UI_RESOURCE_H

#include <errno.h>
#include <gfx/context.h>
#include <gfx/font.h>
#include <stdbool.h>
#include <types/ui/resource.h>

extern errno_t ui_resource_create(gfx_context_t *, bool, ui_resource_t **);
extern void ui_resource_destroy(ui_resource_t *);
extern void ui_resource_set_expose_cb(ui_resource_t *, ui_expose_cb_t,
    void *);
extern void ui_resource_expose(ui_resource_t *);
extern gfx_font_t *ui_resource_get_font(ui_resource_t *);

#endif

/** @}
 */
