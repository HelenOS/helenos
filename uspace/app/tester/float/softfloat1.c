/*
 * Copyright (c) 2012 Martin Decky
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <mathtypes.h>
#include <add.h>
#include <sub.h>
#include <mul.h>
#include <div.h>
#include <comparison.h>
#include <conversion.h>
#include "../tester.h"

#define add_float  __addsf3
#define sub_float  __subsf3
#define mul_float  __mulsf3
#define div_float  __divsf3

#define is_float_lt  __ltsf2
#define is_float_gt  __gtsf2
#define is_float_eq  __eqsf2

#define add_double  __adddf3
#define sub_double  __subdf3
#define mul_double  __muldf3
#define div_double  __divdf3

#define is_double_lt  __ltdf2
#define is_double_gt  __gtdf2
#define is_double_eq  __eqdf2

#define uint_to_double  __floatsidf
#define double_to_uint  __fixunsdfsi
#define double_to_int   __fixdfsi

#define OPERANDS   10
#define PRECISION  1000

#define PRIdCMPTYPE  PRId32

typedef int32_t cmptype_t;

typedef void (*uint_to_double_op_t)(unsigned int, double *, double *);
typedef void (*double_to_uint_op_t)(double, unsigned int *, unsigned int *);
typedef void (*float_binary_op_t)(float, float, float *, float *);
typedef void (*float_cmp_op_t)(float, float, cmptype_t *, cmptype_t *);
typedef void (*double_binary_op_t)(double, double, double *, double *);
typedef void (*double_cmp_op_t)(double, double, cmptype_t *, cmptype_t *);

typedef void (*template_unary_t)(void *, unsigned, cmptype_t *, cmptype_t *);
typedef void (*template_binary_t)(void *, unsigned, unsigned, cmptype_t *,
    cmptype_t *);

#define NUMBERS \
	3.5, -2.1, 100.0, 50.0, -1024.0, 0.0, 768.3156, 1080.499999, -600.0, 1.0

static float fop_a[OPERANDS] = {
	NUMBERS
};

static double dop_a[OPERANDS] = {
	NUMBERS
};

static unsigned int uop_a[OPERANDS] = {
	4, 2, 100, 50, 1024, 0, 1000000, 1, 0x8000000, 500
};

static int fcmp(float a, float b)
{
	if (a < b)
		return -1;

	if (a > b)
		return 1;

	return 0;
}

static int dcmp(double a, double b)
{
	if (a < b)
		return -1;

	if (a > b)
		return 1;

	return 0;
}

static void uint_to_double_template(void *f, unsigned i, cmptype_t *pic,
    cmptype_t *pisc)
{
	uint_to_double_op_t op = (uint_to_double_op_t) f;

	double c;
	double sc;
	op(uop_a[i], &c, &sc);

	*pic = (cmptype_t) (c * PRECISION);
	*pisc = (cmptype_t) (sc * PRECISION);
}

static void double_to_uint_template(void *f, unsigned i, cmptype_t *pic,
    cmptype_t *pisc)
{
	double_to_uint_op_t op = (double_to_uint_op_t) f;

	unsigned int c;
	unsigned int sc;
	op(dop_a[i], &c, &sc);

	*pic = (cmptype_t) c;
	*pisc = (cmptype_t) sc;
}

static void float_template_binary(void *f, unsigned i, unsigned j,
    cmptype_t *pic, cmptype_t *pisc)
{
	float_binary_op_t op = (float_binary_op_t) f;

	float c;
	float sc;
	op(fop_a[i], fop_a[j], &c, &sc);

	*pic = (cmptype_t) (c * PRECISION);
	*pisc = (cmptype_t) (sc * PRECISION);
}

static void float_compare_template(void *f, unsigned i, unsigned j,
    cmptype_t *pis, cmptype_t *piss)
{
	float_cmp_op_t op = (float_cmp_op_t) f;

	op(dop_a[i], dop_a[j], pis, piss);
}

static void double_template_binary(void *f, unsigned i, unsigned j,
    cmptype_t *pic, cmptype_t *pisc)
{
	double_binary_op_t op = (double_binary_op_t) f;

	double c;
	double sc;
	op(dop_a[i], dop_a[j], &c, &sc);

	*pic = (cmptype_t) (c * PRECISION);
	*pisc = (cmptype_t) (sc * PRECISION);
}

static void double_compare_template(void *f, unsigned i, unsigned j,
    cmptype_t *pis, cmptype_t *piss)
{
	double_cmp_op_t op = (double_cmp_op_t) f;

	op(dop_a[i], dop_a[j], pis, piss);
}

static bool test_template_unary(template_unary_t template, void *f)
{
	bool correct = true;

	for (unsigned int i = 0; i < OPERANDS; i++) {
		cmptype_t ic;
		cmptype_t isc;

		template(f, i, &ic, &isc);
		cmptype_t diff = ic - isc;

		if (diff != 0) {
			TPRINTF("i=%u ic=%" PRIdCMPTYPE " isc=%" PRIdCMPTYPE "\n",
			    i, ic, isc);
			correct = false;
		}
	}

	return correct;
}

static bool test_template_binary(template_binary_t template, void *f)
{
	bool correct = true;

	for (unsigned int i = 0; i < OPERANDS; i++) {
		for (unsigned int j = 0; j < OPERANDS; j++) {
			cmptype_t ic;
			cmptype_t isc;

			template(f, i, j, &ic, &isc);
			cmptype_t diff = ic - isc;

			if (diff != 0) {
				TPRINTF("i=%u, j=%u ic=%" PRIdCMPTYPE
				    " isc=%" PRIdCMPTYPE "\n", i, j, ic, isc);
				correct = false;
			}
		}
	}

	return correct;
}

static void uint_to_double_operator(unsigned int a, double *pc, double *psc)
{
	*pc = (double) a;
	*psc = uint_to_double(a);
}

static void double_to_uint_operator(double a, unsigned int *pc,
    unsigned int *psc)
{
	*pc = (unsigned int) a;
	*psc = double_to_uint(a);
}

static void double_to_int_operator(double a, unsigned int *pc,
    unsigned int *psc)
{
	*pc = (int) a;
	*psc = double_to_int(a);
}

static void float_add_operator(float a, float b, float *pc, float *psc)
{
	*pc = a + b;
	*psc = add_float(a, b);
}

static void float_sub_operator(float a, float b, float *pc, float *psc)
{
	*pc = a - b;
	*psc = sub_float(a, b);
}

static void float_mul_operator(float a, float b, float *pc, float *psc)
{
	*pc = a * b;
	*psc = mul_float(a, b);
}

static void float_div_operator(float a, float b, float *pc, float *psc)
{
	if ((cmptype_t) b == 0) {
		*pc = 0.0;
		*psc = 0.0;
		return;
	}

	*pc = a / b;
	*psc = div_float(a, b);
}

static void float_cmp_operator(float a, float b, cmptype_t *pis,
    cmptype_t *piss)
{
	*pis = fcmp(a, b);

	if (is_float_lt(a, b) == -1)
		*piss = -1;
	else if (is_float_gt(a, b) == 1)
		*piss = 1;
	else if (is_float_eq(a, b) == 0)
		*piss = 0;
	else
		*piss = 42;
}

static void double_add_operator(double a, double b, double *pc, double *psc)
{
	*pc = a + b;
	*psc = add_double(a, b);
}

static void double_sub_operator(double a, double b, double *pc, double *psc)
{
	*pc = a - b;
	*psc = sub_double(a, b);
}

static void double_mul_operator(double a, double b, double *pc, double *psc)
{
	*pc = a * b;
	*psc = mul_double(a, b);
}

static void double_div_operator(double a, double b, double *pc, double *psc)
{
	if ((cmptype_t) b == 0) {
		*pc = 0.0;
		*psc = 0.0;
		return;
	}

	*pc = a / b;
	*psc = div_double(a, b);
}

static void double_cmp_operator(double a, double b, cmptype_t *pis,
    cmptype_t *piss)
{
	*pis = dcmp(a, b);

	if (is_double_lt(a, b) == -1)
		*piss = -1;
	else if (is_double_gt(a, b) == 1)
		*piss = 1;
	else if (is_double_eq(a, b) == 0)
		*piss = 0;
	else
		*piss = 42;
}

const char *test_softfloat1(void)
{
	bool err = false;

	if (!test_template_binary(float_template_binary, float_add_operator)) {
		err = true;
		TPRINTF("%s\n", "Float addition failed");
	}

	if (!test_template_binary(float_template_binary, float_sub_operator)) {
		err = true;
		TPRINTF("%s\n", "Float addition failed");
	}

	if (!test_template_binary(float_template_binary, float_mul_operator)) {
		err = true;
		TPRINTF("%s\n", "Float multiplication failed");
	}

	if (!test_template_binary(float_template_binary, float_div_operator)) {
		err = true;
		TPRINTF("%s\n", "Float division failed");
	}

	if (!test_template_binary(float_compare_template, float_cmp_operator)) {
		err = true;
		TPRINTF("%s\n", "Float comparison failed");
	}

	if (!test_template_binary(double_template_binary, double_add_operator)) {
		err = true;
		TPRINTF("%s\n", "Double addition failed");
	}

	if (!test_template_binary(double_template_binary, double_sub_operator)) {
		err = true;
		TPRINTF("%s\n", "Double addition failed");
	}

	if (!test_template_binary(double_template_binary, double_mul_operator)) {
		err = true;
		TPRINTF("%s\n", "Double multiplication failed");
	}

	if (!test_template_binary(double_template_binary, double_div_operator)) {
		err = true;
		TPRINTF("%s\n", "Double division failed");
	}

	if (!test_template_binary(double_compare_template, double_cmp_operator)) {
		err = true;
		TPRINTF("%s\n", "Double comparison failed");
	}

	if (!test_template_unary(uint_to_double_template,
	    uint_to_double_operator)) {
		err = true;
		TPRINTF("%s\n", "Conversion from unsigned int to double failed");
	}

	if (!test_template_unary(double_to_uint_template,
	    double_to_uint_operator)) {
		err = true;
		TPRINTF("%s\n", "Conversion from double to unsigned int failed");
	}

	if (!test_template_unary(double_to_uint_template,
	    double_to_int_operator)) {
		err = true;
		TPRINTF("%s\n", "Conversion from double to signed int failed");
	}

	if (err)
		return "Software floating point imprecision";

	return NULL;
}
