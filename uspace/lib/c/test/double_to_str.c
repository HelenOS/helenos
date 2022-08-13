/*
 * SPDX-FileCopyrightText: 2019 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <ieee_double.h>
#include <double_to_str.h>

PCUT_INIT;

PCUT_TEST_SUITE(double_to_str);

PCUT_TEST(double_to_short_str_pos_zero)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(0.0);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("0", buf);
}

PCUT_TEST(double_to_short_str_neg_zero)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(-0.0);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("0", buf);
}

PCUT_TEST(double_to_short_str_pos_one)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1.0);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("1", buf);
}

PCUT_TEST(double_to_short_str_neg_one)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(-1.0);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("1", buf);
}

PCUT_TEST(double_to_short_str_small)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1.1);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(2, ret);
	PCUT_ASSERT_INT_EQUALS(-1, dec);
	PCUT_ASSERT_STR_EQUALS("11", buf);
}

PCUT_TEST(double_to_short_str_large)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1234.56789);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(9, ret);
	PCUT_ASSERT_INT_EQUALS(-5, dec);
	PCUT_ASSERT_STR_EQUALS("123456789", buf);
}

PCUT_TEST(double_to_short_str_mill)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1000000.0);
	int ret = double_to_short_str(d, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(6, dec);
	PCUT_ASSERT_STR_EQUALS("1", buf);
}

PCUT_TEST(double_to_fixed_str_zero)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(0.0);
	int ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("0", buf);
}

PCUT_TEST(double_to_fixed_str_pos_one)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1.0);
	int ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(-3, dec);
	PCUT_ASSERT_STR_EQUALS("1000", buf);
}

PCUT_TEST(double_to_fixed_str_neg_one)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(-1.0);
	int ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(-3, dec);
	PCUT_ASSERT_STR_EQUALS("1000", buf);
}

PCUT_TEST(double_to_fixed_str_small)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1.1);
	int ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(-3, dec);
	PCUT_ASSERT_STR_EQUALS("1100", buf);
}

PCUT_TEST(double_to_fixed_str_large)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1234.56789);
	int ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(7, ret);
	PCUT_ASSERT_INT_EQUALS(-3, dec);
	PCUT_ASSERT_STR_EQUALS("1234567", buf);
}

PCUT_TEST(double_to_fixed_str_nodecimals)
{
	size_t size = 255;
	char buf[size];
	int dec;
	ieee_double_t d = extract_ieee_double(1.999);
	int ret = double_to_fixed_str(d, -1, 0, buf, size, &dec);

	PCUT_ASSERT_INT_EQUALS(1, ret);
	PCUT_ASSERT_INT_EQUALS(0, dec);
	PCUT_ASSERT_STR_EQUALS("1", buf);
}

PCUT_EXPORT(double_to_str);
