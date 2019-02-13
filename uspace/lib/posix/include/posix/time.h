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

#ifndef POSIX_TIME_H_
#define POSIX_TIME_H_

#include <stddef.h>
#include <sys/types.h>
#include <libc/time.h>

#define CLOCK_REALTIME ((clockid_t) 0)

#define ASCTIME_BUF_LEN  26

__C_DECLS_BEGIN;

#ifndef __locale_t_defined
#define __locale_t_defined
typedef struct __posix_locale *locale_t;
#endif

struct sigevent;

struct itimerspec {
	struct timespec it_interval; /* Timer period. */
	struct timespec it_value; /* Timer expiration. */
};

typedef struct __posix_timer *timer_t;

/* Timezones */
extern int daylight;
extern long timezone;
extern char *tzname[2];
extern void tzset(void);

/* Time */
extern time_t time(time_t *t);

/* Broken-down Time */
extern struct tm *gmtime_r(const time_t *__restrict__ timer,
    struct tm *__restrict__ result);
extern struct tm *gmtime(const time_t *__restrict__ timep);
extern struct tm *localtime_r(const time_t *__restrict__ timer,
    struct tm *__restrict__ result);
extern struct tm *localtime(const time_t *__restrict__ timep);

/* Formatting Calendar Time */
extern char *asctime_r(const struct tm *__restrict__ timeptr,
    char *__restrict__ buf);
extern char *asctime(const struct tm *__restrict__ timeptr);
extern char *ctime_r(const time_t *timer, char *buf);
extern char *ctime(const time_t *timer);
extern time_t time(time_t *t);

/* Clocks */
extern int clock_getres(clockid_t clock_id,
    struct timespec *res);
extern int clock_gettime(clockid_t clock_id,
    struct timespec *tp);
extern int clock_settime(clockid_t clock_id,
    const struct timespec *tp);
extern int clock_nanosleep(clockid_t clock_id, int flags,
    const struct timespec *rqtp, struct timespec *rmtp);

__C_DECLS_END;

#endif  // POSIX_TIME_H_

/** @}
 */
