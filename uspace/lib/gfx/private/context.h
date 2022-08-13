/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphics context structure
 *
 */

#ifndef _GFX_PRIVATE_CONTEXT_H
#define _GFX_PRIVATE_CONTEXT_H

#include <types/gfx/ops/context.h>

/** Actual structure of graphics context.
 *
 * This is private to libgfx. It is not visible to clients nor backends.
 */
struct gfx_context {
	/** Graphics context ops */
	gfx_context_ops_t *ops;
	/** Instance argument */
	void *arg;
};

#endif

/** @}
 */
