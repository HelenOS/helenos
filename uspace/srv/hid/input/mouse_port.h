/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Mouse port driver interface.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef MOUSE_PORT_H_
#define MOUSE_PORT_H_

#include <stdint.h>

struct mouse_dev;

typedef struct mouse_port_ops {
	errno_t (*init)(struct mouse_dev *);
	void (*write)(uint8_t);
} mouse_port_ops_t;

extern mouse_port_ops_t chardev_mouse_port;

#endif

/**
 * @}
 */
