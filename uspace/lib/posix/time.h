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
/** @file
 */

#ifndef POSIX_TIME_H_
#define POSIX_TIME_H_

#include "libc/time.h"
#include "sys/types.h"

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

#ifndef CLOCKS_PER_SEC
	#define CLOCKS_PER_SEC (1000000L)
#endif

#ifndef __locale_t_defined
	#define __locale_t_defined
	typedef struct __posix_locale *posix_locale_t;
	#ifndef LIBPOSIX_INTERNAL
		#define locale_t posix_locale_t
	#endif
#endif

#undef ASCTIME_BUF_LEN
#define ASCTIME_BUF_LEN 26

#undef CLOCK_REALTIME
#define CLOCK_REALTIME ((posix_clockid_t) 0)

struct posix_tm {
	int tm_sec;         /* Seconds [0,60]. */
	int tm_min;         /* Minutes [0,59]. */
	int tm_hour;        /* Hour [0,23]. */
	int tm_mday;        /* Day of month [1,31]. */
	int tm_mon;         /* Month of year [0,11]. */
	int tm_year;        /* Years since 1900. */
	int tm_wday;        /* Day of week [0,6] (Sunday = 0). */
	int tm_yday;        /* Day of year [0,365]. */
	int tm_isdst;       /* Daylight Savings flag. */
};

// FIXME: should be in sys/types.h
typedef long posix_clock_t;

struct posix_timespec {
	time_t tv_sec; /* Seconds. */
	long tv_nsec; /* Nanoseconds. */
};

struct posix_itimerspec {
	struct posix_timespec it_interval; /* Timer period. */
	struct posix_timespec it_value; /* Timer expiration. */
};

/* Timezones */

extern int posix_daylight;
extern long posix_timezone;
extern char *posix_tzname[2];

extern void posix_tzset(void);

/* time_t */

extern double posix_difftime(time_t time1, time_t time0);

/* Broken-down Time */
extern time_t posix_mktime(struct posix_tm *timeptr);
extern struct posix_tm *posix_localtime(const time_t *timep);
extern struct posix_tm *posix_localtime_r(const time_t *restrict timer,
    struct posix_tm *restrict result);
/* Formatting Calendar Time */
extern char *posix_asctime(const struct posix_tm *timeptr);
extern char *posix_asctime_r(const struct posix_tm *restrict timeptr,
    char *restrict buf);
extern char *posix_ctime(const time_t *timep);
extern size_t posix_strftime(char *restrict s, size_t maxsize,
    const char *restrict format, const struct posix_tm *restrict tm);

/* CPU Time */
extern posix_clock_t posix_clock(void);


#ifndef LIBPOSIX_INTERNAL
	#define tm posix_tm

	#define clock_t posix_clock_t
	#define timespec posix_timespec
	#define itimerspec posix_itimerspec

	#define difftime posix_difftime
	#define mktime posix_mktime
	#define localtime posix_localtime
	#define localtime_r posix_localtime_r

	#define daylight posix_daylight
	#define timezone posix_timezone
	#define tzname posix_tzname
	#define tzset posix_tzset

	#define asctime posix_asctime
	#define asctime_r posix_asctime_r
	#define ctime posix_ctime
	#define strftime posix_strftime

	#define clock posix_clock
#endif

#endif  // POSIX_TIME_H_

/** @}
 */
