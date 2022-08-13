/*
 * SPDX-FileCopyrightText: 2019 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
