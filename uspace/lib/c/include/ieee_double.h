/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
