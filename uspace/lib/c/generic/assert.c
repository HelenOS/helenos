/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	kio_printf("Assertion failed (%s) in task %ld, file \"%s\", line %u.\n",
	    cond, (long) task_get_id(), file, line);

	stacktrace_kio_print();

	/* Sometimes we know in advance that regular printf() would likely fail. */
	abort();
}

void __helenos_assert_abort(const char *cond, const char *file, unsigned int line)
{
	/*
	 * Send the message safely to kio. Nested asserts should not occur.
	 */
	kio_printf("Assertion failed (%s) in task %ld, file \"%s\", line %u.\n",
	    cond, (long) task_get_id(), file, line);

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
	kio_printf("Assertion failed (%s) in task %ld, file \"%s\", line %u.\n",
	    cond, (long) task_get_id(), file, line);
	stacktrace_print();

	abort();
}

/** @}
 */
