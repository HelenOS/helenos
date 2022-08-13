/*
 * SPDX-FileCopyrightText: 2007 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
