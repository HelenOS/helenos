/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_BATTERY_IFACE_H_
#define LIBDRV_BATTERY_IFACE_H_

#include <async.h>

typedef enum {
	BATTERY_OK,
	BATTERY_LOW,
	BATTERY_NOT_PRESENT,
} battery_status_t;

typedef enum {
	BATTERY_STATUS_GET = 0,
	BATTERY_CHARGE_LEVEL_GET,
} battery_dev_method_t;

extern errno_t battery_status_get(async_sess_t *, battery_status_t *);
extern errno_t battery_charge_level_get(async_sess_t *, int *);

#endif
