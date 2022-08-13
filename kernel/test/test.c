/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <test.h>
#include <stddef.h>
#include <str.h>

bool test_quiet;

test_t tests[] = {
#include <atomic/atomic1.def>
#include <debug/mips1.def>
#include <fault/fault1.def>
#include <mm/falloc1.def>
#include <mm/falloc2.def>
#include <mm/mapping1.def>
#include <mm/slab1.def>
#include <mm/slab2.def>
#include <synch/semaphore1.def>
#include <synch/semaphore2.def>
#include <print/print1.def>
#include <print/print2.def>
#include <print/print3.def>
#include <print/print4.def>
#include <print/print5.def>
#include <thread/thread1.def>
	{
		.name = NULL,
		.desc = NULL,
		.entry = NULL
	}
};

const char *tests_hints_enum(const char *input, const char **help,
    void **ctx)
{
	size_t len = str_length(input);
	test_t **test = (test_t **) ctx;

	if (*test == NULL)
		*test = tests;

	for (; (*test)->name; (*test)++) {
		const char *curname = (*test)->name;

		if (str_length(curname) < len)
			continue;

		if (str_lcmp(input, curname, len) == 0) {
			(*test)++;
			if (help)
				*help = (*test)->desc;
			return (curname + str_lsize(curname, len));
		}
	}

	return NULL;
}

/** @}
 */
