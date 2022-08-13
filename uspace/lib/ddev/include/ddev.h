/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libddev
 * @{
 */
/** @file
 */

#ifndef _LIBDDEV_DDEV_H_
#define _LIBDDEV_DDEV_H_

#include <ddev/info.h>
#include <errno.h>
#include <gfx/context.h>
#include <stdbool.h>
#include "types/ddev.h"
#include "types/ddev/info.h"

extern errno_t ddev_open(const char *, ddev_t **);
extern void ddev_close(ddev_t *);
extern errno_t ddev_get_gc(ddev_t *, gfx_context_t **);
extern errno_t ddev_get_info(ddev_t *, ddev_info_t *);

#endif

/** @}
 */
