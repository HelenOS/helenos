/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_genarch
 * @{
 */
/** @file
 */

#ifndef _ABI_VISUALS_H_
#define _ABI_VISUALS_H_

typedef enum {
	VISUAL_UNKNOWN = 0,
	VISUAL_INDIRECT_8,
	VISUAL_RGB_5_5_5_LE,
	VISUAL_RGB_5_5_5_BE,
	VISUAL_RGB_5_6_5_LE,
	VISUAL_RGB_5_6_5_BE,
	VISUAL_BGR_8_8_8,
	VISUAL_BGR_0_8_8_8,
	VISUAL_BGR_8_8_8_0,
	VISUAL_ABGR_8_8_8_8,
	VISUAL_BGRA_8_8_8_8,
	VISUAL_RGB_8_8_8,
	VISUAL_RGB_0_8_8_8,
	VISUAL_RGB_8_8_8_0,
	VISUAL_ARGB_8_8_8_8,
	VISUAL_RGBA_8_8_8_8
} visual_t;

#endif

/** @}
 */
