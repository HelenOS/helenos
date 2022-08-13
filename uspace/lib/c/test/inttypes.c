/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Test greatest-width integer types
 */

#include <pcut/pcut.h>
#include <inttypes.h>

PCUT_INIT;

PCUT_TEST_SUITE(inttypes);

PCUT_TEST(decls)
{
	/* Make sure intmax_t is defined */
	intmax_t i;
	(void) i;

	/* Make sure imaxdiv_t is defined */
	imaxdiv_t d;
	(void) d;
}

/** llabs function of positive number */
PCUT_TEST(imaxabs_pos)
{
	intmax_t i;

	i = imaxabs(1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** llabs function of negative number */
PCUT_TEST(imaxabs_neg)
{
	intmax_t i;

	i = imaxabs(-1);
	PCUT_ASSERT_TRUE(i == 1);
}

/** Greatest-width integer division */
PCUT_TEST(imaxdiv)
{
	imaxdiv_t d;

	d = imaxdiv(41, 7);
	PCUT_ASSERT_INT_EQUALS(5, d.quot);
	PCUT_ASSERT_INT_EQUALS(6, d.rem);
}

PCUT_EXPORT(inttypes);
