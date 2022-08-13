/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Keyboard port driver interface.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef KBD_PORT_H_
#define KBD_PORT_H_

#include <stdint.h>

struct kbd_dev;

typedef struct kbd_port_ops {
	errno_t (*init)(struct kbd_dev *);
	void (*write)(uint8_t);
} kbd_port_ops_t;

extern kbd_port_ops_t chardev_port;
extern kbd_port_ops_t ns16550_port;

#endif

/**
 * @}
 */
