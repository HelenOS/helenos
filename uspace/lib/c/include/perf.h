/*
 * Copyright (c) 2018 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
