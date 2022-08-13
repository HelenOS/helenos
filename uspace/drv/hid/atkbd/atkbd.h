/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup atkbd
 * @{
 */
/** @file
 * @brief AT keyboard driver
 */

#ifndef AT_KBD_H_
#define AT_KBD_H_

#include <ddf/driver.h>
#include <fibril.h>
#include <io/chardev.h>

/** PC/AT keyboard driver structure. */
typedef struct {
	/** Keyboard function */
	ddf_fun_t *kbd_fun;
	/** Device providing keyboard connection */
	chardev_t *chardev;
	/** Callback connection to client */
	async_sess_t *client_sess;
	/** Fibril retrieving and parsing data */
	fid_t polling_fibril;
} at_kbd_t;

extern errno_t at_kbd_init(at_kbd_t *, ddf_dev_t *);

#endif

/**
 * @}
 */
