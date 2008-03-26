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

/** @addtogroup tester User space Tester
 * @brief	User space testing infrastructure.
 * @{
 */ 
/**
 * @file
 */

#include <unistd.h>
#include <stdio.h>
#include "tester.h"

int myservice = 0;
int phones[MAX_PHONES];
int connections[MAX_CONNECTIONS];
ipc_callid_t callids[MAX_CONNECTIONS];

test_t tests[] = {
#include "thread/thread1.def"
#include "print/print1.def"
#include "fault/fault1.def"
#include "fault/fault2.def"
#include "ipc/register.def"
#include "ipc/connect.def"
#include "ipc/send_async.def"
#include "ipc/send_sync.def"
#include "ipc/answer.def"
#include "ipc/hangup.def"
#include "devmap/devmap1.def"
#include "vfs/vfs1.def"
	{NULL, NULL, NULL}
};

static bool run_test(test_t *test)
{
	printf("%s\t\t%s\n", test->name, test->desc);
	
	/* Execute the test */
	char * ret = test->entry(false);
	
	if (ret == NULL) {
		printf("Test passed\n\n");
		return true;
	}

	printf("%s\n\n", ret);
	return false;
}

static void run_safe_tests(void)
{
	test_t *test;
	int i = 0, n = 0;

	printf("\n*** Running all safe tests\n\n");

	for (test = tests; test->name != NULL; test++) {
		if (test->safe) {
			if (run_test(test))
				i++;
			else
				n++;
		}
	}

	printf("\nSafe tests completed, %d tests run, %d passed.\n\n", i + n, i);
}

static void list_tests(void)
{
	test_t *test;
	char c = 'a';
	
	for (test = tests; test->name != NULL; test++, c++)
		printf("%c\t%s\t\t%s%s\n", c, test->name, test->desc, (test->safe ? "" : " (unsafe)"));
	
	printf("*\t\t\tRun all safe tests\n");
}

int main(void)
{
	while (1) {
		char c;
		test_t *test;
		
		list_tests();
		printf("> ");
		
		c = getchar();
		printf("%c\n", c);
		
		if ((c >= 'a') && (c <= 'z')) {
			for (test = tests; test->name != NULL; test++, c--)
				if (c == 'a')
					break;
			
			if (c > 'a')
				printf("Unknown test\n\n");
			else
				run_test(test);
		} else if (c == '*')
			run_safe_tests();
		else
			printf("Invalid test\n\n");
	}
}

/** @}
 */
