/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display device
 */

#ifndef DDEV_H
#define DDEV_H

#include <errno.h>
#include <loc.h>
#include "types/display/ddev.h"
#include "types/display/display.h"

extern errno_t ds_ddev_create(ds_display_t *, ddev_t *, ddev_info_t *,
    char *, service_id_t, gfx_context_t *, ds_ddev_t **);
extern errno_t ds_ddev_open(ds_display_t *, service_id_t, ds_ddev_t **);
extern void ds_ddev_close(ds_ddev_t *);

#endif

/** @}
 */
