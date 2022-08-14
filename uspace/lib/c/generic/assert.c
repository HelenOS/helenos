/*
 * Copyright (c) 2011 Martin Decky
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

#include <assert.h>
#include <stdio.h>
#include <io/kio.h>
#include <stdlib.h>
#include <stacktrace.h>
#include <stdint.h>
#include <task.h>

__thread int failed_asserts = 0;

void __helenos_assert_quick_abort(const char *cond, const char *file, unsigned int line)
{
	/*
	 * Send the message safely to kio. Nested asserts should not occur.
	 */
	kio_printf("Assertion failed (%s) in task %" PRIu64 ", file \"%s\", line %u.\n",
	    cond, task_get_id(), file, line);

	stacktrace_kio_print();

	/* Sometimes we know in advance that regular printf() would likely fail. */
	abort();
}

void __helenos_assert_abort(const char *cond, const char *file, unsigned int line)
{
	/*
	 * Send the message safely to kio. Nested asserts should not occur.
	 */
	kio_printf("Assertion failed (%s) in task %" PRIu64 ", file \"%s\", line %u.\n",
	    cond, task_get_id(), file, line);

	stacktrace_kio_print();

	/*
	 * Check if this is a nested or parallel assert.
	 */
	failed_asserts++;
	if (failed_asserts > 0)
		abort();

	/*
	 * Attempt to print the message to standard output and display
	 * the stack trace. These operations can theoretically trigger nested
	 * assertions.
	 */
	kio_printf("Assertion failed (%s) in task %" PRIu64 ", file \"%s\", line %u.\n",
	    cond, task_get_id(), file, line);
	stacktrace_print();

	abort();
}

/** @}
 */
