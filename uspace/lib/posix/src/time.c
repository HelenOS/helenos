/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2011 Jiri Zarevucky
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

/** @addtogroup libposix
 * @{
 */
/** @file Time measurement support.
 */

#include "internal/common.h"
#include "posix/time.h"

#include "posix/ctype.h"

#include <errno.h>

#include "posix/signal.h"
#include <assert.h>

#include "libc/async.h"
#include "libc/malloc.h"
#include "libc/task.h"
#include "libc/stats.h"
#include "libc/stddef.h"
#include "libc/sys/time.h"

// TODO: test everything in this file

/* In some places in this file, phrase "normalized broken-down time" is used.
 * This means time broken down to components (year, month, day, hour, min, sec),
 * in which every component is in its proper bounds. Non-normalized time could
 * e.g. be 2011-54-5 29:13:-5, which would semantically mean start of year 2011
 * + 53 months + 4 days + 29 hours + 13 minutes - 5 seconds.
 */

int posix_daylight;
long posix_timezone;
char *posix_tzname[2];

/**
 * Set timezone conversion information.
 */
void tzset(void)
{
	// TODO: read environment
	posix_tzname[0] = (char *) "GMT";
	posix_tzname[1] = (char *) "GMT";
	posix_daylight = 0;
	posix_timezone = 0;
}

/**
 * Converts a time value to a broken-down UTC time.
 * 
 * @param timer Time to convert.
 * @param result Structure to store the result to.
 * @return Value of result on success, NULL on overflow.
 */
struct tm *gmtime_r(const time_t *restrict timer,
    struct tm *restrict result)
{
	if (failed(time_utc2tm(*timer, result))) {
		return NULL;
	}

	return result;
}

/**
 * Converts a time value to a broken-down UTC time.
 * (non reentrant version)
 *
 * @param timep  Time to convert
 * @return       Pointer to a statically allocated structure that stores
 *               the result, NULL in case of error.
 */
struct tm *gmtime(const time_t *restrict timep)
{
	static struct tm result;

	return gmtime_r(timep, &result);
}

/**
 * Converts a time value to a broken-down local time.
 * 
 * @param timer Time to convert.
 * @param result Structure to store the result to.
 * @return Value of result on success, NULL on overflow.
 */
struct tm *localtime_r(const time_t *restrict timer,
    struct tm *restrict result)
{
	// TODO: deal with timezone
	// currently assumes system and all times are in GMT
	return gmtime_r(timer, result);
}

/**
 * Converts a time value to a broken-down local time.
 * (non reentrant version)
 *
 * @param timep    Time to convert.
 * @return         Pointer to a statically allocated structure that stores
 *                 the result, NULL in case of error.
 */
struct tm *localtime(const time_t *restrict timep)
{
	static struct tm result;

	return localtime_r(timep, &result);
}

/**
 * Converts broken-down time to a string in format
 * "Sun Jan 1 00:00:00 1970\n". (Obsolete)
 *
 * @param timeptr Broken-down time structure.
 * @param buf Buffer to store string to, must be at least ASCTIME_BUF_LEN
 *     bytes long.
 * @return Value of buf.
 */
char *asctime_r(const struct tm *restrict timeptr,
    char *restrict buf)
{
	time_tm2str(timeptr, buf);
	return buf;
}

/**
 * Convers broken-down time to a string in format
 * "Sun Jan 1 00:00:00 1970\n". (Obsolete)
 * (non reentrant version)
 *
 * @param timeptr    Broken-down time structure.
 * @return           Pointer to a statically allocated buffer that stores
 *                   the result, NULL in case of error.
 */
char *asctime(const struct tm *restrict timeptr)
{
	static char buf[ASCTIME_BUF_LEN];

	return asctime_r(timeptr, buf);
}

/**
 * Converts the calendar time to a string in format
 * "Sun Jan 1 00:00:00 1970\n" (Obsolete)
 * 
 * @param timer Time to convert.
 * @param buf Buffer to store string to. Must be at least ASCTIME_BUF_LEN
 *     bytes long.
 * @return Pointer to buf on success, NULL on failure.
 */
char *ctime_r(const time_t *timer, char *buf)
{
	if (failed(time_local2str(*timer, buf))) {
		return NULL;
	}

	return buf;
}

/**
 * Converts the calendar time to a string in format
 * "Sun Jan 1 00:00:00 1970\n" (Obsolete)
 * (non reentrant version)
 *
 * @param timep    Time to convert.
 * @return         Pointer to a statically allocated buffer that stores
 *                 the result, NULL in case of error.
 */
char *ctime(const time_t *timep)
{
	static char buf[ASCTIME_BUF_LEN];

	return ctime_r(timep, buf);
}

/**
 * Get clock resolution. Only CLOCK_REALTIME is supported.
 *
 * @param clock_id Clock ID.
 * @param res Pointer to the variable where the resolution is to be written.
 * @return 0 on success, -1 with errno set on failure.
 */
int clock_getres(clockid_t clock_id, struct timespec *res)
{
	assert(res != NULL);

	switch (clock_id) {
		case CLOCK_REALTIME:
			res->tv_sec = 0;
			res->tv_nsec = 1000; /* Microsecond resolution. */
			return 0;
		default:
			errno = EINVAL;
			return -1;
	}
}

/**
 * Get time. Only CLOCK_REALTIME is supported.
 * 
 * @param clock_id ID of the clock to query.
 * @param tp Pointer to the variable where the time is to be written.
 * @return 0 on success, -1 with errno on failure.
 */
int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	assert(tp != NULL);

	switch (clock_id) {
		case CLOCK_REALTIME:
			;
			struct timeval tv;
			gettimeofday(&tv, NULL);
			tp->tv_sec = tv.tv_sec;
			tp->tv_nsec = tv.tv_usec * 1000;
			return 0;
		default:
			errno = EINVAL;
			return -1;
	}
}

/**
 * Set time on a specified clock. As HelenOS doesn't support this yet,
 * this function always fails.
 * 
 * @param clock_id ID of the clock to set.
 * @param tp Time to set.
 * @return 0 on success, -1 with errno on failure.
 */
int clock_settime(clockid_t clock_id,
    const struct timespec *tp)
{
	assert(tp != NULL);

	switch (clock_id) {
		case CLOCK_REALTIME:
			// TODO: setting clock
			// FIXME: HelenOS doesn't actually support hardware
			//        clock yet
			errno = EPERM;
			return -1;
		default:
			errno = EINVAL;
			return -1;
	}
}

/**
 * Sleep on a specified clock.
 * 
 * @param clock_id ID of the clock to sleep on (only CLOCK_REALTIME supported).
 * @param flags Flags (none supported).
 * @param rqtp Sleep time.
 * @param rmtp Remaining time is written here if sleep is interrupted.
 * @return 0 on success, -1 with errno set on failure.
 */
int clock_nanosleep(clockid_t clock_id, int flags,
    const struct timespec *rqtp, struct timespec *rmtp)
{
	assert(rqtp != NULL);
	assert(rmtp != NULL);

	switch (clock_id) {
		case CLOCK_REALTIME:
			// TODO: interruptible sleep
			if (rqtp->tv_sec != 0) {
				async_sleep(rqtp->tv_sec);
			}
			if (rqtp->tv_nsec != 0) {
				async_usleep(rqtp->tv_nsec / 1000);
			}
			return 0;
		default:
			errno = EINVAL;
			return -1;
	}
}

/**
 * Get CPU time used since the process invocation.
 *
 * @return Consumed CPU cycles by this process or -1 if not available.
 */
clock_t clock(void)
{
	clock_t total_cycles = -1;
	stats_task_t *task_stats = stats_get_task(task_get_id());
	if (task_stats) {
		total_cycles = (clock_t) (task_stats->kcycles +
		    task_stats->ucycles);
		free(task_stats);
		task_stats = 0;
	}

	return total_cycles;
}

/** @}
 */
