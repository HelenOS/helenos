/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Serial device handling.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <async.h>
#include <io/chardev.h>
#include "kbd.h"

typedef struct serial_dev {
	kbd_dev_t *kdev;
	link_t link;
	async_sess_t *sess;
	chardev_t *chardev;
} serial_dev_t;

#endif

/**
 * @}
 */
