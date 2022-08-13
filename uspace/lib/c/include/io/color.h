/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_COLOR_H_
#define _LIBC_IO_COLOR_H_

typedef enum {
	COLOR_BLACK   = 0,
	COLOR_BLUE    = 1,
	COLOR_GREEN   = 2,
	COLOR_CYAN    = 3,
	COLOR_RED     = 4,
	COLOR_MAGENTA = 5,
	COLOR_YELLOW  = 6,
	COLOR_WHITE   = 7
} console_color_t;

typedef enum {
	CATTR_NORMAL = 0,
	CATTR_BRIGHT = 8,
	CATTR_BLINK  = 16
} console_color_attr_t;

#endif

/** @}
 */
