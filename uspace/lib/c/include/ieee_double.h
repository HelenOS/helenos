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

#ifndef IEEE_DOUBLE_H_
#define IEEE_DOUBLE_H_

#include <stdint.h>
#include <stdbool.h>

/** Represents a non-negative floating point number: significand * 2^exponent */
typedef struct fp_num_t_tag {
	/** Significand (aka mantissa). */
	uint64_t significand;
	/** Binary exponent. */
	int exponent;
} fp_num_t;

/** Double number description according to IEEE 754. */
typedef struct ieee_double_t_tag {
	/** The number is a NaN or infinity. */
	bool is_special;
	/** Not a number. */
	bool is_nan;
	bool is_negative;
	/** The number denoted infinity. */
	bool is_infinity;
	/** The number could not be represented as a normalized double. */
	bool is_denormal;
	/**
	 * The predecessor double is closer than the successor. This happens
	 * if a normal number is of the form 2^k and it is not the smallest
	 * normal number.
	 */
	bool is_accuracy_step;
	/**
	 * If !is_special the double's value is:
	 *   pos_val.significand * 2^pos_val.exponent
	 */
	fp_num_t pos_val;
} ieee_double_t;

extern ieee_double_t extract_ieee_double(double);

#endif
