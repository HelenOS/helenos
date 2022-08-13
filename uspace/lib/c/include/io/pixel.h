/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_IO_PIXEL_H_
#define _LIBC_IO_PIXEL_H_

#include <stdint.h>

#define NARROW(channel, bits) \
	((channel) >> (8 - (bits)))

#define INVERT(pixel) ((pixel) ^ 0x00ffffff)

#define ALPHA(pixel)  ((pixel) >> 24)
#define RED(pixel)    (((pixel) & 0x00ff0000) >> 16)
#define GREEN(pixel)  (((pixel) & 0x0000ff00) >> 8)
#define BLUE(pixel)   ((pixel) & 0x000000ff)

#define PIXEL(a, r, g, b) \
	((((unsigned)(a) & 0xff) << 24) | (((unsigned)(r) & 0xff) << 16) | \
	(((unsigned)(g) & 0xff) << 8) | ((unsigned)(b) & 0xff))

typedef uint32_t pixel_t;

#endif

/** @}
 */
