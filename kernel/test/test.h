/*
 * Copyright (c) 2006 Martin Decky
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
