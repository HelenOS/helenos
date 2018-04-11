/*
 * Copyright (c) 2014 Vojtech Horky
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



#include <errno.h>

#include "posix/stdio.h"

#include <pcut/pcut.h>

#define EPSILON 0.000001

PCUT_INIT;

PCUT_TEST_SUITE(scanf);


#ifndef UARCH_sparc64

/*
 * We need some floating point functions for scanf() imlementation
 * that are not yet available for SPARC-64.
 */

PCUT_TEST(int_decimal)
{
	int number;
	int rc = sscanf("4242", "%d", &number);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_INT_EQUALS(4242, number);
}

PCUT_TEST(int_negative_decimal)
{
	int number;
	int rc = sscanf("-53", "%d", &number);
	PCUT_ASSERT_INT_EQUALS(1, rc);
	PCUT_ASSERT_INT_EQUALS(-53, number);
}

/*
 * The following tests were copied from stdio/scanf.c where they were
 * commented-out. We ought to convert them to more independent tests
 * eventually.
 */

PCUT_TEST(int_misc)
{
	unsigned char uhh;
	signed char shh;
	unsigned short uh;
	short sh;
	unsigned udef;
	int sdef;
	unsigned long ul;
	long sl;
	unsigned long long ull;
	long long sll;
	void *p;

	int rc = sscanf(
	    "\n j tt % \t -121314 98765 aqw 0765 0x77 0xABCDEF88 -99 884",
	    " j tt %%%3hhd%1hhu%3hd %3hu%u aqw%n %lo%llx %p %li %lld",
	    &shh, &uhh, &sh, &uh, &udef, &sdef, &ul, &ull, &p, &sl, &sll);

	PCUT_ASSERT_INT_EQUALS(10, rc);

	PCUT_ASSERT_INT_EQUALS(-12, shh);
	PCUT_ASSERT_INT_EQUALS(1, uhh);
	PCUT_ASSERT_INT_EQUALS(314, sh);
	PCUT_ASSERT_INT_EQUALS(987, uh);
	PCUT_ASSERT_INT_EQUALS(65, udef);
	PCUT_ASSERT_INT_EQUALS(28, sdef);
	PCUT_ASSERT_INT_EQUALS(0765, ul);
	PCUT_ASSERT_INT_EQUALS(0x77, ull);
	PCUT_ASSERT_INT_EQUALS(0xABCDEF88, (long long) (uintptr_t) p);
	PCUT_ASSERT_INT_EQUALS(-99, sl);
	PCUT_ASSERT_INT_EQUALS(884, sll);
}

PCUT_TEST(double_misc)
{
	float f;
	double d;
	long double ld;

	int rc = sscanf(
	    "\n \t\t1.0 -0x555.AP10 1234.5678e12",
	    "%f %lf %Lf",
	    &f, &d, &ld);

	PCUT_ASSERT_INT_EQUALS(3, rc);

	PCUT_ASSERT_DOUBLE_EQUALS(1.0, f, EPSILON);
	PCUT_ASSERT_DOUBLE_EQUALS(-0x555.AP10, d, EPSILON);
	PCUT_ASSERT_DOUBLE_EQUALS(1234.5678e12, ld, EPSILON);
}

PCUT_TEST(str_misc)
{
	char str[20];
	char *pstr;

	int rc = sscanf(
	    "\n\n\thello world    \n",
	    "%5s %ms",
	    str, &pstr);

	PCUT_ASSERT_INT_EQUALS(2, rc);

	PCUT_ASSERT_STR_EQUALS("hello", str);
	PCUT_ASSERT_STR_EQUALS("world", pstr);

	free(pstr);
}

PCUT_TEST(str_matchers)
{
	char scanset[20];
	char *pscanset;

	int rc = sscanf(
	    "\n\n\th-e-l-l-o world-]    \n",
	    " %9[-eh-o] %m[^]-]",
	    scanset, &pscanset);

	PCUT_ASSERT_INT_EQUALS(2, rc);

	PCUT_ASSERT_STR_EQUALS("h-e-l-l-o", scanset);
	PCUT_ASSERT_STR_EQUALS("world", pscanset);

	free(pscanset);
}

PCUT_TEST(char_misc)
{
	char seq[20];
	char *pseq;

	int rc = sscanf(
	    "\n\n\thello world    \n",
	    " %5c %mc",
	    seq, &pseq);

	PCUT_ASSERT_INT_EQUALS(2, rc);

	/* Manually terminate the strings. */
	seq[5] = 0;
	pseq[1] = 0;

	PCUT_ASSERT_STR_EQUALS("hello", seq);
	PCUT_ASSERT_STR_EQUALS("w", pseq);

	free(pseq);
}

#endif

PCUT_EXPORT(scanf);
