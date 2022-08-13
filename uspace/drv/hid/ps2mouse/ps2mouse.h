/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup ps2mouse
 * @{
 */
/** @file
 * @brief PS/2 mouse driver.
 */

#ifndef PS2MOUSE_H_
#define PS2MOUSE_H_

#include <ddf/driver.h>
#include <fibril.h>
#include <io/chardev.h>

/** PS/2 mouse driver structure. */
typedef struct {
	/** Mouse function. */
	ddf_fun_t *mouse_fun;
	/** Device providing mouse connection */
	chardev_t *chardev;
	/** Callback connection to client. */
	async_sess_t *client_sess;
	/** Fibril retrieving an parsing data. */
	fid_t polling_fibril;
} ps2_mouse_t;

extern errno_t ps2_mouse_init(ps2_mouse_t *, ddf_dev_t *);

#endif

/**
 * @}
 */
