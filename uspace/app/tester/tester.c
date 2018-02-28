/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2007 Martin Decky
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

/** @addtogroup tester User space tester
 * @brief User space testing infrastructure.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include <io/log.h>
#include "tester.h"

bool test_quiet;
int test_argc;
char **test_argv;

test_t tests[] = {
#include "thread/thread1.def"
#include "thread/setjmp1.def"
#include "print/print1.def"
#include "print/print2.def"
#include "print/print3.def"
#include "print/print4.def"
#include "print/print5.def"
#include "print/print6.def"
#include "console/console1.def"
#include "stdio/stdio1.def"
#include "stdio/stdio2.def"
#include "stdio/logger1.def"
#include "stdio/logger2.def"
#include "fault/fault1.def"
#include "fault/fault2.def"
#include "fault/fault3.def"
#include "float/float1.def"
#include "float/float2.def"
#include "float/softfloat1.def"
#include "vfs/vfs1.def"
#include "ipc/ping_pong.def"
#include "ipc/starve.def"
#include "loop/loop1.def"
#include "mm/malloc1.def"
#include "mm/malloc2.def"
#include "mm/malloc3.def"
#include "mm/mapping1.def"
#include "mm/pager1.def"
#include "hw/serial/serial1.def"
#include "chardev/chardev1.def"
	{NULL, NULL, NULL, false}
};

static bool run_test(test_t *test)
{
	/* Execute the test */
	const char *ret = test->entry();

	if (ret == NULL) {
		printf("\nTest passed\n");
		return true;
	}

	printf("\n%s\n", ret);
	return false;
}

static void run_safe_tests(void)
{
	test_t *test;
	unsigned int i = 0;
	unsigned int n = 0;

	printf("\n*** Running all safe tests ***\n\n");

	for (test = tests; test->name != NULL; test++) {
		if (test->safe) {
			printf("%s (%s)\n", test->name, test->desc);
			if (run_test(test))
				i++;
			else
				n++;
		}
	}

	printf("\nCompleted, %u tests run, %u passed.\n", i + n, i);
}

static void list_tests(void)
{
	size_t len = 0;
	test_t *test;
	for (test = tests; test->name != NULL; test++) {
		if (str_length(test->name) > len)
			len = str_length(test->name);
	}

	unsigned int _len = (unsigned int) len;
	if ((_len != len) || (((int) _len) < 0)) {
		printf("Command length overflow\n");
		return;
	}

	for (test = tests; test->name != NULL; test++)
		printf("%-*s %s%s\n", _len, test->name, test->desc,
		    (test->safe ? "" : " (unsafe)"));

	printf("%-*s Run all safe tests\n", _len, "*");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage:\n\n");
		printf("%s <test> [args ...]\n\n", argv[0]);
		list_tests();
		return 0;
	}

	log_init("tester");

	test_quiet = false;
	test_argc = argc - 2;
	test_argv = argv + 2;

	if (str_cmp(argv[1], "*") == 0) {
		run_safe_tests();
		return 0;
	}

	test_t *test;
	for (test = tests; test->name != NULL; test++) {
		if (str_cmp(argv[1], test->name) == 0) {
			return (run_test(test) ? 0 : -1);
		}
	}

	printf("Unknown test \"%s\"\n", argv[1]);
	return -2;
}

/** @}
 */
