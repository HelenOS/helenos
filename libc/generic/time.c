/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#include <time.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <stdio.h>
#include <arch/barrier.h>

#include <sysinfo.h>
#include <as.h>
#include <ddi.h>

/* Pointers to public variables with time */
struct {
	volatile sysarg_t seconds1;
	volatile sysarg_t useconds;
	volatile sysarg_t seconds2;
} *ktime = NULL;


/** POSIX gettimeofday
 *
 * The time variables are memory mapped(RO) from kernel, which updates
 * them periodically. As it is impossible to read 2 values atomically, we
 * use a trick: First read a seconds, then read microseconds, then
 * read seconds again. If a second elapsed in the meantime, read
 * useconds again. This provides assurance, that at least the
 * sequence of subsequent gettimeofday calls is ordered.
 */
#define TMAREA (100*1024*1024)
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	void *mapping;
	sysarg_t s1, s2;
	sysarg_t t1;
	int res;

	if (!ktime) {
		/* TODO: specify better, where to map the area */
		/* Get the mapping of kernel clock */
		res = ipc_call_sync_3(PHONE_NS, IPC_M_AS_AREA_RECV, 
				      TMAREA,
				      PAGE_SIZE,
				      AS_AREA_READ | AS_AREA_CACHEABLE,
				      &t1,&t1,&t1);
		if (res) {
			printf("Failed to initialize timeofday memarea\n");
			_exit(1);
		}
		ktime = (void *) (TMAREA);
	}
	if (tz) {
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = DST_NONE;
	}

	s2 = ktime->seconds2;
	read_barrier();
	tv->tv_usec = ktime->useconds;
	read_barrier();
	s1 = ktime->seconds1;
	if (s1 != s2) {
		tv->tv_usec = 0;
		tv->tv_sec = s1 > s2 ? s1 : s2;
	} else
		tv->tv_sec = s1;

	return 0;
}
