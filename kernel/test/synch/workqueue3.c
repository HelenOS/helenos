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


static int core_workq_enqueue(work_t *work_item, work_func_t func)
{
	return workq_global_enqueue(work_item, func);
}



static const char *do_test(bool exit_early)
{
	const char *err = NULL;
	TPRINTF("Stress testing system queue.\n");
	TPRINTF("First run:\n");
	err = run_workq_core(exit_early);

	if (!err) {
		TPRINTF("\nSecond run:\n");
		err = run_workq_core(exit_early);
	}

	TPRINTF("Done.\n");

	return err;
}

const char *test_workqueue3(void)
{
	return do_test(false);
}

const char *test_workqueue3quit(void)
{
	return do_test(true);
}
