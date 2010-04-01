/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup genericload
 * @{
 */

/**
 * @file
 * @brief	System load computation.
 */

#include <proc/thread.h>
#include <ps/load.h>
#include <arch.h>
#include <proc/scheduler.h>
#include <config.h>
#include <arch/types.h>
#include <time/clock.h>
#include <syscall/copy.h>

static size_t get_running_count(void);

unsigned long avenrun[3];

#define FSHIFT   11		/* nr of bits of precision */
#define FIXED_1  (1<<FSHIFT)	/* 1.0 as fixed-point */
#define LOAD_FREQ 5		/* 5 sec intervals */
#define EXP_1  1884		/* 1/exp(5sec/1min) as fixed-point */
#define EXP_5  2014		/* 1/exp(5sec/5min) */
#define EXP_15 2037		/* 1/exp(5sec/15min) */

void get_avenrun(unsigned long *loads, int shift)
{
	loads[0] = avenrun[0] << shift;
	loads[1] = avenrun[1] << shift;
	loads[2] = avenrun[2] << shift;
}

static inline unsigned long calc_load(unsigned long load, size_t exp, size_t active)
{
	load *= exp;
	load += active * (FIXED_1 - exp);
	return load >> FSHIFT;
}

static inline void calc_load_global(void)
{
	size_t active;

	active = get_running_count();
	active = active > 0 ? active * FIXED_1 : 0;
	avenrun[0] = calc_load(avenrun[0], EXP_1, active);
	avenrun[1] = calc_load(avenrun[1], EXP_5, active);
	avenrun[2] = calc_load(avenrun[2], EXP_15, active);
}

static size_t get_running_count(void)
{
	size_t i;
	size_t result = 0;
	ipl_t ipl;

	/* run queues should not change during reading */
	ipl = interrupts_disable();

	for (i = 0; i < config.cpu_active; ++i) {
		cpu_t *cpu = &cpus[i];
		int j;
		for (j = 0; j < RQ_COUNT; ++j) {
			result += cpu->rq[j].n;
		}
	} 

	interrupts_restore(ipl);
	return result;
}

/** Load thread main function.
 *  Thread computes system load every few seconds.
 *
 *  @param arg		Generic thread argument (unused).
 *
 */
void kload_thread(void *arg)
{
	/* Noone will thread_join us */
	thread_detach(THREAD);
	avenrun[0] = 0;
	avenrun[1] = 0;
	avenrun[2] = 0;

	while (true) {
		calc_load_global();
		thread_sleep(LOAD_FREQ);
	}
}

int sys_ps_get_load(unsigned long *user_load)
{
	unsigned long loads[3];
	get_avenrun(loads, 5);
	copy_to_uspace(user_load, loads, sizeof(loads));
	return 0;
}


/** @}
 */
