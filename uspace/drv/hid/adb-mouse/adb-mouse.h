/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file ADB mouse driver
 */

#ifndef ADB_MOUSE_H
#define ADB_MOUSE_H

#include <async.h>
#include <ddf/driver.h>
#include <stdbool.h>

/** ADB mouse */
typedef struct {
	ddf_dev_t *dev;
	async_sess_t *parent_sess;
	ddf_fun_t *fun;
	async_sess_t *client_sess;
	bool b1_pressed;
	bool b2_pressed;
} adb_mouse_t;

extern errno_t adb_mouse_add(adb_mouse_t *);
extern errno_t adb_mouse_remove(adb_mouse_t *);
extern errno_t adb_mouse_gone(adb_mouse_t *);

#endif

/** @}
 */
