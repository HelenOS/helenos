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

	d = extract_ieee_double(768.0);
	ret = double_to_fixed_str(d, -1, 3, buf, size, &dec);
	PCUT_ASSERT_INT_EQUALS(4, ret);
	PCUT_ASSERT_INT_EQUALS(-3, dec);
	PCUT_ASSERT_STR_EQUALS("768", buf);
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
