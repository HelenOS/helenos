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

#include <libc/time.h>

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

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

/* Broken-down Time */
extern struct posix_tm *posix_localtime(const time_t *timep);

/* Formatting Calendar Time */
extern char *posix_asctime(const struct posix_tm *tm);
extern char *posix_ctime(const time_t *timep);
extern size_t posix_strftime(char *restrict s, size_t maxsize, const char *restrict format, const struct posix_tm *restrict tm);

#ifndef LIBPOSIX_INTERNAL
	#define tm posix_tm

	#define localtime posix_localtime

	#define asctime posix_asctime
	#define ctime posix_ctime
	#define strftime posix_strftime
#endif

#endif  // POSIX_TIME_H_

/** @}
 */
