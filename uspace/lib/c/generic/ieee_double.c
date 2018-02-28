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

#include <ieee_double.h>

#include <assert.h>

/** Returns an easily processible description of the double val.
 */
ieee_double_t extract_ieee_double(double val)
{
	const uint64_t significand_mask = 0xfffffffffffffULL;
	const uint64_t exponent_mask = 0x7ff0000000000000ULL;
	const int exponent_shift = 64 - 11 - 1;
	const uint64_t sign_mask = 0x8000000000000000ULL;

	const int special_exponent = 0x7ff;
	const int denormal_exponent = 0;
	const uint64_t hidden_bit = (1ULL << 52);
	const int exponent_bias = 1075;

	static_assert(sizeof(val) == sizeof(uint64_t));

	union {
		uint64_t num;
		double val;
	} bits;

	bits.val = val;

	/*
	 * Extract the binary ieee representation of the double.
	 * Relies on integers having the same endianness as doubles.
	 */
	uint64_t num = bits.num;

	ieee_double_t ret;

	/* Determine the sign. */
	ret.is_negative = ((num & sign_mask) != 0);

	/* Extract the exponent. */
	int raw_exponent = (num & exponent_mask) >> exponent_shift;

	/* The extracted raw significand may not contain the hidden bit */
	uint64_t raw_significand = num & significand_mask;

	ret.is_special = (raw_exponent == special_exponent);

	/* NaN or infinity */
	if (ret.is_special) {
		ret.is_infinity = (raw_significand == 0);
		ret.is_nan = (raw_significand != 0);

		/* These are not valid for special numbers but init them anyway. */
		ret.is_denormal = true;
		ret.is_accuracy_step = false;
		ret.pos_val.significand = 0;
		ret.pos_val.exponent = 0;
	} else {
		ret.is_infinity = false;
		ret.is_nan = false;

		ret.is_denormal = (raw_exponent == denormal_exponent);

		/* Denormal or zero. */
		if (ret.is_denormal) {
			ret.pos_val.significand = raw_significand;
			ret.pos_val.exponent = 1 - exponent_bias;
			ret.is_accuracy_step = false;
		} else {
			ret.pos_val.significand = raw_significand + hidden_bit;
			ret.pos_val.exponent = raw_exponent - exponent_bias;

			/* The predecessor is closer to val than the successor
			 * if val is a normal value of the form 2^k (hence
			 * raw_significand == 0) with the only exception being
			 * the smallest normal (raw_exponent == 1). The smallest
			 * normal's predecessor is the largest denormal and denormals
			 * do not get an extra bit of precision because their exponent
			 * stays the same (ie it does not decrease from k to k-1).
			 */
			ret.is_accuracy_step = (raw_significand == 0) && (raw_exponent != 1);
		}
	}

	return ret;
}

