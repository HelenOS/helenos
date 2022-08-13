/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
