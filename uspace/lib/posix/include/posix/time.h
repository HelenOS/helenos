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

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "sys/types.h"

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

#ifndef CLOCKS_PER_SEC
	#define CLOCKS_PER_SEC (1000000L)
#endif

#ifndef __locale_t_defined
	#define __locale_t_defined
	typedef struct __posix_locale *__POSIX_DEF__(locale_t);
	#ifndef LIBPOSIX_INTERNAL
		#define locale_t __POSIX_DEF__(locale_t)
	#endif
#endif

#ifndef POSIX_SIGNAL_H_
	struct __POSIX_DEF__(sigevent);
	#ifndef LIBPOSIX_INTERNAL
		#define sigevent __POSIX_DEF__(sigevent)
	#endif
#endif

#undef CLOCK_REALTIME
#define CLOCK_REALTIME ((__POSIX_DEF__(clockid_t)) 0)

struct __POSIX_DEF__(timespec) {
	time_t tv_sec; /* Seconds. */
	long tv_nsec; /* Nanoseconds. */
};

struct __POSIX_DEF__(itimerspec) {
	struct __POSIX_DEF__(timespec) it_interval; /* Timer period. */
	struct __POSIX_DEF__(timespec) it_value; /* Timer expiration. */
};

typedef struct __posix_timer *__POSIX_DEF__(timer_t);

/* Timezones */
extern int __POSIX_DEF__(daylight);
extern long __POSIX_DEF__(timezone);
extern char *__POSIX_DEF__(tzname)[2];
extern void __POSIX_DEF__(tzset)(void);

/* Time */
extern time_t __POSIX_DEF__(time)(time_t *t);

/* Broken-down Time */
extern struct tm *__POSIX_DEF__(gmtime_r)(const time_t *restrict timer,
    struct tm *restrict result);
extern struct tm *__POSIX_DEF__(gmtime)(const time_t *restrict timep);
extern struct tm *__POSIX_DEF__(localtime_r)(const time_t *restrict timer,
    struct tm *restrict result);
extern struct tm *__POSIX_DEF__(localtime)(const time_t *restrict timep);

/* Formatting Calendar Time */
extern char *__POSIX_DEF__(asctime_r)(const struct tm *restrict timeptr,
    char *restrict buf);
extern char *__POSIX_DEF__(asctime)(const struct tm *restrict timeptr);
extern char *__POSIX_DEF__(ctime_r)(const time_t *timer, char *buf);
extern char *__POSIX_DEF__(ctime)(const time_t *timer);
extern time_t time(time_t *t);

/* Clocks */
extern int __POSIX_DEF__(clock_getres)(__POSIX_DEF__(clockid_t) clock_id,
    struct __POSIX_DEF__(timespec) *res);
extern int __POSIX_DEF__(clock_gettime)(__POSIX_DEF__(clockid_t) clock_id,
    struct __POSIX_DEF__(timespec) *tp);
extern int __POSIX_DEF__(clock_settime)(__POSIX_DEF__(clockid_t) clock_id,
    const struct __POSIX_DEF__(timespec) *tp); 
extern int __POSIX_DEF__(clock_nanosleep)(__POSIX_DEF__(clockid_t) clock_id, int flags,
    const struct __POSIX_DEF__(timespec) *rqtp, struct __POSIX_DEF__(timespec) *rmtp);

/* CPU Time */
extern __POSIX_DEF__(clock_t) __POSIX_DEF__(clock)(void);


#endif  // POSIX_TIME_H_

/** @}
 */
