/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Cloning grapics context
 */

#ifndef CLONEGC_H
#define CLONEGC_H

#include <errno.h>
#include <gfx/bitmap.h>
#include "types/display/clonegc.h"

extern gfx_context_ops_t ds_clonegc_ops;

extern errno_t ds_clonegc_create(gfx_context_t *, ds_clonegc_t **);
extern errno_t ds_clonegc_delete(ds_clonegc_t *);
extern errno_t ds_clonegc_add_output(ds_clonegc_t *, gfx_context_t *);
extern gfx_context_t *ds_clonegc_get_ctx(ds_clonegc_t *);

#endif

/** @}
 */
