/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libdevice
 * @{
 */

#ifndef LIBDEVICE_TYPES_IO_CHARDEV_H
#define LIBDEVICE_TYPES_IO_CHARDEV_H

/** Chardev read/write operation flags */
typedef enum {
	/** No flags */
	chardev_f_none = 0,
	/** Do not block even if no bytes can be transferred */
	chardev_f_nonblock = 0x1
} chardev_flags_t;

#endif
/**
 * @}
 */
