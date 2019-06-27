/*
 * Copyright (c) 2018 Jakub Jermar
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

#ifndef _LIBC_TIME_H_
#define _LIBC_TIME_H_

#include <_bits/decls.h>

/* ISO/IEC 9899:2011 7.27.1 (2) */

#include <_bits/NULL.h>

#define CLOCKS_PER_SEC	((clock_t) 1000000)

#define TIME_UTC	1

/* ISO/IEC 9899:2011 7.27.1 (3) */

#include <_bits/size_t.h>

__C_DECLS_BEGIN;

/* ISO/IEC 9899:2011 7.27.1 (3), (4) */

typedef long long time_t;
typedef long long clock_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct tm {
	int tm_sec;
	int tm_nsec;  /**< Nonstandard extension, nanoseconds since last second. */
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

/* ISO/IEC 9899:2011 7.27.2.1 (1) */
extern clock_t clock(void);

/* ISO/IEC 9899:2011 7.27.2.2 (1) */
extern double difftime(time_t, time_t);

/* ISO/IEC 9899:2011 7.27.2.3 (1) */
extern time_t mktime(struct tm *);

/* ISO/IEC 9899:2011 7.27.2.4 (1) */
extern time_t time(time_t *);

/* ISO/IEC 9899:2011 7.27.2.5 (1) */
extern int timespec_get(struct timespec *, int);

/* ISO/IEC 9899:2011 7.27.3.1 (1) */
extern char *asctime(const struct tm *);

/* ISO/IEC 9899:2011 7.27.3.2 (1) */
extern char *ctime(const time_t *);

/* ISO/IEC 9899:2011 7.27.3.3 (1) */
extern struct tm *gmtime(const time_t *);

/* ISO/IEC 9899:2011 7.27.3.4 (1) */
extern struct tm *localtime(const time_t *);

/* ISO/IEC 9899:2011 7.27.3.5 (1) */
extern size_t strftime(char *__restrict__, size_t, const char *__restrict__,
    const struct tm *__restrict__);

__C_DECLS_END;

#ifdef _HELENOS_SOURCE

/*
 * HelenOS specific extensions
 */

#include <stdbool.h>
#include <_bits/errno.h>

__HELENOS_DECLS_BEGIN;

typedef long long sec_t;
typedef long long msec_t;
typedef long long usec_t;
typedef long long nsec_t;	/* good for +/- 292 years */

#define SEC2MSEC(s)	((s) * 1000ll)
#define SEC2USEC(s)	((s) * 1000000ll)
#define SEC2NSEC(s)	((s) * 1000000000ll)

#define MSEC2SEC(ms)	((ms) / 1000ll)
#define MSEC2USEC(ms)	((ms) * 1000ll)
#define MSEC2NSEC(ms)	((ms) * 1000000ll)

#define USEC2SEC(us)	((us) / 1000000ll)
#define USEC2MSEC(us)	((us) / 1000ll)
#define USEC2NSEC(us)	((us) * 1000ll)

#define NSEC2SEC(ns)	((ns) / 1000000000ll)
#define NSEC2MSEC(ns)	((ns) / 1000000ll)
#define NSEC2USEC(ns)	((ns) / 1000ll)

extern void getuptime(struct timespec *);
extern void getrealtime(struct timespec *);

extern void ts_add_diff(struct timespec *, nsec_t);
extern void ts_add(struct timespec *, const struct timespec *);
extern void ts_sub(struct timespec *, const struct timespec *);
extern nsec_t ts_sub_diff(const struct timespec *, const struct timespec *);
extern bool ts_gt(const struct timespec *, const struct timespec *);
extern bool ts_gteq(const struct timespec *, const struct timespec *);

extern errno_t time_utc2tm(const time_t, struct tm *);
extern errno_t time_utc2str(const time_t, char *);
extern void time_tm2str(const struct tm *, char *);
extern errno_t time_ts2tm(const struct timespec *, struct tm *);
extern errno_t time_local2tm(const time_t, struct tm *);
extern errno_t time_local2str(const time_t, char *);

extern void udelay(sysarg_t);

__HELENOS_DECLS_END;

#endif /* _HELENOS_SOURCE */

#endif

/** @}
 */
