/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphics context
 *
 * A graphics context is the target of rendering operations. It can carry
 * some additional state (hence context). It is an abstract interface,
 * to be implemented by various backends (drivers).
 */

#ifndef _GFX_TYPES_CONTEXT_H
#define _GFX_TYPES_CONTEXT_H

struct gfx_context;
typedef struct gfx_context gfx_context_t;

#endif

/** @}
 */
