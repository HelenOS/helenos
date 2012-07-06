/*
 * Copyright (c) 2012 Adam Hraska
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

#include <test.h>
#include <arch.h>
#include <print.h>
#include <memstr.h>
#include <synch/workqueue.h>


#define WAVES 10
#define COUNT_POW 12
#define COUNT ((1 << COUNT_POW) - 1)
#define WAVE_SLEEP_MS 100
#define MAIN_POLL_SLEEP_MS 100
#define MAIN_MAX_SLEEP_SEC 40

/*
 * Include the test implementation.
 */
#include "workq-test-core.h"


struct work_queue *workq = NULL;

static int core_workq_enqueue(work_t *work_item, work_func_t func)
{
	return workq_enqueue(workq, work_item, func);
}



const char *test_workqueue2(void)
{
	workq = workq_create("test-workq");
	
	if (!workq) {
		return "Failed to create a work queue.";
	}
	
	const char *ret = run_workq_core(false);
	
	TPRINTF("Stopping work queue...\n");
	workq_stop(workq);
	
	TPRINTF("Destroying work queue...\n");
	workq_destroy(workq);

	return ret;
}


const char *test_workqueue2stop(void)
{
	workq = workq_create("test-workq");
	
	if (!workq) {
		return "Failed to create a work queue.";
	}
	
	const char *ret = run_workq_core(true);
	
	TPRINTF("Stopping work queue...\n");
	workq_stop(workq);
	
	TPRINTF("Destroying work queue...\n");
	workq_destroy(workq);

	return ret;
}

