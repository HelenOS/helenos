/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#include <sys/types.h>

#define DST_NONE 0

typedef long time_t;
typedef long suseconds_t;

typedef uint32_t useconds_t;
typedef uint32_t mseconds_t;

struct timeval {
	time_t tv_sec;        /* seconds */
	suseconds_t tv_usec;  /* microseconds */
};

struct timezone {
	int tz_minuteswest;  /* minutes W of Greenwich */
	int tz_dsttime;      /* type of dst correction */
};

extern void tv_add(struct timeval *tv, suseconds_t usecs);
extern suseconds_t tv_sub(struct timeval *tv1, struct timeval *tv2);
extern int tv_gt(struct timeval *tv1, struct timeval *tv2);
extern int tv_gteq(struct timeval *tv1, struct timeval *tv2);
extern int gettimeofday(struct timeval *tv, struct timezone *tz);

extern void udelay(useconds_t);

#endif

/** @}
 */
