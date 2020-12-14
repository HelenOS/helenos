/*
 * Copyright (c) 2012 Adam Hraska
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

#include <double_to_str.h>

#include "private/power_of_ten.h"
#include <ieee_double.h>

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

/*
 * Floating point numbers are converted from their binary representation
 * into a decimal string using the algorithm described in:
 *   Printing floating-point numbers quickly and accurately with integers
 *   Loitsch, 2010
 */

/** The computation assumes a significand of 64 bits. */
static const int significand_width = 64;

/* Scale exponents to interval [alpha, gamma] to simplify conversion. */
static const int alpha = -59;
static const int gamma = -32;

/** Returns true if the most-significant bit of num.significand is set. */
static bool is_normalized(fp_num_t num)
{
	assert(8 * sizeof(num.significand) == significand_width);

	/* Normalized == most significant bit of the significand is set. */
	return (num.significand & (1ULL << (significand_width - 1))) != 0;
}

/** Returns a normalized num with the MSbit set. */
static fp_num_t normalize(fp_num_t num)
{
	const uint64_t top10bits = 0xffc0000000000000ULL;

	/* num usually comes from ieee_double with top 10 bits zero. */
	while (0 == (num.significand & top10bits)) {
		num.significand <<= 10;
		num.exponent -= 10;
	}

	while (!is_normalized(num)) {
		num.significand <<= 1;
		--num.exponent;
	}

	return num;
}

/** Returns x * y with an error of less than 0.5 ulp. */
static fp_num_t multiply(fp_num_t x, fp_num_t y)
{
	assert(/* is_normalized(x) && */ is_normalized(y));

	const uint32_t low_bits = -1;

	uint64_t a, b, c, d;
	a = x.significand >> 32;
	b = x.significand & low_bits;
	c = y.significand >> 32;
	d = y.significand & low_bits;

	uint64_t bd, ad, bc, ac;
	bd = b * d;
	ad = a * d;

	bc = b * c;
	ac = a * c;

	/*
	 * Denote 32 bit parts of x a y as: x == a b, y == c d. Then:
	 *        a  b
	 *  *     c  d
	 *  ----------
	 *       ad bd .. multiplication of 32bit parts results in 64bit parts
	 *  + ac bc
	 *  ----------
	 *       [b|d] .. Depicts 64 bit intermediate results and how
	 *     [a|d]      the 32 bit parts of these results overlap and
	 *     [b|c]      contribute to the final result.
	 *  +[a|c]
	 *  ----------
	 *   [ret]
	 *  [tmp]
	 */
	uint64_t tmp = (bd >> 32) + (ad & low_bits) + (bc & low_bits);

	/* Round upwards. */
	tmp += 1U << 31;

	fp_num_t ret;
	ret.significand = ac + (bc >> 32) + (ad >> 32) + (tmp >> 32);
	ret.exponent = x.exponent + y.exponent + significand_width;

	return ret;
}

/** Returns a - b. Both must have the same exponent. */
static fp_num_t subtract(fp_num_t a, fp_num_t b)
{
	assert(a.exponent == b.exponent);
	assert(a.significand >= b.significand);

	fp_num_t result;

	result.significand = a.significand - b.significand;
	result.exponent = a.exponent;

	return result;
}

/** Returns the interval [low, high] of numbers that convert to binary val. */
static void get_normalized_bounds(ieee_double_t val, fp_num_t *high,
    fp_num_t *low, fp_num_t *val_dist)
{
	/*
	 * Only works if val comes directly from extract_ieee_double without
	 * being manipulated in any way (eg it must not be normalized).
	 */
	assert(!is_normalized(val.pos_val));

	high->significand = (val.pos_val.significand << 1) + 1;
	high->exponent = val.pos_val.exponent - 1;

	/* val_dist = high - val */
	val_dist->significand = 1;
	val_dist->exponent = val.pos_val.exponent - 1;

	/* Distance from both lower and upper bound is the same. */
	if (!val.is_accuracy_step) {
		low->significand = (val.pos_val.significand << 1) - 1;
		low->exponent = val.pos_val.exponent - 1;
	} else {
		low->significand = (val.pos_val.significand << 2) - 1;
		low->exponent = val.pos_val.exponent - 2;
	}

	*high = normalize(*high);

	/*
	 * Lower bound may not be normalized if subtracting 1 unit
	 * reset the most-significant bit to 0.
	 */
	low->significand = low->significand << (low->exponent - high->exponent);
	low->exponent = high->exponent;

	val_dist->significand =
	    val_dist->significand << (val_dist->exponent - high->exponent);
	val_dist->exponent = high->exponent;
}

/** Determines the interval of numbers that have the binary representation
 *  of val.
 *
 * Numbers in the range [scaled_upper_bound - bounds_delta, scaled_upper_bound]
 * have the same double binary representation as val.
 *
 * Bounds are scaled by 10^scale so that alpha <= exponent <= gamma.
 * Moreover, scaled_upper_bound is normalized.
 *
 * val_dist is the scaled distance from val to the upper bound, ie
 * val_dist == (upper_bound - val) * 10^scale
 */
static void calc_scaled_bounds(ieee_double_t val, fp_num_t *scaled_upper_bound,
    fp_num_t *bounds_delta, fp_num_t *val_dist, int *scale)
{
	fp_num_t upper_bound, lower_bound;

	get_normalized_bounds(val, &upper_bound, &lower_bound, val_dist);

	assert(upper_bound.exponent == lower_bound.exponent);
	assert(is_normalized(upper_bound));
	assert(normalize(val.pos_val).exponent == upper_bound.exponent);

	/*
	 * Find such a cached normalized power of 10 that if multiplied
	 * by upper_bound the binary exponent of upper_bound almost vanishes,
	 * ie:
	 *   upper_scaled := upper_bound * 10^scale
	 *   alpha <= upper_scaled.exponent <= gamma
	 *   alpha <= upper_bound.exponent + pow_10.exponent + 64 <= gamma
	 */
	fp_num_t scaling_power_of_10;
	int lower_bin_exp = alpha - upper_bound.exponent - significand_width;

	get_power_of_ten(lower_bin_exp, &scaling_power_of_10, scale);

	int scale_exp = scaling_power_of_10.exponent;
	assert(alpha <= upper_bound.exponent + scale_exp + significand_width);
	assert(upper_bound.exponent + scale_exp + significand_width <= gamma);

	fp_num_t upper_scaled = multiply(upper_bound, scaling_power_of_10);
	fp_num_t lower_scaled = multiply(lower_bound, scaling_power_of_10);
	*val_dist = multiply(*val_dist, scaling_power_of_10);

	assert(alpha <= upper_scaled.exponent && upper_scaled.exponent <= gamma);

	/*
	 * Any value between lower and upper bound would be represented
	 * in binary as the double val originated from. The bounds were
	 * however scaled by an imprecise power of 10 (error less than
	 * 1 ulp) so the scaled bounds have an error of less than 1 ulp.
	 * Conservatively round the lower bound up and the upper bound
	 * down by 1 ulp just to be on the safe side. It avoids pronouncing
	 * produced decimal digits as correct if such a decimal number
	 * is close to the bounds to within 1 ulp.
	 */
	upper_scaled.significand -= 1;
	lower_scaled.significand += 1;

	*bounds_delta = subtract(upper_scaled, lower_scaled);
	*scaled_upper_bound = upper_scaled;
}

/** Rounds the last digit of buf so that it is closest to the converted number. */
static void round_last_digit(uint64_t rest, uint64_t w_dist, uint64_t delta,
    uint64_t digit_val_diff, char *buf, int len)
{
	/*
	 *  | <------- delta -------> |
	 *  |    | <---- w_dist ----> |
	 *  |    |       | <- rest -> |
	 *  |    |       |            |
	 *  |    |       ` buffer     |
	 *  |    ` w                  ` upper
	 *  ` lower
	 *
	 * delta = upper - lower .. conservative/safe interval
	 * w_dist = upper - w
	 * upper = "number represented by digits in buf" + rest
	 *
	 * Changing buf[len - 1] changes the value represented by buf
	 * by digit_val_diff * scaling, where scaling is shared by
	 * all parameters.
	 *
	 */

	/* Current number in buf is greater than the double being converted */
	bool cur_greater_w = rest < w_dist;
	/* Rounding down by one would keep buf in between bounds (in safe rng). */
	bool next_in_val_rng = cur_greater_w && (rest + digit_val_diff < delta);
	/* Rounding down by one would bring buf closer to the processed number. */
	bool next_closer = next_in_val_rng &&
	    (rest + digit_val_diff < w_dist || rest - w_dist < w_dist - rest);

	/*
	 * Of the shortest strings pick the one that is closest to the actual
	 * floating point number.
	 */
	while (next_closer) {
		assert('0' < buf[len - 1]);
		assert(0 < digit_val_diff);

		--buf[len - 1];
		rest += digit_val_diff;

		cur_greater_w = rest < w_dist;
		next_in_val_rng = cur_greater_w && (rest + digit_val_diff < delta);
		next_closer = next_in_val_rng &&
		    (rest + digit_val_diff < w_dist || rest - w_dist < w_dist - rest);
	}
}

/** Generates the shortest accurate decimal string representation.
 *
 * Outputs (mostly) the shortest accurate string representation
 * for the number scaled_upper - val_dist. Numbers in the interval
 * [scaled_upper - delta, scaled_upper] have the same binary
 * floating point representation and will therefore share the
 * shortest string representation (up to the rounding of the last
 * digit to bring the shortest string also the closest to the
 * actual number).
 *
 * @param scaled_upper Scaled upper bound of numbers that have the
 *              same binary representation as the converted number.
 *              Scaled by 10^-scale so that alpha <= exponent <= gamma.
 * @param delta scaled_upper - delta is the lower bound of numbers
 *              that share the same binary representation in double.
 * @param val_dist scaled_upper - val_dist is the number whose
 *              decimal string we're generating.
 * @param scale Decimal scaling of the value to convert (ie scaled_upper).
 * @param buf   Buffer to store the string representation. Must be large
 *              enough to store all digits and a null terminator. At most
 *              MAX_DOUBLE_STR_LEN digits will be written (not counting
 *              the null terminator).
 * @param buf_size Size of buf in bytes.
 * @param dec_exponent Will be set to the decimal exponent of the number
 *              string in buf.
 *
 * @return Number of digits; negative on failure (eg buffer too small).
 */
static int gen_dec_digits(fp_num_t scaled_upper, fp_num_t delta,
    fp_num_t val_dist, int scale, char *buf, size_t buf_size, int *dec_exponent)
{
	/*
	 * The integral part of scaled_upper is 5 to 32 bits long while
	 * the remaining fractional part is 59 to 32 bits long because:
	 * -59 == alpha <= scaled_upper.e <= gamma == -32
	 *
	 *  | <------- delta -------> |
	 *  |    | <--- val_dist ---> |
	 *  |    |    |<- remainder ->|
	 *  |    |    |               |
	 *  |    |    ` buffer        |
	 *  |    ` val                ` upper
	 *  ` lower
	 *
	 */
	assert(scaled_upper.significand != 0);
	assert(alpha <= scaled_upper.exponent && scaled_upper.exponent <= gamma);
	assert(scaled_upper.exponent == delta.exponent);
	assert(scaled_upper.exponent == val_dist.exponent);
	assert(val_dist.significand <= delta.significand);

	/* We'll produce at least one digit and a null terminator. */
	if (buf_size < 2) {
		return -1;
	}

	/* one is number 1 encoded with the same exponent as scaled_upper */
	fp_num_t one;
	one.significand = ((uint64_t) 1) << (-scaled_upper.exponent);
	one.exponent = scaled_upper.exponent;

	/*
	 * Extract the integral part of scaled_upper.
	 *  upper / one == upper >> -one.e
	 */
	uint32_t int_part = (uint32_t)(scaled_upper.significand >> (-one.exponent));

	/*
	 * Fractional part of scaled_upper.
	 *  upper % one == upper & (one.f - 1)
	 */
	uint64_t frac_part = scaled_upper.significand & (one.significand - 1);

	/*
	 * The integral part of upper has at least 5 bits (64 + alpha) and
	 * at most 32 bits (64 + gamma). The integral part has at most 10
	 * decimal digits, so kappa <= 10.
	 */
	int kappa = 10;
	uint32_t div = 1000000000;
	size_t len = 0;

	/* Produce decimal digits for the integral part of upper. */
	while (kappa > 0) {
		int digit = int_part / div;
		int_part %= div;

		--kappa;

		/* Skip leading zeros. */
		if (digit != 0 || len != 0) {
			/* Current length + new digit + null terminator <= buf_size */
			if (len + 2 <= buf_size) {
				buf[len] = '0' + digit;
				++len;
			} else {
				return -1;
			}
		}

		/*
		 * Difference between the so far produced decimal number and upper
		 * is calculated as: remaining_int_part * one + frac_part
		 */
		uint64_t remainder = (((uint64_t)int_part) << -one.exponent) + frac_part;

		/* The produced decimal number would convert back to upper. */
		if (remainder <= delta.significand) {
			assert(0 < len && len < buf_size);
			*dec_exponent = kappa - scale;
			buf[len] = '\0';

			/* Of the shortest representations choose the numerically closest. */
			round_last_digit(remainder, val_dist.significand, delta.significand,
			    (uint64_t)div << (-one.exponent), buf, len);
			return len;
		}

		div /= 10;
	}

	/* Generate decimal digits for the fractional part of upper. */
	do {
		/*
		 * Does not overflow because at least 5 upper bits were
		 * taken by the integral part and are now unused in frac_part.
		 */
		frac_part *= 10;
		delta.significand *= 10;
		val_dist.significand *= 10;

		/* frac_part / one */
		int digit = (int)(frac_part >> (-one.exponent));

		/* frac_part %= one */
		frac_part &= one.significand - 1;

		--kappa;

		/* Skip leading zeros. */
		if (digit == 0 && len == 0) {
			continue;
		}

		/* Current length + new digit + null terminator <= buf_size */
		if (len + 2 <= buf_size) {
			buf[len] = '0' + digit;
			++len;
		} else {
			return -1;
		}
	} while (frac_part > delta.significand);

	assert(0 < len && len < buf_size);

	*dec_exponent = kappa - scale;
	buf[len] = '\0';

	/* Of the shortest representations choose the numerically closest one. */
	round_last_digit(frac_part, val_dist.significand, delta.significand,
	    one.significand, buf, len);

	return len;
}

/** Produce a string for 0.0 */
static int zero_to_str(char *buf, size_t buf_size, int *dec_exponent)
{
	if (2 <= buf_size) {
		buf[0] = '0';
		buf[1] = '\0';
		*dec_exponent = 0;
		return 1;
	} else {
		return -1;
	}
}

/** Converts a non-special double into its shortest accurate string
 *  representation.
 *
 * Produces an accurate string representation, ie the string will
 * convert back to the same binary double (eg via strtod). In the
 * vast majority of cases (99%) the string will be the shortest such
 * string that is also the closest to the value of any shortest
 * string representations. Therefore, no trailing zeros are ever
 * produced.
 *
 * Conceptually, the value is: buf * 10^dec_exponent
 *
 * Never outputs trailing zeros.
 *
 * @param ieee_val Binary double description to convert. Must be the product
 *                 of extract_ieee_double and it must not be a special number.
 * @param buf      Buffer to store the string representation. Must be large
 *                 enough to store all digits and a null terminator. At most
 *                 MAX_DOUBLE_STR_LEN digits will be written (not counting
 *                 the null terminator).
 * @param buf_size Size of buf in bytes.
 * @param dec_exponent Will be set to the decimal exponent of the number
 *                 string in buf.
 *
 * @return The number of printed digits. A negative value indicates
 *         an error: buf too small (or ieee_val.is_special).
 */
int double_to_short_str(ieee_double_t ieee_val, char *buf, size_t buf_size,
    int *dec_exponent)
{
	/* The whole computation assumes 64bit significand. */
	static_assert(sizeof(ieee_val.pos_val.significand) == sizeof(uint64_t), "");

	if (ieee_val.is_special) {
		return -1;
	}

	/* Zero cannot be normalized. Handle it here. */
	if (0 == ieee_val.pos_val.significand) {
		return zero_to_str(buf, buf_size, dec_exponent);
	}

	fp_num_t scaled_upper_bound;
	fp_num_t delta;
	fp_num_t val_dist;
	int scale;

	calc_scaled_bounds(ieee_val, &scaled_upper_bound,
	    &delta, &val_dist, &scale);

	int len = gen_dec_digits(scaled_upper_bound, delta, val_dist, scale,
	    buf, buf_size, dec_exponent);

	assert(len <= MAX_DOUBLE_STR_LEN);
	return len;
}

/** Generates a fixed number of decimal digits of w_scaled.
 *
 * double == w_scaled * 10^-scale, where alpha <= w_scaled.e <= gamma
 *
 * @param w_scaled Scaled number by 10^-scale so that
 *              alpha <= exponent <= gamma
 * @param scale Decimal scaling of the value to convert (ie w_scaled).
 * @param signif_d_cnt Maximum number of significant digits to output.
 *              Negative if as many as possible are requested.
 * @param frac_d_cnt   Maximum number of fractional digits to output.
 *              Negative if as many as possible are requested.
 *              Eg. if 2 then 1.234 -> "1.23"; if 2 then 3e-9 -> "0".
 * @param buf   Buffer to store the string representation. Must be large
 *              enough to store all digits and a null terminator. At most
 *              MAX_DOUBLE_STR_LEN digits will be written (not counting
 *              the null terminator).
 * @param buf_size Size of buf in bytes.
 *
 * @return Number of digits; negative on failure (eg buffer too small).
 */
static int gen_fixed_dec_digits(fp_num_t w_scaled, int scale, int signif_d_cnt,
    int frac_d_cnt, char *buf, size_t buf_size, int *dec_exponent)
{
	/* We'll produce at least one digit and a null terminator. */
	if (0 == signif_d_cnt || buf_size < 2) {
		return -1;
	}

	/*
	 * The integral part of w_scaled is 5 to 32 bits long while the
	 * remaining fractional part is 59 to 32 bits long because:
	 * -59 == alpha <= w_scaled.e <= gamma == -32
	 *
	 * Therefore:
	 *  | 5..32 bits | 32..59 bits | == w_scaled == w * 10^scale
	 *  |  int_part  |  frac_part  |
	 *  |0 0  ..  0 1|0 0   ..  0 0| == one == 1.0
	 *  |      0     |0 0   ..  0 1| == w_err == 1 * 2^w_scaled.e
	 */
	assert(alpha <= w_scaled.exponent && w_scaled.exponent <= gamma);
	assert(0 != w_scaled.significand);

	/*
	 * Scaling the number being converted by 10^scale introduced
	 * an error of less that 1 ulp. The actual value of w_scaled
	 * could lie anywhere between w_scaled.signif +/- w_err.
	 * Scale the error locally as we scale the fractional part
	 * of w_scaled.
	 */
	uint64_t w_err = 1;

	/* one is number 1.0 encoded with the same exponent as w_scaled */
	fp_num_t one;
	one.significand = ((uint64_t) 1) << (-w_scaled.exponent);
	one.exponent = w_scaled.exponent;

	/*
	 * Extract the integral part of w_scaled.
	 * w_scaled / one == w_scaled >> -one.e
	 */
	uint32_t int_part = (uint32_t)(w_scaled.significand >> (-one.exponent));

	/*
	 * Fractional part of w_scaled.
	 * w_scaled % one == w_scaled & (one.f - 1)
	 */
	uint64_t frac_part = w_scaled.significand & (one.significand - 1);

	size_t len = 0;
	/*
	 * The integral part of w_scaled has at least 5 bits (64 + alpha = 5)
	 * and at most 32 bits (64 + gamma = 32). The integral part has
	 * at most 10 decimal digits, so kappa <= 10.
	 */
	int kappa = 10;
	uint32_t div = 1000000000;

	int rem_signif_d_cnt = signif_d_cnt;
	int rem_frac_d_cnt =
	    (frac_d_cnt >= 0) ? (kappa - scale + frac_d_cnt) : INT_MAX;

	/* Produce decimal digits for the integral part of w_scaled. */
	while (kappa > 0 && rem_signif_d_cnt != 0 && rem_frac_d_cnt > 0) {
		int digit = int_part / div;
		int_part %= div;

		div /= 10;
		--kappa;
		--rem_frac_d_cnt;

		/* Skip leading zeros. */
		if (digit == 0 && len == 0) {
			continue;
		}

		/* Current length + new digit + null terminator <= buf_size */
		if (len + 2 <= buf_size) {
			buf[len] = '0' + digit;
			++len;
			--rem_signif_d_cnt;
		} else {
			return -1;
		}
	}

	/* Generate decimal digits for the fractional part of w_scaled. */
	while (w_err <= frac_part && rem_signif_d_cnt != 0 && rem_frac_d_cnt > 0) {
		/*
		 * Does not overflow because at least 5 upper bits were
		 * taken by the integral part and are now unused in frac_part.
		 */
		frac_part *= 10;
		w_err *= 10;

		/* frac_part / one */
		int digit = (int)(frac_part >> (-one.exponent));

		/* frac_part %= one */
		frac_part &= one.significand - 1;

		--kappa;
		--rem_frac_d_cnt;

		/* Skip leading zeros. */
		if (digit == 0 && len == 0) {
			continue;
		}

		/* Current length + new digit + null terminator <= buf_size */
		if (len + 2 <= buf_size) {
			buf[len] = '0' + digit;
			++len;
			--rem_signif_d_cnt;
		} else {
			return -1;
		}
	}

	assert(/* 0 <= len && */ len < buf_size);

	if (0 < len) {
		*dec_exponent = kappa - scale;
		assert(frac_d_cnt < 0 || -frac_d_cnt <= *dec_exponent);
	} else {
		/*
		 * The number of fractional digits was too limiting to produce
		 * any digits.
		 */
		assert(rem_frac_d_cnt <= 0 || w_scaled.significand == 0);
		*dec_exponent = 0;
		buf[0] = '0';
		len = 1;
	}

	if (len < buf_size) {
		buf[len] = '\0';
		assert(signif_d_cnt < 0 || (int)len <= signif_d_cnt);
		return len;
	} else {
		return -1;
	}
}

/** Converts a non-special double into its string representation.
 *
 * Conceptually, the truncated double value is: buf * 10^dec_exponent
 *
 * Conversion errors are tracked, so all produced digits except the
 * last one are accurate. Garbage digits are never produced.
 * If the requested number of digits cannot be produced accurately
 * due to conversion errors less digits are produced than requested
 * and the last digit has an error of +/- 1 (so if '7' is the last
 * converted digit it might have been converted to any of '6'..'8'
 * had the conversion been completely precise).
 *
 * If no error occurs at least one digit is output.
 *
 * The conversion stops once the requested number of significant or
 * fractional digits is reached or the conversion error is too large
 * to generate any more digits (whichever happens first).
 *
 * Any digits following the first (most-significant) digit (this digit
 * included) are counted as significant digits; eg:
 *   1.4,    4 signif -> "1400" * 10^-3, ie 1.400
 *   1000.3, 1 signif -> "1" * 10^3      ie 1000
 *   0.003,  2 signif -> "30" * 10^-4    ie 0.003
 *   9.5     1 signif -> "9" * 10^0,     ie 9
 *
 * Any digits following the decimal point are counted as fractional digits.
 * This includes the zeros that would appear between the decimal point
 * and the first non-zero fractional digit. If fewer fractional digits
 * are requested than would allow to place the most-significant digit
 * a "0" is output. Eg:
 *   1.4,   3 frac -> "1400" * 10^-3,   ie 1.400
 *   12.34  4 frac -> "123400" * 10^-4, ie 12.3400
 *   3e-99  4 frac -> "0" * 10^0,       ie 0
 *   0.009  2 frac -> "0" * 10^-2,      ie 0
 *
 * @param ieee_val Binary double description to convert. Must be the product
 *                 of extract_ieee_double and it must not be a special number.
 * @param signif_d_cnt Maximum number of significant digits to produce.
 *                 The output is not rounded.
 *                 Set to a negative value to generate as many digits
 *                 as accurately possible.
 * @param frac_d_cnt Maximum number of fractional digits to produce including
 *                 any zeros immediately trailing the decimal point.
 *                 The output is not rounded.
 *                 Set to a negative value to generate as many digits
 *                 as accurately possible.
 * @param buf      Buffer to store the string representation. Must be large
 *                 enough to store all digits and a null terminator. At most
 *                 MAX_DOUBLE_STR_LEN digits will be written (not counting
 *                 the null terminator).
 * @param buf_size Size of buf in bytes.
 * @param dec_exponent Set to the decimal exponent of the number string
 *                 in buf.
 *
 * @return The number of output digits. A negative value indicates
 *         an error: buf too small (or ieee_val.is_special, or
 *         signif_d_cnt == 0).
 */
int double_to_fixed_str(ieee_double_t ieee_val, int signif_d_cnt,
    int frac_d_cnt, char *buf, size_t buf_size, int *dec_exponent)
{
	/* The whole computation assumes 64bit significand. */
	static_assert(sizeof(ieee_val.pos_val.significand) == sizeof(uint64_t), "");

	if (ieee_val.is_special) {
		return -1;
	}

	/* Zero cannot be normalized. Handle it here. */
	if (0 == ieee_val.pos_val.significand) {
		return zero_to_str(buf, buf_size, dec_exponent);
	}

	/* Normalize and scale. */
	fp_num_t w = normalize(ieee_val.pos_val);

	int lower_bin_exp = alpha - w.exponent - significand_width;

	int scale;
	fp_num_t scaling_power_of_10;

	get_power_of_ten(lower_bin_exp, &scaling_power_of_10, &scale);

	fp_num_t w_scaled = multiply(w, scaling_power_of_10);

	/* Produce decimal digits from the scaled number. */
	int len = gen_fixed_dec_digits(w_scaled, scale, signif_d_cnt, frac_d_cnt,
	    buf, buf_size, dec_exponent);

	assert(len <= MAX_DOUBLE_STR_LEN);
	return len;
}
