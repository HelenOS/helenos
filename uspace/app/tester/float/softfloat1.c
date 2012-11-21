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
#include <sftypes.h>
#include <add.h>
#include <sub.h>
#include <mul.h>
#include <div.h>
#include <comparison.h>
#include <bool.h>
#include "../tester.h"

#define OPERANDS  6 
#define PRECISION  10000

#define PRIdCMPTYPE  PRId32

typedef int32_t cmptype_t;
typedef void (* float_op_t)(float, float, float *, float_t *);
typedef void (* double_op_t)(double, double, double *, double_t *);
typedef void (* double_cmp_op_t)(double, double, cmptype_t *, cmptype_t *);
typedef void (* template_t)(void *, unsigned, unsigned, cmptype_t *,
    cmptype_t *);

static float fop_a[OPERANDS] =
	{3.5, -2.1, 100.0, 50.0, -1024.0, 0.0};

static float fop_b[OPERANDS] =
	{-2.1, 100.0, 50.0, -1024.0, 3.5, 0.0};

static double dop_a[OPERANDS] =
	{3.5, -2.1, 100.0, 50.0, -1024.0, 0.0};

static double dop_b[OPERANDS] =
	{-2.1, 100.0, 50.0, -1024.0, 3.5, 0.0};

static cmptype_t cmpabs(cmptype_t a)
{
	if (a >= 0)
		return a;
	
	return -a;
}

static int dcmp(double a, double b)
{
	if (a < b)
		return -1;
	else if (a > b)
		return 1;

	return 0;
}

static void
float_template(void *f, unsigned i, unsigned j, cmptype_t *pic,
    cmptype_t *pisc)
{
	float c;
	float_t sc;

	float_op_t op = (float_op_t) f;
	
	op(fop_a[i], fop_b[j], &c, &sc);

	*pic = (cmptype_t) (c * PRECISION);
	*pisc = (cmptype_t) (sc.val * PRECISION);
}

static void
double_template(void *f, unsigned i, unsigned j, cmptype_t *pic,
    cmptype_t *pisc)
{
	double c;
	double_t sc;

	double_op_t op = (double_op_t) f;
	
	op(dop_a[i], dop_b[j], &c, &sc);

	*pic = (cmptype_t) (c * PRECISION);
	*pisc = (cmptype_t) (sc.val * PRECISION);
}

static void
double_compare_template(void *f, unsigned i, unsigned j, cmptype_t *pis,
    cmptype_t *piss)
{
	double_cmp_op_t op = (double_cmp_op_t) f;
	
	op(dop_a[i], dop_b[j], pis, piss);
}

static bool test_template(template_t template, void *f)
{
	bool correct = true;
	
	for (unsigned int i = 0; i < OPERANDS; i++) {
		for (unsigned int j = 0; j < OPERANDS; j++) {			
			cmptype_t ic;
			cmptype_t isc;

			template(f, i, j, &ic, &isc);	
			cmptype_t diff = cmpabs(ic - isc);
			
			if (diff != 0) {
				TPRINTF("i=%u, j=%u diff=%" PRIdCMPTYPE "\n",
				    i, j, diff);
				correct = false;
			}
		}
	}
	
	return correct;
}

static void float_add_operator(float a, float b, float *pc, float_t *psc)
{
	*pc = a + b;
	
	float_t sa;
	float_t sb;
	
	sa.val = a;
	sb.val = b;
	if (sa.data.parts.sign == sb.data.parts.sign)
		psc->data = add_float(sa.data, sb.data);
	else if (sa.data.parts.sign) {
		sa.data.parts.sign = 0;
		psc->data = sub_float(sb.data, sa.data);
	} else {
		sb.data.parts.sign = 0;
		psc->data = sub_float(sa.data, sb.data);
	}
}

static void float_mul_operator(float a, float b, float *pc, float_t *psc)
{
	*pc = a * b;
	
	float_t sa;
	float_t sb;
	
	sa.val = a;
	sb.val = b;
	psc->data = mul_float(sa.data, sb.data);
}

static void float_div_operator(float a, float b, float *pc, float_t *psc)
{
	if ((cmptype_t) b == 0) {
		*pc = 0.0;
		psc->val = 0.0;
		return;
	}

	*pc = a / b;
	
	float_t sa;
	float_t sb;
	
	sa.val = a;
	sb.val = b;
	psc->data = div_float(sa.data, sb.data);
}

static void double_add_operator(double a, double b, double *pc, double_t *psc)
{
	*pc = a + b;
	
	double_t sa;
	double_t sb;
	
	sa.val = a;
	sb.val = b;
	if (sa.data.parts.sign == sb.data.parts.sign)
		psc->data = add_double(sa.data, sb.data);
	else if (sa.data.parts.sign) {
		sa.data.parts.sign = 0;
		psc->data = sub_double(sb.data, sa.data);
	} else {
		sb.data.parts.sign = 0;
		psc->data = sub_double(sa.data, sb.data);
	}
}

static void double_mul_operator(double a, double b, double *pc, double_t *psc)
{
	*pc = a * b;
	
	double_t sa;
	double_t sb;
	
	sa.val = a;
	sb.val = b;
	psc->data = mul_double(sa.data, sb.data);
}

static void double_div_operator(double a, double b, double *pc, double_t *psc)
{
	if ((cmptype_t) b == 0) {
		*pc = 0.0;
		psc->val = 0.0;
		return;
	}

	*pc = a / b;
	
	double_t sa;
	double_t sb;
	
	sa.val = a;
	sb.val = b;
	psc->data = div_double(sa.data, sb.data);
}

static void
double_cmp_operator(double a, double b, cmptype_t *pis, cmptype_t *piss)
{
	*pis = dcmp(a, b);

	double_t sa;
	double_t sb;

	sa.val = a;
	sb.val = b;

	if (is_double_lt(sa.data, sb.data))
		*piss = -1;
	else if (is_double_gt(sa.data, sb.data))
		*piss = 1;
	else if (is_double_eq(sa.data, sb.data))
		*piss = 0;
	else
		*piss = 42;
}

const char *test_softfloat1(void)
{
	const char *err = NULL;

	if (!test_template(float_template, float_add_operator)) {
		err = "Float addition failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(float_template, float_mul_operator)) {
		err = "Float multiplication failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(float_template, float_div_operator)) {
		err = "Float division failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(double_template, double_add_operator)) {
		err = "Double addition failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(double_template, double_mul_operator)) {
		err = "Double multiplication failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(double_template, double_div_operator)) {
		err = "Double division failed";
		TPRINTF("%s\n", err);
	}
	if (!test_template(double_compare_template, double_cmp_operator)) {
		err = "Double comparison failed";
		TPRINTF("%s\n", err);
	}
	
	return err;
}

