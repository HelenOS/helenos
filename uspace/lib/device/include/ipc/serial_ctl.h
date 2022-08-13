/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBDEVICE_IPC_SERIAL_CTL_H
#define LIBDEVICE_IPC_SERIAL_CTL_H

#include <ipc/chardev.h>

/** IPC methods for getting/setting serial communication properties
 *
 * 1st IPC arg: baud rate
 * 2nd IPC arg: parity
 * 3rd IPC arg: number of bits in one word
 * 4th IPC arg: number of stop bits
 *
 */
typedef enum {
	SERIAL_GET_COM_PROPS = CHARDEV_LIMIT,
	SERIAL_SET_COM_PROPS
} serial_ctl_t;

typedef enum {
	SERIAL_NO_PARITY = 0,
	SERIAL_ODD_PARITY = 1,
	SERIAL_EVEN_PARITY = 3,
	SERIAL_MARK_PARITY = 5,
	SERIAL_SPACE_PARITY = 7
} serial_parity_t;

#endif
