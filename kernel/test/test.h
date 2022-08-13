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

#ifndef KERN_TEST_H_
#define KERN_TEST_H_

#include <stdbool.h>
#include <stdio.h>

extern bool test_quiet;

#define TPRINTF(format, ...) \
	{ \
		if (!test_quiet) { \
			printf(format, ##__VA_ARGS__); \
		} \
	}

typedef const char *(*test_entry_t)(void);

typedef struct {
	const char *name;
	const char *desc;
	test_entry_t entry;
	bool safe;
} test_t;

extern const char *test_atomic1(void);
extern const char *test_mips1(void);
extern const char *test_fault1(void);
extern const char *test_falloc1(void);
extern const char *test_falloc2(void);
extern const char *test_mapping1(void);
extern const char *test_purge1(void);
extern const char *test_slab1(void);
extern const char *test_slab2(void);
extern const char *test_semaphore1(void);
extern const char *test_semaphore2(void);
extern const char *test_print1(void);
extern const char *test_print2(void);
extern const char *test_print3(void);
extern const char *test_print4(void);
extern const char *test_print5(void);
extern const char *test_thread1(void);

extern test_t tests[];

extern const char *tests_hints_enum(const char *, const char **, void **);

#endif

/** @}
 */
