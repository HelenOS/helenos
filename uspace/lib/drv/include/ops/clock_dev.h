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

#ifndef LIBDRV_OPS_CLOCK_DEV_H_
#define LIBDRV_OPS_CLOCK_DEV_H_

#include <time.h>
#include "../ddf/driver.h"

typedef struct {
	errno_t (*time_get)(ddf_fun_t *, struct tm *);
	errno_t (*time_set)(ddf_fun_t *, struct tm *);
} clock_dev_ops_t;

#endif

/**
 * @}
 */
