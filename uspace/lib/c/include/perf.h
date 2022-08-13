/*
 * SPDX-FileCopyrightText: 2018 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * System performance measurement utilities.
 */

#ifndef _LIBC_PERF_H_
#define _LIBC_PERF_H_

#include <time.h>

/** Stopwatch is THE way to measure elapsed time on HelenOS. */
typedef struct {
	struct timespec start;
	struct timespec end;
} stopwatch_t;

#define STOPWATCH_INITIALIZE_STATIC { { 0, 0 }, { 0, 0 } }

/** Initialize system stopwatch.
 *
 * @param stopwatch Stopwatch to initialize.
 */
static inline void stopwatch_init(stopwatch_t *stopwatch)
{
	stopwatch->start.tv_sec = 0;
	stopwatch->start.tv_nsec = 0;
	stopwatch->end.tv_sec = 0;
	stopwatch->end.tv_nsec = 0;
}

/** Emulate elapsed time for use in tests.
 *
 * @param stopwatch Stopwatch to use.
 * @param nanos Elapsed time in nanoseconds to set.
 */
static inline void stopwatch_set_nanos(stopwatch_t *stopwatch, nsec_t nanos)
{
	stopwatch->start.tv_sec = 0;
	stopwatch->start.tv_nsec = 0;
	stopwatch->end.tv_sec = NSEC2SEC(nanos);
	stopwatch->end.tv_nsec = nanos - SEC2NSEC(stopwatch->end.tv_sec);
}

/** Start the stopwatch.
 *
 * Note that repeated starts/stops do NOT aggregate the elapsed time.
 *
 * @param stopwatch Stopwatch to start.
 */
static inline void stopwatch_start(stopwatch_t *stopwatch)
{
	getuptime(&stopwatch->start);
}

/** Stop the stopwatch.
 *
 * Note that repeated starts/stops do NOT aggregate the elapsed time.
 *
 * @param stopwatch Stopwatch to stop.
 */
static inline void stopwatch_stop(stopwatch_t *stopwatch)
{
	getuptime(&stopwatch->end);
}

/** Get elapsed time in nanoseconds.
 *
 * @param stopwatch Stopwatch to read from.
 * @return Elapsed time in nanoseconds.
 */
static inline nsec_t stopwatch_get_nanos(stopwatch_t *stopwatch)
{
	return ts_sub_diff(&stopwatch->end, &stopwatch->start);
}

#endif

/** @}
 */
