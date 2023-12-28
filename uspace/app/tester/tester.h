/*
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

/** @addtogroup tester
 * @{
 */
/** @file
 */

#ifndef TESTER_H_
#define TESTER_H_

#include <stdbool.h>
#include <stacktrace.h>
#include <stdio.h>

#define IPC_TEST_SERVICE  10240
#define IPC_TEST_METHOD   2000

extern bool test_quiet;
extern int test_argc;
extern char **test_argv;

/**
 * sizeof_array
 * @array array to determine the size of
 *
 * Returns the size of @array in array elements.
 */
#define sizeof_array(array) \
	(sizeof(array) / sizeof((array)[0]))

#define TPRINTF(format, ...) \
	do { \
		if (!test_quiet) { \
			fprintf(stdout, (format), ##__VA_ARGS__); \
		} \
	} while (0)

#define TSTACKTRACE() \
	do { \
		if (!test_quiet) { \
			stacktrace_print(); \
		} \
	} while (0)

typedef const char *(*test_entry_t)(void);

typedef struct {
	const char *name;
	const char *desc;
	test_entry_t entry;
	bool safe;
} test_t;

extern const char *test_thread1(void);
extern const char *test_setjmp1(void);
extern const char *test_print1(void);
extern const char *test_print2(void);
extern const char *test_print3(void);
extern const char *test_print4(void);
extern const char *test_print5(void);
extern const char *test_print6(void);
extern const char *test_console1(void);
extern const char *test_stdio1(void);
extern const char *test_stdio2(void);
extern const char *test_logger1(void);
extern const char *test_logger2(void);
extern const char *test_fault1(void);
extern const char *test_fault2(void);
extern const char *test_fault3(void);
extern const char *test_float1(void);
extern const char *test_float2(void);
extern const char *test_vfs1(void);
extern const char *test_ping_pong(void);
extern const char *test_readwrite(void);
extern const char *test_sharein(void);
extern const char *test_starve_ipc(void);
extern const char *test_loop1(void);
extern const char *test_malloc1(void);
extern const char *test_malloc2(void);
extern const char *test_malloc3(void);
extern const char *test_mapping1(void);
extern const char *test_pager1(void);
extern const char *test_serial1(void);
extern const char *test_devman1(void);
extern const char *test_devman2(void);
extern const char *test_chardev1(void);

extern test_t tests[];

#endif

/** @}
 */
