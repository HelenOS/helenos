/*
 * Copyright (C) 2005 Jakub Vana
 * Copyright (C) 2005 Jakub Jermar
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

#include <print.h>
#include <debug.h>
#include <panic.h>

#include <test.h>
#include <arch/atomic.h>
#include <proc/thread.h>

#include <arch.h>
#include <arch/arch.h>

#define THREADS		15*2
#define ATTEMPTS	10

#define E_10e8	271828182
#define PI_10e8	314159265


#ifdef __ia32_ARCH_H__
static inline double sqrt(double x) { double v; __asm__ ("fsqrt\n" : "=t" (v) : "0" (x)); return v; }
#endif

#ifdef __amd64_ARCH_H__
static inline double sqrt(double x) { double v; __asm__ ("fsqrt\n" : "=t" (v) : "0" (x)); return v; }
#endif

#ifdef __ia64_ARCH_H__
static inline long double sqrt(long double a) 
{   
	long double x =	1;
	long double lx = 0;

	if(a<0.00000000000000001) return 0;
		
	while(x!=lx)
	{
		lx=x;
		x=(x+(a/x))/2;
	}
	return x; 
}
#endif



static atomic_t threads_ok;
static waitq_t can_start;

static void e(void *data)
{
	int i;
	double e,d,le,f;

	waitq_sleep(&can_start);

	for (i = 0; i<ATTEMPTS; i++) {
		le=-1;
		e=0;
		f=1;

		for(d=1;e!=le;d*=f,f+=1) {
			le=e;
			e=e+1/d;
		}

		if((int)(100000000*e)!=E_10e8)
			panic("tid%d: e*10e8=%d should be %d\n", THREAD->tid, (__native) (100000000*e),(__native) E_10e8);
	}

	printf("tid%d: e*10e8=%d should be %d\n", THREAD->tid, (__native) (100000000*e),(__native) E_10e8);
	atomic_inc(&threads_ok);
}

static void pi(void *data)
{

#ifdef __ia64_ARCH_H__
#undef PI_10e8	
#define PI_10e8	3141592
#endif

	int i;
	double lpi, pi;
	double n, ab, ad;

	waitq_sleep(&can_start);


	for (i = 0; i<ATTEMPTS; i++) {
		lpi = -1;
		pi = 0;

		for (n=2, ab = sqrt(2); lpi != pi; n *= 2, ab = ad) {
			double sc, cd;

			sc = sqrt(1 - (ab*ab/4));
			cd = 1 - sc;
			ad = sqrt(ab*ab/4 + cd*cd);
			lpi = pi;
			pi = 2 * n * ad;
		}

#ifdef __ia64_ARCH_H__
		if((int)(1000000*pi)!=PI_10e8)
			panic("tid%d: pi*10e8=%d should be %d\n", THREAD->tid, (__native) (1000000*pi),(__native) (PI_10e8/100));
#else
		if((int)(100000000*pi)!=PI_10e8)
			panic("tid%d: pi*10e8=%d should be %d\n", THREAD->tid, (__native) (100000000*pi),(__native) PI_10e8);
#endif

	}

	printf("tid%d: pi*10e8=%d should be %d\n", THREAD->tid, (__native) (100000000*pi),(__native) PI_10e8);
	atomic_inc(&threads_ok);
}


void test(void)
{
	thread_t *t;
	int i;

	waitq_initialize(&can_start);

	printf("FPU test #1\n");
	printf("Creating %d threads... ", THREADS);

	for (i=0; i<THREADS/2; i++) {  
		if (!(t = thread_create(e, NULL, TASK, 0)))
			panic("could not create thread\n");
		thread_ready(t);
		if (!(t = thread_create(pi, NULL, TASK, 0)))
			panic("could not create thread\n");
		thread_ready(t);
	}
	printf("ok\n");
	
	thread_sleep(1);
	waitq_wakeup(&can_start, WAKEUP_ALL);

	while (atomic_get(&threads_ok) != THREADS)
		;
		
	printf("Test passed.\n");
}

/*
static void pi(void *data)
{
#undef PI_10e8	
#define PI_10e8	3141592


	int i;
	double lpi, pi;
	double n, ab, ad;


	printf("pi test\n");

	waitq_sleep(&can_start);


	for (i = 0; i<ATTEMPTS; i++) {
		lpi = -1;
		pi = 0;

		for (n=2, ab = sqrt(2); lpi != pi; n *= 2, ab = ad) {
			double sc, cd;

			sc = sqrt(1 - (ab*ab/4));
			cd = 1 - sc;
			ad = sqrt(ab*ab/4 + cd*cd);
			lpi = pi;
			pi = 2 * n * ad;
		}

		atomic_inc(&threads_ok);
		if((int)(1000000*pi)!=PI_10e8)
			panic("tid%d: pi*10e6=%d\n", THREAD->tid, (int) 1000000*pi);
	}

	printf("tid%d: pi*10e6=%d\n", THREAD->tid, (int) 1000000*pi);
}
*/
