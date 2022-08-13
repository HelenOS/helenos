/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Image
 */

#ifndef _UI_IMAGE_H
#define _UI_IMAGE_H

#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/coord.h>
#include <types/ui/control.h>
#include <types/ui/image.h>
#include <types/ui/resource.h>

extern errno_t ui_image_create(ui_resource_t *, gfx_bitmap_t *, gfx_rect_t *,
    ui_image_t **);
extern void ui_image_destroy(ui_image_t *);
extern ui_control_t *ui_image_ctl(ui_image_t *);
extern void ui_image_set_bmp(ui_image_t *, gfx_bitmap_t *, gfx_rect_t *);
extern void ui_image_set_rect(ui_image_t *, gfx_rect_t *);
extern void ui_image_set_flags(ui_image_t *, ui_image_flags_t);
extern errno_t ui_image_paint(ui_image_t *);

#endif

/** @}
 */
