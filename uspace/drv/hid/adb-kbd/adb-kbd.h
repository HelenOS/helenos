/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef ADB_KBD_H
#define ADB_KBD_H

#include <async.h>
#include <ddf/driver.h>

/** ADB keyboard */
typedef struct {
	ddf_dev_t *dev;
	async_sess_t *parent_sess;
	ddf_fun_t *fun;
	async_sess_t *client_sess;
} adb_kbd_t;

extern errno_t adb_kbd_add(adb_kbd_t *);
extern errno_t adb_kbd_remove(adb_kbd_t *);
extern errno_t adb_kbd_gone(adb_kbd_t *);

#endif

/** @}
 */
