/*
 * Copyright (c) 2005 Josef Cejka
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup softfloat generic
 * @ingroup sfl
 * @brief Architecture independent parts of FPU software emulation library.
 * @{
 */
/** @file Softfloat API.
 */

#include "softfloat.h"
#include "sftypes.h"
#include "add.h"
#include "sub.h"
#include "mul.h"
#include "div.h"
#include "conversion.h"
#include "comparison.h"

/* Arithmetic functions */

float __addsf3(float a, float b)
{
	float_t fa;
	float_t fb;
	float_t res;
	
	fa.val = a;
	fb.val = b;
	
	if (fa.data.parts.sign != fb.data.parts.sign) {
		if (fa.data.parts.sign) {
			fa.data.parts.sign = 0;
			res.data = sub_float(fb.data, fa.data);
			
			return res.val;
		}
		
		fb.data.parts.sign = 0;
		res.data = sub_float(fa.data, fb.data);
		
		return res.val;
	}
	
	res.data = add_float(fa.data, fb.data);
	return res.val;
}

double __adddf3(double a, double b)
{
	double_t da;
	double_t db;
	double_t res;
	
	da.val = a;
	db.val = b;
	
	if (da.data.parts.sign != db.data.parts.sign) {
		if (da.data.parts.sign) {
			da.data.parts.sign = 0;
			res.data = sub_double(db.data, da.data);
			
			return res.val;
		}
		
		db.data.parts.sign = 0;
		res.data = sub_double(da.data, db.data);
		
		return res.val;
	}
	
	res.data = add_double(da.data, db.data);
	return res.val;
}

long double __addtf3(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	long_double_t res;
	
	ta.val = a;
	tb.val = b;
	
	if (ta.data.parts.sign != tb.data.parts.sign) {
		if (ta.data.parts.sign) {
			ta.data.parts.sign = 0;
			res.data = sub_long_double(tb.data, ta.data);
			
			return res.val;
		}
		
		tb.data.parts.sign = 0;
		res.data = sub_long_double(ta.data, tb.data);
		
		return res.val;
	}
	
	res.data = add_long_double(ta.data, tb.data);
	return res.val;
}

float __subsf3(float a, float b)
{
	float_t fa;
	float_t fb;
	float_t res;
	
	fa.val = a;
	fb.val = b;
	
	if (fa.data.parts.sign != fb.data.parts.sign) {
		fb.data.parts.sign = !fb.data.parts.sign;
		res.data = add_float(fa.data, fb.data);
		
		return res.val;
	}
	
	res.data = sub_float(fa.data, fb.data);
	return res.val;
}

double __subdf3(double a, double b)
{
	double_t da;
	double_t db;
	double_t res;
	
	da.val = a;
	db.val = b;
	
	if (da.data.parts.sign != db.data.parts.sign) {
		db.data.parts.sign = !db.data.parts.sign;
		res.data = add_double(da.data, db.data);
		
		return res.val;
	}
	
	res.data = sub_double(da.data, db.data);
	return res.val;
}

long double __subtf3(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	long_double_t res;
	
	ta.val = a;
	tb.val = b;
	
	if (ta.data.parts.sign != tb.data.parts.sign) {
		tb.data.parts.sign = !tb.data.parts.sign;
		res.data = add_long_double(ta.data, tb.data);
		
		return res.val;
	}
	
	res.data = sub_long_double(ta.data, tb.data);
	return res.val;
}

float __mulsf3(float a, float b)
{
	float_t fa;
	float_t fb;
	float_t res;
	
	fa.val = a;
	fb.val = b;
	
	res.data = mul_float(fa.data, fb.data);
	return res.val;
}

double __muldf3(double a, double b)
{
	double_t da;
	double_t db;
	double_t res;
	
	da.val = a;
	db.val = b;
	
	res.data = mul_double(da.data, db.data);
	return res.val;
}

long double __multf3(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	long_double_t res;
	
	ta.val = a;
	tb.val = b;
	
	res.data = mul_long_double(ta.data, tb.data);
	return res.val;
}

float __divsf3(float a, float b)
{
	float_t fa;
	float_t fb;
	float_t res;
	
	fa.val = a;
	fb.val = b;
	
	res.data = div_float(fa.data, fb.data);
	return res.val;
}

double __divdf3(double a, double b)
{
	double_t da;
	double_t db;
	double_t res;
	
	da.val = a;
	db.val = b;
	
	res.data = div_double(da.data, db.data);
	return res.val;
}

long double __divtf3(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	long_double_t res;
	
	ta.val = a;
	tb.val = b;
	
	res.data = div_long_double(ta.data, tb.data);
	return res.val;
}

float __negsf2(float a)
{
	float_t fa;
	
	fa.val = a;
	fa.data.parts.sign = !fa.data.parts.sign;
	
	return fa.val;
}

double __negdf2(double a)
{
	double_t da;
	
	da.val = a;
	da.data.parts.sign = !da.data.parts.sign;
	
	return da.val;
}

long double __negtf2(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	ta.data.parts.sign = !ta.data.parts.sign;
	
	return ta.val;
}

/* Conversion functions */

double __extendsfdf2(float a)
{
	float_t fa;
	double_t res;
	
	fa.val = a;
	res.data = float_to_double(fa.data);
	
	return res.val;
}

long double __extendsftf2(float a)
{
	float_t fa;
	long_double_t res;
	
	fa.val = a;
	res.data = float_to_long_double(fa.data);
	
	return res.val;
}

long double __extenddftf2(double a)
{
	double_t da;
	long_double_t res;
	
	da.val = a;
	res.data = double_to_long_double(da.data);
	
	return res.val;
}

float __truncdfsf2(double a)
{
	double_t da;
	float_t res;
	
	da.val = a;
	res.data = double_to_float(da.data);
	
	return res.val;
}

float __trunctfsf2(long double a)
{
	long_double_t ta;
	float_t res;
	
	ta.val = a;
	res.data = long_double_to_float(ta.data);
	
	return res.val;
}

double __trunctfdf2(long double a)
{
	long_double_t ta;
	double_t res;
	
	ta.val = a;
	res.data = long_double_to_double(ta.data);
	
	return res.val;
}

int __fixsfsi(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_int(fa.data);
}

int __fixdfsi(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_int(da.data);
}

int __fixtfsi(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_int(ta.data);
}
 
long __fixsfdi(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_long(fa.data);
}

long __fixdfdi(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_long(da.data);
}

long __fixtfdi(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_long(ta.data);
}
 
long long __fixsfti(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_llong(fa.data);
}

long long __fixdfti(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_llong(da.data);
}

long long __fixtfti(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_llong(ta.data);
}

unsigned int __fixunssfsi(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_uint(fa.data);
}

unsigned int __fixunsdfsi(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_uint(da.data);
}

unsigned int __fixunstfsi(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_uint(ta.data);
}
 
unsigned long __fixunssfdi(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_ulong(fa.data);
}

unsigned long __fixunsdfdi(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_ulong(da.data);
}

unsigned long __fixunstfdi(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_ulong(ta.data);
}
 
unsigned long long __fixunssfti(float a)
{
	float_t fa;
	
	fa.val = a;
	return float_to_ullong(fa.data);
}

unsigned long long __fixunsdfti(double a)
{
	double_t da;
	
	da.val = a;
	return double_to_ullong(da.data);
}

unsigned long long __fixunstfti(long double a)
{
	long_double_t ta;
	
	ta.val = a;
	return long_double_to_ullong(ta.data);
}
 
float __floatsisf(int i)
{
	float_t res;
	
	res.data = int_to_float(i);
	return res.val;
}

double __floatsidf(int i)
{
	double_t res;
	
	res.data = int_to_double(i);
	return res.val;
}

long double __floatsitf(int i)
{
	long_double_t res;
	
	res.data = int_to_long_double(i);
	return res.val;
}
 
float __floatdisf(long i)
{
	float_t res;
	
	res.data = long_to_float(i);
	return res.val;
}

double __floatdidf(long i)
{
	double_t res;
	
	res.data = long_to_double(i);
	return res.val;
}

long double __floatditf(long i)
{
	long_double_t res;
	
	res.data = long_to_long_double(i);
	return res.val;
}

float __floattisf(long long i)
{
	float_t res;
	
	res.data = llong_to_float(i);
	return res.val;
}

double __floattidf(long long i)
{
	double_t res;
	
	res.data = llong_to_double(i);
	return res.val;
}

long double __floattitf(long long i)
{
	long_double_t res;
	
	res.data = llong_to_long_double(i);
	return res.val;
}

float __floatunsisf(unsigned int i)
{
	float_t res;
	
	res.data = uint_to_float(i);
	return res.val;
}

double __floatunsidf(unsigned int i)
{
	double_t res;
	
	res.data = uint_to_double(i);
	return res.val;
}

long double __floatunsitf(unsigned int i)
{
	long_double_t res;
	
	res.data = uint_to_long_double(i);
	return res.val;
}
 
float __floatundisf(unsigned long i)
{
	float_t res;
	
	res.data = ulong_to_float(i);
	return res.val;
}

double __floatundidf(unsigned long i)
{
	double_t res;
	
	res.data = ulong_to_double(i);
	return res.val;
}

long double __floatunditf(unsigned long i)
{
	long_double_t res;
	
	res.data = ulong_to_long_double(i);
	return res.val;
}
 
float __floatuntisf(unsigned long long i)
{
	float_t res;
	
	res.data = ullong_to_float(i);
	return res.val;
}

double __floatuntidf(unsigned long long i)
{
	double_t res;
	
	res.data = ullong_to_double(i);
	return res.val;
}

long double __floatuntitf(unsigned long long i)
{
	long_double_t res;
	
	res.data = ullong_to_long_double(i);
	return res.val;
}

/* Comparison functions */

int __cmpsf2(float a, float b) 
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		/* no special constant for unordered - maybe signaled? */
		return 1;
	}
	
	if (is_float_eq(fa.data, fb.data))
		return 0;
	
	if (is_float_lt(fa.data, fb.data))
		return -1;
	
	return 1;
}

int __cmpdf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		/* no special constant for unordered - maybe signaled? */
		return 1;
	}
	
	if (is_double_eq(da.data, db.data))
		return 0;
	
	if (is_double_lt(da.data, db.data))
		return -1;
	
	return 1;
}

int __cmptf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		/* no special constant for unordered - maybe signaled? */
		return 1;
	}
	
	if (is_long_double_eq(ta.data, tb.data))
		return 0;
	
	if (is_long_double_lt(ta.data, tb.data))
		return -1;
	
	return 1;
}

int __unordsf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	return ((is_float_nan(fa.data)) || (is_float_nan(fb.data)));
}

int __unorddf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	return ((is_double_nan(da.data)) || (is_double_nan(db.data)));
}

int __unordtf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	return ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)));
}

int __eqsf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	return is_float_eq(fa.data, fb.data) - 1;
}

int __eqdf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	return is_double_eq(da.data, db.data) - 1;
}

int __eqtf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	return is_long_double_eq(ta.data, tb.data) - 1;
}

int __nesf2(float a, float b)
{
	/* strange behavior, but it was in gcc documentation */
	return __eqsf2(a, b);
}

int __nedf2(double a, double b)
{
	/* strange behavior, but it was in gcc documentation */
	return __eqdf2(a, b);
}

int __netf2(long double a, long double b)
{
	/* strange behavior, but it was in gcc documentation */
	return __eqtf2(a, b);
}

int __gesf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_float_eq(fa.data, fb.data))
		return 0;
	
	if (is_float_gt(fa.data, fb.data))
		return 1;
	
	return -1;
}

int __gedf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_double_eq(da.data, db.data))
		return 0;
	
	if (is_double_gt(da.data, db.data))
		return 1;
	
	return -1;
}

int __getf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_long_double_eq(ta.data, tb.data))
		return 0;
	
	if (is_long_double_gt(ta.data, tb.data))
		return 1;
	
	return -1;
}

int __ltsf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_float_lt(fa.data, fb.data))
		return -1;
	
	return 0;
}

int __ltdf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_double_lt(da.data, db.data))
		return -1;
	
	return 0;
}

int __lttf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_long_double_lt(ta.data, tb.data))
		return -1;
	
	return 0;
}

int __lesf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_float_eq(fa.data, fb.data))
		return 0;
	
	if (is_float_lt(fa.data, fb.data))
		return -1;
	
	return 1;
}

int __ledf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_double_eq(da.data, db.data))
		return 0;
	
	if (is_double_lt(da.data, db.data))
		return -1;
	
	return 1;
}

int __letf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		// TODO: sigNaNs
		return 1;
	}
	
	if (is_long_double_eq(ta.data, tb.data))
		return 0;
	
	if (is_long_double_lt(ta.data, tb.data))
		return -1;
	
	return 1;
}

int __gtsf2(float a, float b)
{
	float_t fa;
	float_t fb;
	
	fa.val = a;
	fb.val = b;
	
	if ((is_float_nan(fa.data)) || (is_float_nan(fb.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_float_gt(fa.data, fb.data))
		return 1;
	
	return 0;
}

int __gtdf2(double a, double b)
{
	double_t da;
	double_t db;
	
	da.val = a;
	db.val = b;
	
	if ((is_double_nan(da.data)) || (is_double_nan(db.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_double_gt(da.data, db.data))
		return 1;
	
	return 0;
}

int __gttf2(long double a, long double b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = a;
	tb.val = b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data))) {
		// TODO: sigNaNs
		return -1;
	}
	
	if (is_long_double_gt(ta.data, tb.data))
		return 1;
	
	return 0;
}

/* SPARC quadruple-precision wrappers */

void _Qp_add(long double *c, long double *a, long double *b)
{
	*c = __addtf3(*a, *b);
}

void _Qp_sub(long double *c, long double *a, long double *b)
{
	*c = __subtf3(*a, *b);
}

void _Qp_mul(long double *c, long double *a, long double *b)
{
	*c = __multf3(*a, *b);
}

void _Qp_div(long double *c, long double *a, long double *b)
{
	*c = __divtf3(*a, *b);
}

void _Qp_neg(long double *c, long double *a)
{
	*c = __negtf2(*a);
}

void _Qp_stoq(long double *c, float a)
{
	*c = __extendsftf2(a);
}

void _Qp_dtoq(long double *c, double a)
{
	*c = __extenddftf2(a);
}

float _Qp_qtos(long double *a)
{
	return __trunctfsf2(*a);
}

double _Qp_qtod(long double *a)
{
	return __trunctfdf2(*a);
}

int _Qp_qtoi(long double *a)
{
	return __fixtfsi(*a);
}

unsigned int _Qp_qtoui(long double *a)
{
	return __fixunstfsi(*a);
}

long _Qp_qtox(long double *a)
{
	return __fixtfdi(*a);
}

unsigned long _Qp_qtoux(long double *a)
{
	return __fixunstfdi(*a);
}

void _Qp_itoq(long double *c, int a)
{
	*c = __floatsitf(a);
}

void _Qp_uitoq(long double *c, unsigned int a)
{
	*c = __floatunsitf(a);
}

void _Qp_xtoq(long double *c, long a)
{
	*c = __floatditf(a);
}

void _Qp_uxtoq(long double *c, unsigned long a)
{
	*c = __floatunditf(a);
}

int _Qp_cmp(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 3;
	
	if (is_long_double_eq(ta.data, tb.data))
		return 0;
	
	if (is_long_double_lt(ta.data, tb.data))
		return 1;
	
	return 2;
}

int _Qp_cmpe(long double *a, long double *b)
{
	/* strange, but is defined this way in SPARC Compliance Definition */
	return _Qp_cmp(a, b);
}

int _Qp_feq(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return is_long_double_eq(ta.data, tb.data);
}

int _Qp_fge(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return is_long_double_eq(ta.data, tb.data) ||
	    is_long_double_gt(ta.data, tb.data);
}

int _Qp_fgt(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return is_long_double_gt(ta.data, tb.data);
}

int _Qp_fle(long double*a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return is_long_double_eq(ta.data, tb.data) ||
	    is_long_double_lt(ta.data, tb.data);
}

int _Qp_flt(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return is_long_double_lt(ta.data, tb.data);
}

int _Qp_fne(long double *a, long double *b)
{
	long_double_t ta;
	long_double_t tb;
	
	ta.val = *a;
	tb.val = *b;
	
	if ((is_long_double_nan(ta.data)) || (is_long_double_nan(tb.data)))
		return 0;
	
	return !is_long_double_eq(ta.data, tb.data);
}

float __aeabi_d2f(double a)
{
	return __truncdfsf2(a);
}

double __aeabi_f2d(float a)
{
	return __extendsfdf2(a);
}


float __aeabi_i2f(int i)
{
	return __floatsisf(i);
}

float __aeabi_ui2f(int i)
{
	return __floatunsisf(i);
}

double __aeabi_i2d(int i)
{
	return __floatsidf(i);
}

double __aeabi_ui2d(unsigned int i)
{
	return __floatunsidf(i);
}

double __aeabi_l2d(long long i)
{
	return __floattidf(i);
}

float __aeabi_l2f(long long i)
{
	return __floattisf(i);
}

float __aeabi_ul2f(unsigned long long u)
{
	return __floatuntisf(u);
}

int __aeabi_f2iz(float a)
{
	return __fixsfsi(a);
}

int __aeabi_f2uiz(float a)
{
	return __fixunssfsi(a);
}

int __aeabi_d2iz(double a)
{
	return __fixdfsi(a);
}

unsigned int __aeabi_d2uiz(double a)
{
	return __fixunsdfsi(a);
}

long long __aeabi_d2lz(double a)
{
	return __fixdfti(a);
}

int __aeabi_fcmpge(float a, float b)
{
	return __gesf2(a, b);
}

int __aeabi_fcmpgt(float a, float b)
{
	return __gtsf2(a, b);
}

int __aeabi_fcmplt(float a, float b)
{
	return __ltsf2(a, b);
}

int __aeabi_fcmpeq(float a, float b)
{
	return __eqsf2(a, b);
}

int __aeabi_dcmpge(double a, double b)
{
	return __gedf2(a, b);
}

int __aeabi_dcmpgt(double a, double b)
{
	return __gtdf2(a, b);
}

int __aeabi_dcmplt(double a, double b)
{
	return __ltdf2(a, b);
}

int __aeabi_dcmple(double a, double b)
{
	return __ledf2(a, b);
}

int __aeabi_dcmpeq(double a, double b)
{
	return __eqdf2(a, b);
}

float __aeabi_fadd(float a, float b)
{
	return __addsf3(a, b);
}

float __aeabi_fsub(float a, float b)
{
	return __subsf3(a, b);
}

float __aeabi_fmul(float a, float b)
{
	return __mulsf3(a, b);
}

float __aeabi_fdiv(float a, float b)
{
	return __divsf3(a, b);
}

double __aeabi_dadd(double a, double b)
{
	return __adddf3(a, b);
}

double __aeabi_dsub(double a, double b)
{
	return __subdf3(a, b);
}

double __aeabi_dmul(double a, double b)
{
	return __muldf3(a, b);
}

double __aeabi_ddiv(double a, double b)
{
	return __divdf3(a, b);
}

/** @}
 */
