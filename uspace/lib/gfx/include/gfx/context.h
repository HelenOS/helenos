/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphic context
 *
 * A graphics context is the target of rendering operations. It can carry
 * some additional state (hence context). It is an abstract interface,
 * to be implemented by various backends (drivers).
 */

#ifndef _GFX_CONTEXT_H
#define _GFX_CONTEXT_H

#include <errno.h>
#include <types/gfx/context.h>
#include <types/gfx/ops/context.h>

extern errno_t gfx_context_new(gfx_context_ops_t *, void *,
    gfx_context_t **);
extern errno_t gfx_context_delete(gfx_context_t *);

#endif

/** @}
 */
