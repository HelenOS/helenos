/*
 * SPDX-FileCopyrightText: 2017 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */

#ifndef POSIX_SYS_TIME_H_
#define POSIX_SYS_TIME_H_

#include <time.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

struct timeval {
	time_t tv_sec;        /* seconds */
	suseconds_t tv_usec;  /* microseconds */
};

extern int gettimeofday(struct timeval *, void *);

__C_DECLS_END;

#endif

/** @}
 */
