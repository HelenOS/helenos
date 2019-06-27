/*
 * Copyright (c) 2019 Matthieu Riolo
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

#include <pcut/pcut.h>
#include <imath.h>

static uint64_t MAX_NUM = 10000000000000000000U;
static unsigned MAX_EXP = 19;

PCUT_INIT;

PCUT_TEST_SUITE(imath);

PCUT_TEST(ipow10_u64_zero)
{
	errno_t ret;
	uint64_t result;
	ret = ipow10_u64(0, &result);

	PCUT_ASSERT_ERRNO_VAL(EOK, ret);
	PCUT_ASSERT_INT_EQUALS(1, result);
}

PCUT_TEST(ipow10_u64_one)
{
	errno_t ret;
	uint64_t result;
	ret = ipow10_u64(1, &result);

	PCUT_ASSERT_ERRNO_VAL(EOK, ret);
	PCUT_ASSERT_INT_EQUALS(10, result);
}

PCUT_TEST(ipow10_u64_max)
{
	errno_t ret;
	uint64_t result;
	ret = ipow10_u64(MAX_EXP, &result);

	PCUT_ASSERT_ERRNO_VAL(EOK, ret);
	PCUT_ASSERT_INT_EQUALS(MAX_NUM, result);
}

PCUT_TEST(ipow10_u64_too_large)
{
	errno_t ret;
	uint64_t result;
	ret = ipow10_u64(MAX_EXP + 1, &result);

	PCUT_ASSERT_ERRNO_VAL(ERANGE, ret);
}

PCUT_TEST(ilog10_u64_zero)
{
	unsigned ret = ilog10_u64(0);
	PCUT_ASSERT_INT_EQUALS(0, ret);
}

PCUT_TEST(ilog10_u64_one)
{
	unsigned ret = ilog10_u64(1);
	PCUT_ASSERT_INT_EQUALS(0, ret);
}

PCUT_TEST(ilog10_u64_max)
{
	unsigned ret = ilog10_u64(MAX_NUM);
	PCUT_ASSERT_INT_EQUALS(MAX_EXP, ret);
}

PCUT_EXPORT(imath);
