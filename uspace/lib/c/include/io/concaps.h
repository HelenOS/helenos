/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_CONCAPS_H_
#define _LIBC_IO_CONCAPS_H_

typedef enum {
	CONSOLE_CAP_NONE = 0,
	CONSOLE_CAP_STYLE = 1,
	CONSOLE_CAP_INDEXED = 2,
	CONSOLE_CAP_RGB = 4
} console_caps_t;

#endif

/** @}
 */
