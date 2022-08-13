/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Keyboard controller driver interface.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef KBD_CTL_H_
#define KBD_CTL_H_

#include "kbd_port.h"

struct kbd_dev;

typedef struct kbd_ctl_ops {
	void (*parse)(sysarg_t);
	errno_t (*init)(struct kbd_dev *);
	void (*set_ind)(struct kbd_dev *, unsigned int);
} kbd_ctl_ops_t;

extern kbd_ctl_ops_t kbdev_ctl;
extern kbd_ctl_ops_t stty_ctl;
extern kbd_ctl_ops_t sun_ctl;

#endif

/**
 * @}
 */
