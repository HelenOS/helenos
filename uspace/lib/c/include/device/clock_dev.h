/*
 * SPDX-FileCopyrightText: 2012 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_DEVICE_CLOCK_DEV_H_
#define _LIBC_DEVICE_CLOCK_DEV_H_

#include <async.h>
#include <time.h>

typedef enum {
	CLOCK_DEV_TIME_GET = 0,
	CLOCK_DEV_TIME_SET,
} clock_dev_method_t;

extern errno_t clock_dev_time_get(async_sess_t *, struct tm *);
extern errno_t clock_dev_time_set(async_sess_t *, struct tm *);

#endif
