/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup xtkbd
 * @{
 */
/** @file
 * @brief XT keyboard driver
 */

#ifndef XT_KBD_H_
#define XT_KBD_H_

#include <async.h>
#include <ddf/driver.h>
#include <fibril.h>
#include <io/chardev.h>

/** PC/XT keyboard driver structure */
typedef struct {
	/** Keyboard function */
	ddf_fun_t *kbd_fun;
	/** Device providing keyboard connection */
	chardev_t *chardev;
	/** Callback connection to client */
	async_sess_t *client_sess;
	/** Fibril retrieving an parsing data */
	fid_t polling_fibril;
} xt_kbd_t;

extern errno_t xt_kbd_init(xt_kbd_t *, ddf_dev_t *);

#endif

/**
 * @}
 */
