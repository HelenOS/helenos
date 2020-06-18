/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Test General utilities (stdlib.h)
 */

#include <pcut/pcut.h>
#include <stdlib.h>

PCUT_INIT;

PCUT_TEST_SUITE(stdlib);

PCUT_TEST(decls)
{
	/* Make sure size_t is defined */
	size_t sz = 0;
	(void) sz;

	/* Make sure char32_t is defined */
	char32_t wc = L'\0';
	(void) wc;

	/* Make sure EXIT_FAILURE and EXIT_SUCCESS are defined */
	if (0)
		exit(EXIT_FAILURE);
	if (0)
		exit(EXIT_SUCCESS);

	/* Make sure NULL is defined */
	void *ptr = NULL;
	(void) ptr;
}

/** strtold function */
#include <stdio.h>
PCUT_TEST(strtold)
{
	long double ld;
	const char *str = " \t4.2e1@";
	char *endptr;

	ld = strtold(str, &endptr);
	printf("ld=%.10lf\n", (double)ld);
	PCUT_ASSERT_TRUE(ld == 42.0);
}

/** rand function */
PCUT_TEST(rand)
{
	int i;
	int r;

	for (i = 0; i < 100; i++) {
		r = rand();
		PCUT_ASSERT_TRUE(r >= 0);
		PCUT_ASSERT_TRUE(r <= RAND_MAX);
	}

	PCUT_ASSERT_TRUE(RAND_MAX >= 32767);
}

/** srand function */
PCUT_TEST(srand)
{
	int r1;
	int r2;

	srand(1);
	r1 = rand();
	srand(1);
	r2 = rand();

	PCUT_ASSERT_INT_EQUALS(r2, r1);

	srand(42);
	r1 = rand();
	srand(42);
	r2 = rand();

	PCUT_ASSERT_INT_EQUALS(r2, r1);
}

/** Just make sure we have memory allocation function prototypes */
PCUT_TEST(malloc)
{
	void *p;

#if 0
	// TODO
	p = aligned_alloc(4, 8);
	PCUT_ASSERT_NOT_NULL(p);
	free(p);
#endif
	p = calloc(4, 4);
	PCUT_ASSERT_NOT_NULL(p);
	free(p);

	p = malloc(4);
	PCUT_ASSERT_NOT_NULL(p);
	p = realloc(p, 2);
	PCUT_ASSERT_NOT_NULL(p);
	free(p);
}

/** Just check abort() is defined */
PCUT_TEST(abort)
{
	if (0)
		abort();
}

static void dummy_exit_handler(void)
{
}

/** atexit function */
PCUT_TEST(atexit)
{
	int rc;

	rc = atexit(dummy_exit_handler);
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** exit function -- just make sure it is declared */
PCUT_TEST(exit)
{
	if (0)
		exit(0);
}

/** at_quick_exit function */
PCUT_TEST(at_quick_exit)
{
	int rc;

	rc = at_quick_exit(dummy_exit_handler);
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** quick_exit function -- just make sure it is declared */
PCUT_TEST(quick_exit)
{
	if (0)
		quick_exit(0);
}

/** getenv function */
PCUT_TEST(getenv)
{
	char *s;

	s = getenv("FOO");
	PCUT_ASSERT_NULL(s);
}

/** Test availability of command processor */
PCUT_TEST(system_null)
{
	int rc;

	rc = system(NULL);
	PCUT_ASSERT_INT_EQUALS(0, rc);
}

/** Test running a command */
PCUT_TEST(system_cmd)
{
	int rc;

	/* This should fail as system is just a stub */
	rc = system("/app/bdsh");
	PCUT_ASSERT_INT_EQUALS(1, rc);
}

/** Comparison function for bsearch test */
static int test_compar(const void *a, const void *b)
{
	const int *ia, *ib;

	ia = (const int *)a;
	ib = (const int *)b;

	return *ia - *ib;
}

PCUT_TEST(bsearch)
{
	int numbers[] = { 1, 2, 6, 7, 7, 10, 100, 120 };
	int k;
	void *r;

	k = 0;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NULL(r);

	k = 1;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NOT_NULL(r);
	PCUT_ASSERT_INT_EQUALS(1, *(int *)r);

	k = 3;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NULL(r);

	k = 6;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NOT_NULL(r);
	PCUT_ASSERT_INT_EQUALS(6, *(int *)r);

	k = 7;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NOT_NULL(r);
	PCUT_ASSERT_INT_EQUALS(7, *(int *)r);

	k = 200;
	r = bsearch(&k, numbers, sizeof(numbers) / sizeof(int), sizeof(int),
	    test_compar);
	PCUT_ASSERT_NULL(r);
}

/** abs function of positive number */
PCUT_TEST(abs_pos)
{
	int i;

	i = abs(1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** abs function of negative number */
PCUT_TEST(abs_neg)
{
	int i;

	i = abs(-1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** labs function of positive number */
PCUT_TEST(labs_pos)
{
	long li;

	li = labs(1);
	PCUT_ASSERT_TRUE(li == 1);
}

/** labs function of negative number */
PCUT_TEST(labs_neg)
{
	long li;

	li = labs(-1);
	PCUT_ASSERT_TRUE(li == 1);
}

/** llabs function of positive number */
PCUT_TEST(llabs_pos)
{
	long long lli;

	lli = llabs(1);
	PCUT_ASSERT_TRUE(lli == 1);
}

/** llabs function of negative number */
PCUT_TEST(llabs_neg)
{
	long long lli;

	lli = llabs(-1);
	PCUT_ASSERT_TRUE(lli == 1);
}

/** Integer division */
PCUT_TEST(div_func)
{
	div_t d;

	d = div(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

/** Long integer division */
PCUT_TEST(ldiv_func)
{
	ldiv_t d;

	d = ldiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

/** Long long integer division */
PCUT_TEST(lldiv_func)
{
	lldiv_t d;

	d = lldiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

PCUT_EXPORT(stdlib);
