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

/** @addtogroup generic
 * @{
 */
/** @file taskinfo.h		Contains all thread and task defs common for
 * 				kernel and uspace.
 */

#ifndef KERN_PS_TASKINFO_H_
#define KERN_PS_TASKINFO_H_

#ifdef KERNEL
#include <typedefs.h>
#else
#include <thread.h>
#endif

#define TASK_NAME_BUFLEN	20

typedef struct {
	task_id_t taskid;
	char name[TASK_NAME_BUFLEN];
	uint64_t virt_mem;
	int thread_count;
	uint64_t ucycles;
	uint64_t kcycles;
} task_info_t;

/** Thread states. */
typedef enum {
	/** It is an error, if thread is found in this state. */
	Invalid,
	/** State of a thread that is currently executing on some CPU. */
	Running,
	/** Thread in this state is waiting for an event. */
	Sleeping,
	/** State of threads in a run queue. */
	Ready,
	/** Threads are in this state before they are first readied. */
	Entering,
	/** After a thread calls thread_exit(), it is put into Exiting state. */
	Exiting,
	/** Threads that were not detached but exited are Lingering. */
	Lingering
} state_t;

typedef struct {
	thread_id_t tid;
	state_t state;
	int priority;
	uint64_t cycles;
	uint64_t ucycles;
	uint64_t kcycles;
	unsigned int cpu;
} thread_info_t;


#endif

/** @}
 */
