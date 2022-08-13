/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Color structure
 *
 */

#ifndef _GFX_PRIVATE_COLOR_H
#define _GFX_PRIVATE_COLOR_H

#include <stdint.h>

/** Actual structure of graphics color.
 *
 * This is private to libgfx. It is not visible to clients nor backends.
 */
struct gfx_color {
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint8_t attr;
};

#endif

/** @}
 */
