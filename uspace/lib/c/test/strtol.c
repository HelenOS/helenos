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
 * @brief Test string to int conversion functions
 */

#include <pcut/pcut.h>
#include <stdlib.h>
#include <str.h>
#include <limits.h>

PCUT_INIT;

PCUT_TEST_SUITE(strtol);

/** atoi function */
PCUT_TEST(atoi)
{
	int i;

	i = atoi(" \t42");
	PCUT_ASSERT_TRUE(i == 42);
}

/** atol function */
PCUT_TEST(atol)
{
	long li;

	li = atol(" \t42");
	PCUT_ASSERT_TRUE(li == 42);
}

/** atoll function */
PCUT_TEST(atoll)
{
	long long lli;

	lli = atoll(" \t42");
	PCUT_ASSERT_TRUE(lli == 42);
}

/** strtol function */
PCUT_TEST(strtol)
{
	long li;
	char *ep;

	li = strtol(" \t42x", &ep, 10);
	PCUT_ASSERT_TRUE(li == 42);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with auto-detected base 10 */
PCUT_TEST(strtol_dec_auto)
{
	long li;
	char *ep;

	li = strtol(" \t42x", &ep, 0);
	PCUT_ASSERT_TRUE(li == 42);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with octal number */
PCUT_TEST(strtol_oct)
{
	long li;
	char *ep;

	li = strtol(" \t052x", &ep, 8);
	PCUT_ASSERT_TRUE(li == 052);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with octal number with prefix */
#include <stdio.h>
PCUT_TEST(strtol_oct_prefix)
{
	long li;
	char *ep;

	li = strtol(" \t052x", &ep, 0);
	printf("li=%ld (0%lo)\n", li, li);
	PCUT_ASSERT_TRUE(li == 052);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with hex number */
PCUT_TEST(strtol_hex)
{
	long li;
	char *ep;

	li = strtol(" \t2ax", &ep, 16);
	PCUT_ASSERT_TRUE(li == 0x2a);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with hex number with hex prefix */
PCUT_TEST(strtol_hex_prefixed)
{
	long li;
	char *ep;

	li = strtol(" \t0x2ax", &ep, 0);
	PCUT_ASSERT_TRUE(li == 0x2a);
	PCUT_ASSERT_TRUE(*ep == 'x');
}

/** strtol function with base 16 and number with 0x prefix */
PCUT_TEST(strtol_base16_prefix)
{
	long li;
	char *ep;

	li = strtol(" \t0x1y", &ep, 16);
	printf("li=%ld\n", li);
	PCUT_ASSERT_TRUE(li == 1);
	PCUT_ASSERT_TRUE(*ep == 'y');
}

/** strtol function with base 36 number */
PCUT_TEST(strtol_base36)
{
	long li;
	char *ep;

	li = strtol(" \tz1.", &ep, 36);
	PCUT_ASSERT_TRUE(li == 35 * 36 + 1);
	PCUT_ASSERT_TRUE(*ep == '.');
}

PCUT_TEST(str_uint_hex)
{
	int base;
	bool strict;
	const char *endp;
	uint32_t result;
	errno_t rc;
	const char *input;

	/*
	 * str_* conversion function do not admit a prefix when base is
	 * specified.
	 */

	rc = str_uint32_t(input = "0x10", &endp, base = 0, strict = false, &result);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(0x10, result);
	PCUT_ASSERT_PTR_EQUALS(input + 4, endp);

	rc = str_uint32_t(input = "0x10", &endp, base = 16, strict = false, &result);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(0, result);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);

	rc = str_uint32_t(input = "  \t0x10", &endp, base = 0, strict = false, &result);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(0x10, result);
	PCUT_ASSERT_PTR_EQUALS(input + 7, endp);

	rc = str_uint32_t(input = "  \t0x10", &endp, base = 16, strict = false, &result);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(0, result);
	PCUT_ASSERT_PTR_EQUALS(input + 4, endp);
}

PCUT_TEST(str_uint_overflow)
{
	int base;
	bool strict;
	uint64_t result;
	errno_t rc;
	const char *input;

	/*
	 * Naive overflow check will not detect this overflow,
	 * since the intermediate result is equal to the previous step.
	 */

	rc = str_uint64_t(input = "0xffffffffffffffffffffffffffffffff", NULL, base = 0, strict = false, &result);
#if 0
	/* Buggy result. */
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_UINT_EQUALS(0xffffffffffffffff, result);
#else
	/* Correct result. */
	PCUT_ASSERT_INT_EQUALS(EOVERFLOW, rc);
#endif

	/* 3^40 */

	rc = str_uint64_t(input = "1" "0000000000" "0000000000" "0000000000" "0000000000", NULL, base = 3, strict = false, &result);
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	PCUT_ASSERT_UINT_EQUALS(0xa8b8b452291fe821, result);

	/*
	 * Naive overflow check will not detect this overflow,
	 * since the intermediate result is greater than the previous step,
	 * despite overflowing 64 bits.
	 *
	 * The input is 3^41 which is greater than 2^64,
	 * but (3^41 mod 2^64) is still greater than 3^40, so overflow is not
	 * detected by a naive magnitude check.
	 */

	rc = str_uint64_t(input = "10" "0000000000" "0000000000" "0000000000" "0000000000", NULL, base = 3, strict = false, &result);
#if 0
	/* Buggy result. */
	PCUT_ASSERT_INT_EQUALS(EOK, rc);
	/* Correct value is    0x1fa2a1cf67b5fb863 */
	PCUT_ASSERT_UINT_EQUALS(0xfa2a1cf67b5fb863, result);
#else
	/* Correct result. */
	PCUT_ASSERT_INT_EQUALS(EOVERFLOW, rc);
#endif
}

PCUT_TEST(strtoul_negative_wraparound)
{
	long output;
	char *endp;
	char *endp_unchanged = (char *) "endp_unchanged unique pointer";
	int errno_unchanged = -1;
	const char *input;
	int base;

	/*
	 * N2176 7.22.1.4 The strtol, strtoll, strtoul, and strtoull functions
	 *
	 * "If the subject sequence begins with a minus sign, the value
	 * resulting from the conversion is negated (in the return type)."
	 */

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtoul(input = "-10", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 3, endp);
	PCUT_ASSERT_UINT_EQUALS(-(10ul), output);
}

PCUT_TEST(strtol_fringe)
{
	long output;
	char *endp;
	char *endp_unchanged = (char *) "endp_unchanged unique pointer";
	int errno_unchanged = -1;
	const char *input;
	int base;

	/* Variants of plain zero with various bases. */
	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = -10);
	PCUT_ASSERT_INT_EQUALS(EINVAL, errno);
	PCUT_ASSERT_PTR_EQUALS(endp_unchanged, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 1);
	PCUT_ASSERT_INT_EQUALS(EINVAL, errno);
	PCUT_ASSERT_PTR_EQUALS(endp_unchanged, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	for (base = 2; base <= 36; base++) {
		endp = endp_unchanged;
		errno = errno_unchanged;
		output = strtol(input = "0", &endp, base);
		PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
		PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
		PCUT_ASSERT_INT_EQUALS(0, output);

		endp = endp_unchanged;
		errno = errno_unchanged;
		output = strtol(input = "1", &endp, base);
		PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
		PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
		PCUT_ASSERT_INT_EQUALS(1, output);

		endp = endp_unchanged;
		errno = errno_unchanged;
		output = strtol(input = "10", &endp, base);
		PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
		PCUT_ASSERT_PTR_EQUALS(input + 2, endp);
		PCUT_ASSERT_INT_EQUALS(base, output);

		endp = endp_unchanged;
		errno = errno_unchanged;
		output = strtol(input = "100", &endp, base);
		PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
		PCUT_ASSERT_PTR_EQUALS(input + 3, endp);
		PCUT_ASSERT_INT_EQUALS(base * base, output);

		endp = endp_unchanged;
		errno = errno_unchanged;
		output = strtol(input = "1000", &endp, base);
		PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
		PCUT_ASSERT_PTR_EQUALS(input + 4, endp);
		PCUT_ASSERT_INT_EQUALS(base * base * base, output);
	}

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 8);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 16);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 36);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 1, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 37);
	PCUT_ASSERT_INT_EQUALS(EINVAL, errno);
	PCUT_ASSERT_PTR_EQUALS(endp_unchanged, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "0", &endp, base = 37);
	PCUT_ASSERT_INT_EQUALS(EINVAL, errno);
	PCUT_ASSERT_PTR_EQUALS(endp_unchanged, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	/* No valid number */
	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    ", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    ", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    x", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    x0", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0x", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 5, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0xg", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 5, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0x1", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 7, endp);
	PCUT_ASSERT_INT_EQUALS(1, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0x", &endp, base = 16);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 5, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0xg", &endp, base = 16);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 5, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    g", &endp, base = 16);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    0x1", &endp, base = 16);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input + 7, endp);
	PCUT_ASSERT_INT_EQUALS(1, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    +", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    -", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    +", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "    -", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "+", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "-", &endp, base = 0);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "+", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);

	endp = endp_unchanged;
	errno = errno_unchanged;
	output = strtol(input = "-", &endp, base = 10);
	PCUT_ASSERT_INT_EQUALS(errno_unchanged, errno);
	PCUT_ASSERT_PTR_EQUALS(input, endp);
	PCUT_ASSERT_INT_EQUALS(0, output);
}

PCUT_EXPORT(strtol);
