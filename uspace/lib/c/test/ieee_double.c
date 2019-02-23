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

union {
	uint64_t integer;
	double floating;
} ieee_double_bits;

const int EXP_BIAS = 1023;
const int EXP_BIAS_UNDERFLOWED = 1075;
const int SIGN_SHIFT = 52;
const uint64_t HIDDEN_BIT = 1ULL << SIGN_SHIFT;

PCUT_INIT;

PCUT_TEST_SUITE(ieee_double);

PCUT_TEST(extract_ieee_sizeof_double)
{
	PCUT_ASSERT_INT_EQUALS(8, sizeof(double));
}

PCUT_TEST(extract_ieee_double_pos_infinity)
{
	ieee_double_bits.integer = 0x7FF0000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_TRUE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_TRUE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_neg_infinity)
{
	ieee_double_bits.integer = 0xFFF0000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_TRUE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_TRUE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_nan)
{
	ieee_double_bits.integer = 0xFFFFFFFFFFFFFFFFULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_TRUE(d.is_special);
	PCUT_ASSERT_TRUE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_pos_zero)
{
	ieee_double_bits.integer = 0x0000000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.significand);
}

PCUT_TEST(extract_ieee_double_neg_zero)
{
	ieee_double_bits.integer = 0x8000000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);

	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0, d.pos_val.significand);
}

PCUT_TEST(extract_ieee_double_normal_pos_one)
{
	ieee_double_bits.integer = 0x3FF0000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_TRUE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(EXP_BIAS - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_normal_neg_one)
{
	ieee_double_bits.integer = 0xBFF0000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_TRUE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(EXP_BIAS - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_denormal_pos_smallest)
{
	ieee_double_bits.integer = 0x0000000000000001ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(1, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_denormal_neg_smallest)
{
	ieee_double_bits.integer = 0x8000000000000001ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(1, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_denormal_pos_largest)
{
	ieee_double_bits.integer = 0x000FFFFFFFFFFFFFULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0xFFFFFFFFFFFFFULL, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_denormal_neg_largest)
{
	ieee_double_bits.integer = 0x800FFFFFFFFFFFFFULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_TRUE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);
	PCUT_ASSERT_INT_EQUALS(0xFFFFFFFFFFFFFULL, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_normal_pos_smallest)
{
	ieee_double_bits.integer = 0x0010000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_normal_neg_smallest)
{
	ieee_double_bits.integer = 0x8010000000000000ULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(1 - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_normal_pos_largest)
{
	ieee_double_bits.integer = 0x7FEFFFFFFFFFFFFFULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_FALSE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0xFFFFFFFFFFFFFULL + HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(0x7FEULL - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_TEST(extract_ieee_double_normal_neg_largest)
{
	ieee_double_bits.integer = 0xFFEFFFFFFFFFFFFFULL;
	ieee_double_t d = extract_ieee_double(ieee_double_bits.floating);

	PCUT_ASSERT_FALSE(d.is_special);
	PCUT_ASSERT_FALSE(d.is_nan);
	PCUT_ASSERT_FALSE(d.is_infinity);
	PCUT_ASSERT_TRUE(d.is_negative);
	PCUT_ASSERT_FALSE(d.is_denormal);
	PCUT_ASSERT_FALSE(d.is_accuracy_step);

	PCUT_ASSERT_INT_EQUALS(0xFFFFFFFFFFFFFULL + HIDDEN_BIT, d.pos_val.significand);
	PCUT_ASSERT_INT_EQUALS(0x7FEULL - EXP_BIAS_UNDERFLOWED, d.pos_val.exponent);
}

PCUT_EXPORT(ieee_double);
