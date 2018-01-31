/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_SYS_TIME_H_
#define LIBC_SYS_TIME_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define DST_NONE  0
#define ASCTIME_BUF_LEN  26

typedef long time_t;
typedef long suseconds_t;

typedef uint32_t useconds_t;
typedef uint32_t mseconds_t;

struct tm {
	int tm_usec;   /* Microseconds [0,999999]. */
	int tm_sec;    /* Seconds [0,60]. */
	int tm_min;    /* Minutes [0,59]. */
	int tm_hour;   /* Hour [0,23]. */
	int tm_mday;   /* Day of month [1,31]. */
	int tm_mon;    /* Month of year [0,11]. */
	int tm_year;   /* Years since 1900. */
	int tm_wday;   /* Day of week [0,6] (Sunday = 0). */
	int tm_yday;   /* Day of year [0,365]. */
	int tm_isdst;  /* Daylight Savings flag. */
};

struct timeval {
	time_t tv_sec;        /* seconds */
	suseconds_t tv_usec;  /* microseconds */
};

struct timezone {
	int tz_minuteswest;  /* minutes W of Greenwich */
	int tz_dsttime;      /* type of dst correction */
};

extern void tv_add_diff(struct timeval *, suseconds_t);
extern void tv_add(struct timeval *, struct timeval *);
extern suseconds_t tv_sub_diff(struct timeval *, struct timeval *);
extern void tv_sub(struct timeval *, struct timeval *);
extern int tv_gt(struct timeval *, struct timeval *);
extern int tv_gteq(struct timeval *, struct timeval *);
extern void gettimeofday(struct timeval *, struct timezone *);
extern void getuptime(struct timeval *);

extern void udelay(useconds_t);
extern errno_t usleep(useconds_t);

extern time_t mktime(struct tm *);
extern errno_t time_utc2tm(const time_t, struct tm *);
extern errno_t time_utc2str(const time_t, char *);
extern void time_tm2str(const struct tm *, char *);
extern errno_t time_tv2tm(const struct timeval *, struct tm *);
extern errno_t time_local2tm(const time_t, struct tm *);
extern errno_t time_local2str(const time_t, char *);
extern double difftime(time_t, time_t);
extern size_t strftime(char *__restrict__, size_t, const char *__restrict__,
    const struct tm *__restrict__);

#endif

/** @}
 */
