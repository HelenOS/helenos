/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <setjmp.h>
#include <stdlib.h>
#include <stddef.h>
#include "../tester.h"

static jmp_buf jmp_env;
static int counter;

static void do_the_long_jump(void)
{
	TPRINTF("Will do a long jump back to test_it().\n");
	longjmp(jmp_env, 1);
}

static const char *test_it(void)
{
	int second_round = setjmp(jmp_env);
	counter++;
	TPRINTF("Just after setjmp(), counter is %d.\n", counter);
	if (second_round) {
		if (counter != 2) {
			return "setjmp() have not returned twice";
		} else {
			return NULL;
		}
	}

	if (counter != 1) {
		return "Shall not reach here more than once";
	}

	do_the_long_jump();

	return "Survived a long jump";
}

const char *test_setjmp1(void)
{
	counter = 0;

	const char *err_msg = test_it();

	return err_msg;
}
