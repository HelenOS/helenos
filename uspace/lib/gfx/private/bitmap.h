/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Bitmap structure
 *
 */

#ifndef _GFX_PRIVATE_BITMAP_H
#define _GFX_PRIVATE_BITMAP_H

#include <types/gfx/context.h>

/** Bitmap
 *
 * This is private to libgfx. It is not visible to clients nor backends.
 */
struct gfx_bitmap {
	/** Graphics context of the bitmap */
	gfx_context_t *gc;
	/** GC-private data */
	void *gc_priv;
};

#endif

/** @}
 */
