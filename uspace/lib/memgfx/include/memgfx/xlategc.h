/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libmemgfx
 * @{
 */
/**
 * @file Translating graphics context
 */

#ifndef _MEMGFX_XLATEGC_H
#define _MEMGFX_XLATEGC_H

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <types/memgfx/xlategc.h>

extern gfx_context_ops_t xlate_gc_ops;

extern errno_t xlate_gc_create(gfx_coord2_t *, gfx_context_t *, xlate_gc_t **);
extern errno_t xlate_gc_delete(xlate_gc_t *);
extern gfx_context_t *xlate_gc_get_ctx(xlate_gc_t *);
extern void xlate_gc_set_off(xlate_gc_t *, gfx_coord2_t *);

#endif

/** @}
 */
