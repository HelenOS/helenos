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

#ifndef LIBDRV_OPS_BATTERY_DEV_H_
#define LIBDRV_OPS_BATTERY_DEV_H_

#include "../ddf/driver.h"
#include "battery_iface.h"

typedef struct {
	errno_t (*battery_status_get)(ddf_fun_t *, battery_status_t *);
	errno_t (*battery_charge_level_get)(ddf_fun_t *, int *);
} battery_dev_ops_t;

#endif

/**
 * @}
 */
