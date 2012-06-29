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
#include <time.h>
#include <bool.h>
#include <libarch/barrier.h>
#include <macros.h>
#include <errno.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <libc.h>
#include <unistd.h>

/** Pointer to kernel shared variables with time */
struct {
	volatile sysarg_t seconds1;
	volatile sysarg_t useconds;
	volatile sysarg_t seconds2;
} *ktime = NULL;

/** Add microseconds to given timeval.
 *
 * @param tv    Destination timeval.
 * @param usecs Number of microseconds to add.
 *
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
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 *
 * @return Difference between tv1 and tv2 (tv1 - tv2) in
 *         microseconds.
 *
 */
suseconds_t tv_sub(struct timeval *tv1, struct timeval *tv2)
{
	return (tv1->tv_usec - tv2->tv_usec) +
	    ((tv1->tv_sec - tv2->tv_sec) * 1000000);
}

/** Decide if one timeval is greater than the other.
 *
 * @param t1 First timeval.
 * @param t2 Second timeval.
 *
 * @return True if tv1 is greater than tv2.
 * @return False otherwise.
 *
 */
int tv_gt(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return true;
	
	if ((tv1->tv_sec == tv2->tv_sec) && (tv1->tv_usec > tv2->tv_usec))
		return true;
	
	return false;
}

/** Decide if one timeval is greater than or equal to the other.
 *
 * @param tv1 First timeval.
 * @param tv2 Second timeval.
 *
 * @return True if tv1 is greater than or equal to tv2.
 * @return False otherwise.
 *
 */
int tv_gteq(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return true;
	
	if ((tv1->tv_sec == tv2->tv_sec) && (tv1->tv_usec >= tv2->tv_usec))
		return true;
	
	return false;
}

/** Get time of day
 *
 * The time variables are memory mapped (read-only) from kernel which
 * updates them periodically.
 *
 * As it is impossible to read 2 values atomically, we use a trick:
 * First we read the seconds, then we read the microseconds, then we
 * read the seconds again. If a second elapsed in the meantime, set
 * the microseconds to zero.
 *
 * This assures that the values returned by two subsequent calls
 * to gettimeofday() are monotonous.
 *
 */
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (ktime == NULL) {
		uintptr_t faddr;
		int rc = sysinfo_get_value("clock.faddr", &faddr);
		if (rc != EOK) {
			errno = rc;
			return -1;
		}
		
		void *addr;
		rc = physmem_map((void *) faddr, 1,
		    AS_AREA_READ | AS_AREA_CACHEABLE, &addr);
		if (rc != EOK) {
			as_area_destroy(addr);
			errno = rc;
			return -1;
		}
		
		ktime = addr;
	}
	
	if (tz) {
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = DST_NONE;
	}
	
	sysarg_t s2 = ktime->seconds2;
	
	read_barrier();
	tv->tv_usec = ktime->useconds;
	
	read_barrier();
	sysarg_t s1 = ktime->seconds1;
	
	if (s1 != s2) {
		tv->tv_sec = max(s1, s2);
		tv->tv_usec = 0;
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

/** Wait unconditionally for specified number of microseconds
 *
 */
int usleep(useconds_t usec)
{
	(void) __SYSCALL1(SYS_THREAD_USLEEP, usec);
	return 0;
}

void udelay(useconds_t time)
{
	(void) __SYSCALL1(SYS_THREAD_UDELAY, (sysarg_t) time);
}


/** Wait unconditionally for specified number of seconds
 *
 */
unsigned int sleep(unsigned int sec)
{
	/*
	 * Sleep in 1000 second steps to support
	 * full argument range
	 */
	
	while (sec > 0) {
		unsigned int period = (sec > 1000) ? 1000 : sec;
		
		usleep(period * 1000000);
		sec -= period;
	}
	
	return 0;
}

/** @}
 */
