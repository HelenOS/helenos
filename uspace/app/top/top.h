/*
 * Copyright (c) 2008 Stanislav Kozina
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

/** @addtogroup top
 * @{
 */

#ifndef TOP_TOP_H_
#define TOP_TOP_H_

#include <task.h>
#include <kernel/ps/cpuinfo.h>
#include <kernel/ps/taskinfo.h>

#define FRACTION_TO_FLOAT(float, a, b) { \
	(float).upper = (a); \
	(float).lower = (b); \
}

typedef struct {
	uint64_t upper;
	uint64_t lower;
} ps_float;

typedef struct {
	ps_float idle;
	ps_float busy;
} cpu_perc_t;

typedef struct {
	ps_float ucycles;
	ps_float kcycles;
	ps_float mem;
} task_perc_t;

typedef struct {
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;

	unsigned int uptime_d;
	unsigned int uptime_h;
	unsigned int uptime_m;
	unsigned int uptime_s;

	unsigned long load[3];

	size_t task_count;
	task_info_t *taskinfos;
	task_perc_t *task_perc;

	size_t thread_count;
	thread_info_t *thread_infos;

	unsigned int cpu_count;
	uspace_cpu_info_t *cpus;
	cpu_perc_t *cpu_perc;

	uspace_mem_info_t mem_info;
} data_t;

#endif

/**
 * @}
 */
