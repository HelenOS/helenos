/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_time
 * @{
 */
/** @file
 */

#ifndef KERN_CLOCK_H_
#define KERN_CLOCK_H_

#include <typedefs.h>

#define HZ  100

/** Uptime structure */
typedef struct {
	sysarg_t seconds1;
	sysarg_t useconds;
	sysarg_t seconds2;
} uptime_t;

extern uptime_t *uptime;

extern void clock(void);
extern void clock_counter_init(void);

#endif

/** @}
 */
