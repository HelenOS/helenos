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

#include <sys/time.h>
#include <unistd.h>
#include <ipc/ipc.h>
#include <stdio.h>
#include <arch/barrier.h>
#include <unistd.h>
#include <atomic.h>
#include <sysinfo.h>
#include <ipc/services.h>
#include <libc.h>

#include <sysinfo.h>
#include <as.h>
#include <ddi.h>

#include <time.h>

/* Pointers to public variables with time */
struct {
	volatile sysarg_t seconds1;
	volatile sysarg_t useconds;
	volatile sysarg_t seconds2;
} *ktime = NULL;

/** Add microseconds to given timeval.
 *
 * @param tv		Destination timeval.
 * @param usecs		Number of microseconds to add.
 */
void tv_add(struct timeval *tv, suseconds_t usecs)
{
	tv->tv_sec += usecs / 1000000;
	tv->tv_usec += usecs % 1000000;
	if (tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
}

/** Subtract two timevals.
 *
 * @param tv1		First timeval.
 * @param tv2		Second timeval.
 *
 * @return		Return difference between tv1 and tv2 (tv1 - tv2) in
 * 			microseconds.
 */
suseconds_t tv_sub(struct timeval *tv1, struct timeval *tv2)
{
	suseconds_t result;

	result = tv1->tv_usec - tv2->tv_usec;
	result += (tv1->tv_sec - tv2->tv_sec) * 1000000;

	return result;
}

/** Decide if one timeval is greater than the other.
 *
 * @param t1		First timeval.
 * @param t2		Second timeval.
 *
 * @return		Return true tv1 is greater than tv2. Otherwise return
 * 			false.
 */
int tv_gt(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec > tv2->tv_usec)
		return 1;
	return 0;
}

/** Decide if one timeval is greater than or equal to the other.
 *
 * @param tv1		First timeval.
 * @param tv2		Second timeval.
 *
 * @return		Return true if tv1 is greater than or equal to tv2.
 * 			Otherwise return false.
 */
int tv_gteq(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec >= tv2->tv_usec)
		return 1;
	return 0;
}


/** POSIX gettimeofday
 *
 * The time variables are memory mapped(RO) from kernel, which updates
 * them periodically. As it is impossible to read 2 values atomically, we
 * use a trick: First read a seconds, then read microseconds, then
 * read seconds again. If a second elapsed in the meantime, set it to zero. 
 * This provides assurance, that at least the
 * sequence of subsequent gettimeofday calls is ordered.
 */
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	void *mapping;
	sysarg_t s1, s2;
	int rights;
	int res;

	if (!ktime) {
		mapping = as_get_mappable_page(PAGE_SIZE);
		/* Get the mapping of kernel clock */
		res = ipc_share_in_start_1_1(PHONE_NS, mapping, PAGE_SIZE,
		    SERVICE_MEM_REALTIME, &rights);
		if (res) {
			printf("Failed to initialize timeofday memarea\n");
			_exit(1);
		}
		if (!(rights & AS_AREA_READ)) {
			printf("Received bad rights on time area: %X\n",
			    rights);
			as_area_destroy(mapping);
			_exit(1);
		}
		ktime = mapping;
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

time_t time(time_t *tloc)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL))
		return (time_t) -1;
	if (tloc)
		*tloc = tv.tv_sec;
	return tv.tv_sec;
}

/** Wait unconditionally for specified number of microseconds */
int usleep(useconds_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
	return 0;
}

/** Wait unconditionally for specified number of seconds */
unsigned int sleep(unsigned int sec)
{
	/* Sleep in 1000 second steps to support
	   full argument range */
	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;
	
		usleep(period * 1000000);
		sec -= period;
	}
	return 0;
}

/** @}
 */
