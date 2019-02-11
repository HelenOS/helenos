/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @file Big integers.
 *
 * Sysel type @c int should accomodate large numbers. This implementation
 * is limited by the available memory and range of the @c size_t type used
 * to index digits.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "mytypes.h"

#include "bigint.h"

static void bigint_sign_comb(bool_t srf_a, bigint_t *a, bool_t srf_b,
    bigint_t *b, bigint_t *dest);
static void bigint_add_abs(bigint_t *a, bigint_t *b, bigint_t *dest);
static void bigint_sub_abs(bigint_t *a, bigint_t *b, bigint_t *dest);
static void bigint_shift_mul_dig(bigint_t *a, bigint_word_t b, size_t shift,
    bigint_t *dest);

static void bigint_alloc(bigint_t *bigint, size_t length);
static void bigint_refine_len(bigint_t *bigint);

/** Initialize bigint with value from small integer.
 *
 * Initializes a bigint structure with the provided small value.
 *
 * @param value		Initial value (small int).
 */
void bigint_init(bigint_t *bigint, int value)
{
	size_t length;
	size_t idx;
	int t;

#ifdef DEBUG_BIGINT_TRACE
	printf("Initialize bigint with int value %d.\n", value);
#endif

	if (value < 0) {
		bigint->negative = b_true;
		value = -value;
	} else {
		bigint->negative = b_false;
	}

	/* Determine length. */
	length = 0;
	t = value;
	while (t > 0) {
		length += 1;
		t = t / BIGINT_BASE;
	}

	/* Allocate digit array. */
	bigint_alloc(bigint, length);

	/* Compute digits. */
	t = value;
	for (idx = 0; idx < length; ++idx) {
		bigint->digit[idx] = t % BIGINT_BASE;
		t = t / BIGINT_BASE;
	}
}

/** Shallow copy of integer.
 *
 * @param src		Source.
 * @param dest		Destination.
 */
void bigint_shallow_copy(bigint_t *src, bigint_t *dest)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Shallow copy of bigint.\n");
#endif
	dest->negative = src->negative;
	dest->digit = src->digit;
	dest->length = src->length;
}
/** Clone big integer.
 *
 * @param src		Source.
 * @param dest		Destination.
 */
void bigint_clone(bigint_t *src, bigint_t *dest)
{
	size_t idx;

#ifdef DEBUG_BIGINT_TRACE
	printf("Clone bigint.\n");
#endif
	/* Copy sign. */
	dest->negative = src->negative;

	/* Allocate dest digit array. */
	bigint_alloc(dest, src->length);

	/* Copy digits. */
	for (idx = 0; idx < dest->length; ++idx)
		dest->digit[idx] = src->digit[idx];
}

/** Compute big integer with reversed sign.
 *
 * @param src		Source.
 * @param dest		Destination.
 */
void bigint_reverse_sign(bigint_t *src, bigint_t *dest)
{
	size_t idx;

#ifdef DEBUG_BIGINT_TRACE
	printf("Reverse-sign copy of bigint.\n");
#endif
	/* Copy reversed sign. */
	dest->negative = !src->negative;

	/* Allocate dest digit array. */
	bigint_alloc(dest, src->length);

	/* Copy digits. */
	for (idx = 0; idx < dest->length; ++idx)
		dest->digit[idx] = src->digit[idx];
}

/** Destroy big integer.
 *
 * Any bigint that is initialized via bigint_init() or any other bigint
 * function that constructs a new bigint value should be destroyed with
 * this function to free memory associated with the bigint. It should
 * also be used to destroy a bigint before it is reused.
 *
 * @param bigint	The bigint to destroy.
 */
void bigint_destroy(bigint_t *bigint)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Destroy bigint.\n");
#endif
	bigint->negative = b_false;

	bigint->length = 0;

	free(bigint->digit);
	bigint->digit = NULL;
}

/** Get value of big integer.
 *
 * Allows to obtain the value of big integer, provided that it fits
 * into a small integer.
 *
 * @param bigint	Bigint to obtain value from.
 * @param dval		Place to store value.
 * @return		EOK on success, ELIMIT if bigint is too big to fit
 *			to @a dval.
 */
errno_t bigint_get_value_int(bigint_t *bigint, int *dval)
{
	bigint_t vval, diff;
	size_t idx;
	int val;
	bool_t zf;

#ifdef DEBUG_BIGINT_TRACE
	printf("Get int value of bigint.\n");
#endif
	val = 0;
	for (idx = 0; idx < bigint->length; ++idx) {
		val = val * BIGINT_BASE + bigint->digit[idx];
	}

	if (bigint->negative)
		val = -val;

	/* If the value did not fit @c val now contains garbage. Verify. */
	bigint_init(&vval, val);

	bigint_sub(bigint, &vval, &diff);
	zf = bigint_is_zero(&diff);

	bigint_destroy(&vval);
	bigint_destroy(&diff);

	/* If the difference is not zero, the verification failed. */
	if (zf != b_true)
		return EINVAL;

	*dval = val;
	return EOK;
}

/** Determine if bigint is zero.
 *
 * @param bigint	Big integer.
 * @return		b_true if @a bigint is zero, b_false otherwise.
 */
bool_t bigint_is_zero(bigint_t *bigint)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Determine if bigint is zero.\n");
#endif
	return (bigint->length == 0);
}

/** Determine if bigint is negative.
 *
 * @param bigint	Big integer.
 * @return		b_true if @a bigint is negative, b_false otherwise.
 */
bool_t bigint_is_negative(bigint_t *bigint)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Determine if bigint is negative\n");
#endif
	/* Verify that we did not accidentaly introduce a negative zero. */
	assert(bigint->negative == b_false || bigint->length > 0);

	return bigint->negative;
}

/** Divide bigint by (unsigned) digit.
 *
 * @param a		Bigint dividend.
 * @param b		Divisor digit.
 * @param quot		Output bigint quotient.
 * @param rem		Output remainder digit.
 */
void bigint_div_digit(bigint_t *a, bigint_word_t b, bigint_t *quot,
    bigint_word_t *rem)
{
	size_t lbound;
	size_t idx;
	bigint_dword_t da, db;
	bigint_dword_t q, r;

#ifdef DEBUG_BIGINT_TRACE
	printf("Divide bigint by digit.\n");
#endif
	lbound = a->length;
	bigint_alloc(quot, lbound);

	quot->negative = a->negative;

	r = 0;
	idx = lbound;
	while (idx > 0) {
		--idx;

		da = a->digit[idx] + r * BIGINT_BASE;
		db = b;

		q = da / db;
		r = da % db;

		quot->digit[idx] = q;
	}

	bigint_refine_len(quot);
	*rem = r;
}

/** Add two big integers.
 *
 * The big integers @a a and @a b are added and the result is stored in
 * @a dest.
 *
 * @param a		First addend.
 * @param b		Second addend.
 * @param dest		Destination bigint.
 */
void bigint_add(bigint_t *a, bigint_t *b, bigint_t *dest)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Add bigints.\n");
#endif
	bigint_sign_comb(b_false, a, b_false, b, dest);
}

/** Subtract two big integers.
 *
 * The big integers @a a and @a b are subtracted and the result is stored in
 * @a dest.
 *
 * @param a		Minuend.
 * @param b		Subtrahend.
 * @param dest		Destination bigint.
 */
void bigint_sub(bigint_t *a, bigint_t *b, bigint_t *dest)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Subtract bigints.\n");
#endif
	bigint_sign_comb(b_false, a, b_true, b, dest);
}

/** Multiply two big integers.
 *
 * The big integers @a a and @a b are multiplied and the result is stored in
 * @a dest.
 *
 * @param a		First factor.
 * @param b		Second factor.
 * @param dest		Destination bigint.
 */
void bigint_mul(bigint_t *a, bigint_t *b, bigint_t *dest)
{
	size_t idx;
	bigint_t dprod;
	bigint_t sum;
	bigint_t tmp;

#ifdef DEBUG_BIGINT_TRACE
	printf("Multiply bigints.\n");
#endif
	bigint_init(&sum, 0);
	for (idx = 0; idx < b->length; ++idx) {
		bigint_shift_mul_dig(a, b->digit[idx], idx, &dprod);
		bigint_add(&dprod, &sum, &tmp);
		bigint_destroy(&dprod);

		bigint_destroy(&sum);
		bigint_shallow_copy(&tmp, &sum);
	}

	if (b->negative)
		sum.negative = !sum.negative;

	bigint_shallow_copy(&sum, dest);
}

/** Convert bigint to string.
 *
 * @param bigint	Bigint to convert.
 * @param dptr		Place to store pointer to new string.
 */
void bigint_get_as_string(bigint_t *bigint, char **dptr)
{
	static const char digits[] = { '0', '1', '2', '3', '4', '5', '6',
		'7', '8', '9' };

	bigint_t val, tmp;
	bigint_word_t rem;
	size_t nchars;
	char *str;
	size_t idx;

#ifdef DEBUG_BIGINT_TRACE
	printf("Convert bigint to string.\n");
#endif
	static_assert(BIGINT_BASE >= 10, "");

	/* Compute number of characters. */
	nchars = 0;

	if (bigint_is_zero(bigint) || bigint->negative)
		nchars += 1; /* '0' or '-' */

	bigint_clone(bigint, &val);
	while (bigint_is_zero(&val) != b_true) {
		bigint_div_digit(&val, 10, &tmp, &rem);
		bigint_destroy(&val);
		bigint_shallow_copy(&tmp, &val);

		nchars += 1;
	}
	bigint_destroy(&val);

	/* Store characters to array. */

	str = malloc(nchars * sizeof(char) + 1);
	if (str == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	if (bigint_is_zero(bigint)) {
		str[0] = '0';
	} else if (bigint->negative) {
		str[0] = '-';
	}

	idx = 1;
	bigint_clone(bigint, &val);
	while (bigint_is_zero(&val) != b_true) {
		bigint_div_digit(&val, 10, &tmp, &rem);
		bigint_destroy(&val);
		bigint_shallow_copy(&tmp, &val);

		str[nchars - idx] = digits[(int) rem];
		++idx;
	}

	bigint_destroy(&val);
	str[nchars] = '\0';
	*dptr = str;
}

/** Print bigint to standard output.
 *
 * @param bigint	Bigint to print.
 */
void bigint_print(bigint_t *bigint)
{
	char *str;

#ifdef DEBUG_BIGINT_TRACE
	printf("Print bigint.\n");
#endif
	bigint_get_as_string(bigint, &str);
	printf("%s", str);
	free(str);
}

/** Compute sign combination of two big integers.
 *
 * Of the big integers @a a and @a b each is optionally sign-reversed and then
 * they are added and the result is stored in @a dest.
 *
 * @param srf_a		First sign reversal flag.
 * @param a		First bigint.
 * @param srf_b		Second sign reversal flag.
 * @param b		Second bigint.
 * @param dest		Destination bigint.
 */
static void bigint_sign_comb(bool_t srf_a, bigint_t *a, bool_t srf_b,
    bigint_t *b, bigint_t *dest)
{
	bool_t neg_a, neg_b;

#ifdef DEBUG_BIGINT_TRACE
	printf("Signed combination of two bigints.\n");
#endif
	/* Compute resulting signs of combination elements. */
	neg_a = (srf_a != a->negative);
	neg_b = (srf_b != b->negative);

	if (neg_a == neg_b) {
		bigint_add_abs(a, b, dest);
		dest->negative = neg_a;
	} else {
		bigint_sub_abs(a, b, dest);
		dest->negative = (neg_a != dest->negative);
	}
}

/** Add absolute values of two big integers.
 *
 * The absolute values of big integers @a a and @a b are added and the result
 * is stored in @a dest.
 *
 * @param a		First addend.
 * @param b		Second addend.
 * @param dest		Destination bigint.
 */
static void bigint_add_abs(bigint_t *a, bigint_t *b, bigint_t *dest)
{
	size_t lbound;
	size_t idx;
	bigint_dword_t da, db;
	bigint_dword_t tmp;
	bigint_word_t res, carry;

#ifdef DEBUG_BIGINT_TRACE
	printf("Add absolute values of bigints.\n");
#endif
	/* max(a->length, b->length) + 1 */
	lbound = (a->length > b->length ? a->length : b->length) + 1;
	dest->negative = b_false;

	bigint_alloc(dest, lbound);
	carry = 0;

	for (idx = 0; idx < lbound; ++idx) {
		da = idx < a->length ? a->digit[idx] : 0;
		db = idx < b->length ? b->digit[idx] : 0;

		tmp = da + db + (bigint_word_t) carry;

		carry = (bigint_word_t) (tmp / BIGINT_BASE);
		res = (bigint_word_t) (tmp % BIGINT_BASE);

		dest->digit[idx] = res;
	}

	/* If our lbound is correct, carry must be zero. */
	assert(carry == 0);

	bigint_refine_len(dest);
}

/** Subtract absolute values of two big integers.
 *
 * The absolute values of big integers @a a and @a b are subtracted and the
 * result is stored in @a dest.
 *
 * @param a		Minuend.
 * @param b		Subtrahend.
 * @param dest		Destination bigint.
 */
static void bigint_sub_abs(bigint_t *a, bigint_t *b, bigint_t *dest)
{
	size_t lbound;
	size_t idx;
	bigint_dword_t da, db;
	bigint_dword_t tmp;
	bigint_word_t res, borrow;

#ifdef DEBUG_BIGINT_TRACE
	printf("Subtract absolute values of bigints.\n");
#endif
	/* max(a->length, b->length) */
	lbound = a->length > b->length ? a->length : b->length;

	bigint_alloc(dest, lbound);
	borrow = 0;

	for (idx = 0; idx < lbound; ++idx) {
		da = idx < a->length ? a->digit[idx] : 0;
		db = idx < b->length ? b->digit[idx] : 0;

		if (da > db + borrow) {
			tmp = da - db - borrow;
			borrow = 0;
		} else {
			tmp = da + BIGINT_BASE - db - borrow;
			borrow = 1;
		}

		res = (bigint_word_t) tmp;
		dest->digit[idx] = res;
	}

	if (borrow != 0) {
		/* We subtracted the greater number from the smaller. */
		dest->negative = b_true;

		/*
		 * Now we must complement the number to get the correct
		 * absolute value. We do this by subtracting from 10..0
		 * (0 repeated lbound-times).
		 */
		borrow = 0;

		for (idx = 0; idx < lbound; ++idx) {
			da = 0;
			db = dest->digit[idx];

			if (da > db + borrow) {
				tmp = da - db - borrow;
				borrow = 0;
			} else {
				tmp = da + BIGINT_BASE - db - borrow;
				borrow = 1;
			}

			res = (bigint_word_t) tmp;
			dest->digit[idx] = res;
		}

		/* The last step is the '1'. */
		assert(borrow == 1);
	} else {
		dest->negative = b_false;
	}

	bigint_refine_len(dest);
}

/** Multiply big integer by digit.
 *
 * @param a		Bigint factor.
 * @param b		Digit factor.
 * @param dest		Destination bigint.
 */
static void bigint_shift_mul_dig(bigint_t *a, bigint_word_t b, size_t shift,
    bigint_t *dest)
{
	size_t lbound;
	size_t idx;

	bigint_dword_t da, db;
	bigint_dword_t tmp;
	bigint_word_t res, carry;

#ifdef DEBUG_BIGINT_TRACE
	printf("Multiply bigint by digit.\n");
#endif
	/* Compute length bound and allocate. */
	lbound = a->length + shift + 1;
	bigint_alloc(dest, lbound);

	/* Copy sign. */
	dest->negative = a->negative;

	for (idx = 0; idx < shift; ++idx)
		dest->digit[idx] = 0;

	carry = 0;
	for (idx = 0; idx < lbound - shift; ++idx) {
		da = idx < a->length ? a->digit[idx] : 0;
		db = b;

		tmp = da * db + (bigint_word_t) carry;

		carry = (bigint_word_t) (tmp / BIGINT_BASE);
		res = (bigint_word_t) (tmp % BIGINT_BASE);

		dest->digit[shift + idx] = res;
	}

	/* If our lbound is correct, carry must be zero. */
	assert(carry == 0);

	bigint_refine_len(dest);
}

/** Allocate bigint of the given length.
 *
 * @param bigint	Bigint whose digit array should be allocated.
 * @param length	Length of array (also set as bigint length).
 */
static void bigint_alloc(bigint_t *bigint, size_t length)
{
	size_t a_length;

#ifdef DEBUG_BIGINT_TRACE
	printf("Allocate bigint digit array.\n");
#endif
	/* malloc() sometimes cannot allocate blocks of zero size. */
	if (length == 0)
		a_length = 1;
	else
		a_length = length;

	bigint->digit = malloc(a_length * sizeof(bigint_word_t));
	if (bigint->digit == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	bigint->length = length;
}

/** Adjust length field of bigint to be exact.
 *
 * When bigint is allocated with bigint_alloc() its length can be
 * imprecise (higher than actually number of non-zero digits).
 * Then this function is used to lower @c length to the exact value.
 */
static void bigint_refine_len(bigint_t *bigint)
{
#ifdef DEBUG_BIGINT_TRACE
	printf("Refine bigint length.\n");
#endif
	while (bigint->length > 0 && bigint->digit[bigint->length - 1] == 0)
		bigint->length -= 1;

	if (bigint->length == 0)
		bigint->negative = b_false;
}
