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


/*-------------------------------------------------------------------*/

static work_t basic_work;
static int basic_done = 0;

static void basic_test_work(work_t *work_item)
{
	basic_done = 1;
	TPRINTF("basic_test_work()");
}


static void basic_test(void)
{
	TPRINTF("Issue a single work item.\n");
	basic_done = 0;
	workq_global_enqueue(&basic_work, basic_test_work);

	while (!basic_done) {
		TPRINTF(".");
		thread_sleep(1);
	}

	TPRINTF("\nBasic test done\n");
}

/*-------------------------------------------------------------------*/


struct work_queue *workq = NULL;

static int core_workq_enqueue(work_t *work_item, work_func_t func)
{
	return workq_enqueue(workq, work_item, func);
}
/*-------------------------------------------------------------------*/


static const char *test_custom_workq_impl(bool stop, const char *qname)
{
	workq = workq_create(qname);

	if (!workq) {
		return "Failed to create a work queue.\n";
	}

	const char *ret = run_workq_core(stop);

	TPRINTF("Stopping work queue...\n");
	workq_stop(workq);

	TPRINTF("Destroying work queue...\n");
	workq_destroy(workq);
	return ret;
}

static const char *test_custom_workq(void)
{
	TPRINTF("Stress testing a custom queue.\n");
	return test_custom_workq_impl(false, "test-workq");
}


static const char *test_custom_workq_stop(void)
{
	TPRINTF("Stress testing a custom queue. Stops prematurely. "
	    "Errors are expected.\n");
	test_custom_workq_impl(true, "test-workq-stop");
	/* Errors are expected. */
	return NULL;
}


const char *test_workqueue_all(void)
{
	const char *err = NULL;
	const char *res;

	basic_test();

	res = test_custom_workq();
	if (res) {
		TPRINTF("%s", res);
		err = res;
	}

	res = test_custom_workq_stop();
	if (res) {
		TPRINTF("%s", res);
		err = res;
	}

	res = test_workqueue3();
	if (res) {
		TPRINTF("%s", res);
		err = res;
	}

	return err;
}
