/*
 * Copyright (c) 2013 Vojtech Horky
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

#include <setjmp.h>
#include <stdlib.h>
#include <stddef.h>
#include "../tester.h"

static jmp_buf jmp_env;
static int counter;

static void do_the_long_jump(void) {
	TPRINTF("Will do a long jump back to test_it().\n");
	longjmp(jmp_env, 1);
}

static const char *test_it(void) {
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
