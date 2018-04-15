/*
 * Copyright (c) 2014 Martin Decky
 * Copyright (c) 2015 Jiri Svoboda
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "../tester.h"

#define OPERANDS         10
#define PRECISIONF    10000
#define PRECISION 100000000

static double arguments[OPERANDS] = {
	3.5, -2.1, 100.0, 50.0, -1024.0, 0.0, 768.3156, 1080.499999, -600.0, 1.0
};

static double arguments_acos[OPERANDS] = {
	-0.936456687291, -0.504846104600, 0.862318872288, 0.964966028492,
	0.987353618220, 1.0, -0.194939922623, 0.978471923925, -0.999023478833,
	0.540302305868
};

static double arguments_asin[OPERANDS] = {
	-0.350783227690, -0.863209366649, -0.506365641110, -0.262374853704,
	0.158533380044, 0.0, 0.980815184715, -0.206379975025, -0.044182448332,
	0.841470984808
};

static double arguments_atan[OPERANDS] = {
	3.5, 100.0, 50.0, 768.3156, 1080.499999, 1.0, 66.0,
	2.718281828459045, 9.9, 0.001
};

static double arguments_exp[OPERANDS] = {
	3.5, -2.1, 50.0, 0.0, 1.0, 13.2, -1.1, -5.5, 0.1, -66.0
};

static double arguments_log[OPERANDS] = {
	3.5, 100.0, 50.0, 768.3156, 1080.499999, 1.0, 66.0,
	2.718281828459045, 9.9, 0.001
};

static double arguments_sqrt[OPERANDS] = {
	3.5, 100.0, 50.0, 768.3156, 1080.499999, 1.0, 66.0,
	2.718281828459045, 9.9, 0.001
};

static double arguments_tanh[OPERANDS] = {
	3.5, -2.1, 50.0, 0.0, 1.0, 13.2, -1.1, -5.5, 0.000001, -66000000.0
};

static double results_acos[OPERANDS] = {
	2.783185307180, 2.100000000000, 0.530964914873, 0.265482457437,
	0.159205070272, 0.000000000000, 1.766992524091, 0.207873834887,
	3.097395817941, 1.000000000000
};

static double results_asin[OPERANDS] = {
	-0.358407346411, -1.041592653590, -0.530964914874, -0.265482457437,
	0.159205070273, 0.000000000000, 1.374600129498, -0.207873834889,
	-0.044196835651, 1.000000000000
};

static double results_atan[OPERANDS] = {
	1.292496667790, 1.560796660108, 1.550798992822, 1.569494779052,
	1.569870829603, 0.785398163397, 1.555645970920, 1.218282905017,
	1.470127674637, 0.000999999667
};

static double results_ceil[OPERANDS] = {
	4.0, -2.0, 100.0, 50.0, -1024.0, 0.0, 769.0, 1081.0, -600.0, 1.0
};

static double results_cos[OPERANDS] = {
	-0.936456687291, -0.504846104600, 0.862318872288, 0.964966028492,
	0.987353618220, 1.0, -0.194939922623, 0.978471923925, -0.999023478833,
	0.540302305868
};

static double results_cosh[OPERANDS] = {
	16.572824671057, 4.144313170410, 2592352764293536022528.000000000000,
	1.000000000000, 1.543080634815, 270182.468624271103, 1.668518553822,
	122.348009517829, 1.005004168056, 23035933171656458903220125696.0
};

static double results_fabs[OPERANDS] = {
	3.5, 2.1, 100.0, 50.0, 1024.0, 0.0, 768.3156, 1080.499999, 600.0, 1.0
};

static double results_floor[OPERANDS] = {
	3.0, -3.0, 100.0, 50.0, -1024.0, 0.0, 768.0, 1080.0, -600.0, 1.0
};

static double results_exp[OPERANDS] = {
	33.115451958692, 0.122456428253, 5184705528587072045056.0,
	1.000000000000, 2.718281828459, 540364.937246691552, 0.332871083698,
	0.004086771438, 1.105170918076, 0.000000000000
};

static double results_log[OPERANDS] = {
	1.252762968495, 4.605170185988, 3.912023005428, 6.644200586236,
	6.985179175021, 0.000000000000, 4.189654742026, 1.000000000000,
	2.292534757141, -6.907755278982
};

static double results_log10[OPERANDS] = {
	0.544068044350, 2.000000000000, 1.698970004336, 2.885539651261,
	3.033624770817, 0.000000000000, 1.819543935542, 0.434294481903,
	0.995635194598, -3.000000000000
};

static double results_sin[OPERANDS] = {
	-0.350783227690, -0.863209366649, -0.506365641110, -0.262374853704,
	0.158533380044, 0.0, 0.980815184715, -0.206379975025, -0.044182448332,
	0.841470984808
};

static double results_sinh[OPERANDS] = {
	16.542627287635, -4.021856742157, 2592352764293536022528.000000000000,
	0.000000000000, 1.175201193644, 270182.468622420449, -1.335647470124,
	-122.343922746391, 0.100166750020, -23035933171656458903220125696.0
};

static double results_sqrt[OPERANDS] = {
	1.870828693387, 10.000000000000, 7.071067811865, 27.718506453271,
	32.870959812576, 1.000000000000, 8.124038404636, 1.648721270700,
	3.146426544510, 0.031622776602
};

static double results_tan[OPERANDS] = {
	0.374585640159, 1.709846542905, -0.587213915157, -0.271900611998,
	0.160563932839, 0.000000000000, -5.031371570891, -0.210920691722,
	0.044225635601, 1.557407724655
};

static double results_tanh[OPERANDS] = {
	0.998177897611, -0.970451936613, 1.000000000000, 0.000000000000,
	0.761594155956, 0.999999999993, -0.800499021761, -0.999966597156,
	0.000001000000, -1.000000000000
};

static double results_trunc[OPERANDS] = {
	3.0, -2.0, 100.0, 50.0, -1024.0, 0.0, 768.0, 1080.0, -600.0, 1.0
};

static bool cmp_float(float a, float b)
{
	float r;

	/* XXX Need fabsf() */
	if (b < 1.0 / PRECISIONF && b > -1.0 / PRECISIONF)
		r = a;
	else
		r = a / b - 1.0;

	/* XXX Need fabsf() */
	if (r < 0.0)
		r = -r;

	return r < 1.0 / PRECISIONF;
}

static bool cmp_double(double a, double b)
{
	double r;

	/* XXX Need fabs() */
	if (b < 1.0 / PRECISION && b > -1.0 / PRECISION)
		r = a;
	else
		r = a / b - 1.0;

	/* XXX Need fabs() */
	if (r < 0.0)
		r = -r;

	return r < 1.0 / PRECISION;
}

const char *test_float2(void)
{
	bool fail = false;
#if 0
	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = acos(arguments_acos[i]);

		if (!cmp_double(res, results_acos[i])) {
			TPRINTF("Double precision acos failed "
			    "(%lf != %lf, arg %u)\n", res, results_acos[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = acosf(arguments_acos[i]);

		if (!cmp_float(res, results_acos[i])) {
			TPRINTF("Single precision acos failed "
			    "(%f != %lf, arg %u)\n", res, results_acos[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = asin(arguments_asin[i]);

		if (!cmp_double(res, results_asin[i])) {
			TPRINTF("Double precision asin failed "
			    "(%lf != %lf, arg %u)\n", res, results_asin[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = asinf(arguments_asin[i]);

		if (!cmp_float(res, results_asin[i])) {
			TPRINTF("Single precision asin failed "
			    "(%f != %lf, arg %u)\n", res, results_asin[i], i);
			fail = true;
		}
	}
#endif
	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = atan(arguments_atan[i]);

		if (!cmp_double(res, results_atan[i])) {
			TPRINTF("Double precision atan failed "
			    "(%.12lf != %.12lf, arg %u)\n", res, results_atan[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = atanf(arguments_atan[i]);

		if (!cmp_float(res, results_atan[i])) {
			TPRINTF("Single precision atan failed "
			    "(%f != %lf, arg %u)\n", res, results_atan[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = ceil(arguments[i]);

		if (!cmp_double(res, results_ceil[i])) {
			TPRINTF("Double precision ceil failed "
			    "(%lf != %lf, arg %u)\n", res, results_ceil[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = ceilf(arguments[i]);

		if (!cmp_float(res, results_ceil[i])) {
			TPRINTF("Single precision ceil failed "
			    "(%f != %lf, arg %u)\n", res, results_ceil[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = cos(arguments[i]);

		if (!cmp_double(res, results_cos[i])) {
			TPRINTF("Double precision cos failed "
			    "(%lf != %lf, arg %u)\n", res, results_cos[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = cosf(arguments[i]);

		if (!cmp_float(res, results_cos[i])) {
			TPRINTF("Single precision cos failed "
			    "(%f != %lf, arg %u)\n", res, results_cos[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = cosh(arguments_exp[i]);

		if (!cmp_double(res, results_cosh[i])) {
			TPRINTF("Double precision cosh failed "
			    "(%lf != %lf, arg %u)\n", res, results_cosh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = coshf(arguments_exp[i]);

		if (!cmp_float(res, results_cosh[i])) {
			TPRINTF("Single precision cosh failed "
			    "(%f != %lf, arg %u)\n", res, results_cosh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = exp(arguments_exp[i]);

		if (!cmp_double(res, results_exp[i])) {
			TPRINTF("Double precision exp failed "
			    "(%lf != %lf, arg %u)\n", res, results_exp[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = expf(arguments_exp[i]);

		if (!cmp_float(res, results_exp[i])) {
			TPRINTF("Single precision exp failed "
			    "(%f != %lf, arg %u)\n", res, results_exp[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = fabs(arguments[i]);

		if (!cmp_double(res, results_fabs[i])) {
			TPRINTF("Double precision fabs failed "
			    "(%lf != %lf, arg %u)\n", res, results_fabs[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = fabsf(arguments[i]);

		if (!cmp_float(res, results_fabs[i])) {
			TPRINTF("Single precision fabs failed "
			    "(%f != %lf, arg %u)\n", res, results_fabs[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = floor(arguments[i]);

		if (!cmp_double(res, results_floor[i])) {
			TPRINTF("Double precision floor failed "
			    "(%lf != %lf, arg %u)\n", res, results_floor[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = floorf(arguments[i]);

		if (!cmp_float(res, results_floor[i])) {
			TPRINTF("Single precision floor failed "
			    "(%f != %lf, arg %u)\n", res, results_floor[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = log(arguments_log[i]);

		if (!cmp_double(res, results_log[i])) {
			TPRINTF("Double precision log failed "
			    "(%lf != %lf, arg %u)\n", res, results_log[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = logf(arguments_log[i]);

		if (!cmp_float(res, results_log[i])) {
			TPRINTF("Single precision log failed "
			    "(%f != %lf, arg %u)\n", res, results_log[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = log10(arguments_log[i]);

		if (!cmp_double(res, results_log10[i])) {
			TPRINTF("Double precision log10 failed "
			    "(%lf != %lf, arg %u)\n", res, results_log10[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = log10f(arguments_log[i]);

		if (!cmp_float(res, results_log10[i])) {
			TPRINTF("Single precision log10 failed "
			    "(%f != %lf, arg %u)\n", res, results_log10[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = sin(arguments[i]);

		if (!cmp_double(res, results_sin[i])) {
			TPRINTF("Double precision sin failed "
			    "(%lf != %lf, arg %u)\n", res, results_sin[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = sinf(arguments[i]);

		if (!cmp_float(res, results_sin[i])) {
			TPRINTF("Single precision sin failed "
			    "(%f != %lf, arg %u)\n", res, results_sin[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = sinh(arguments_exp[i]);

		if (!cmp_double(res, results_sinh[i])) {
			TPRINTF("Double precision sinh failed "
			    "(%lf != %lf, arg %u)\n", res, results_sinh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = sinhf(arguments_exp[i]);

		if (!cmp_float(res, results_sinh[i])) {
			TPRINTF("Single precision sinh failed "
			    "(%f != %lf, arg %u)\n", res, results_sinh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = sqrt(arguments_sqrt[i]);

		if (!cmp_double(res, results_sqrt[i])) {
			TPRINTF("Double precision sqrt failed "
			    "(%lf != %lf, arg %u)\n", res, results_sqrt[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = sqrtf(arguments_sqrt[i]);

		if (!cmp_float(res, results_sqrt[i])) {
			TPRINTF("Single precision sqrt failed "
			    "(%f != %lf, arg %u)\n", res, results_sqrt[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = tan(arguments[i]);

		if (!cmp_double(res, results_tan[i])) {
			TPRINTF("Double precision tan failed "
			    "(%lf != %lf, arg %u)\n", res, results_tan[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = tanf(arguments[i]);

		if (!cmp_float(res, results_tan[i])) {
			TPRINTF("Single precision tan failed "
			    "(%f != %lf, arg %u)\n", res, results_tan[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = tanh(arguments_tanh[i]);

		if (!cmp_double(res, results_tanh[i])) {
			TPRINTF("Double precision tanh failed "
			    "(%lf != %lf, arg %u)\n", res, results_tanh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = tanhf(arguments_tanh[i]);

		if (!cmp_float(res, results_tanh[i])) {
			TPRINTF("Single precision tanh failed "
			    "(%f != %lf, arg %u)\n", res, results_tanh[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		double res = trunc(arguments[i]);

		if (!cmp_double(res, results_trunc[i])) {
			TPRINTF("Double precision trunc failed "
			    "(%lf != %lf, arg %u)\n", res, results_trunc[i], i);
			fail = true;
		}
	}

	for (unsigned int i = 0; i < OPERANDS; i++) {
		float res = truncf(arguments[i]);

		if (!cmp_float(res, results_trunc[i])) {
			TPRINTF("Single precision trunc failed "
			    "(%f != %lf, arg %u)\n", res, results_trunc[i], i);
			fail = true;
		}
	}

	if (fail)
		return "Floating point imprecision";

	return NULL;
}
