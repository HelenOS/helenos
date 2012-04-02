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

#include <softfloat.h>
#include <sftypes.h>

#include <add.h>
#include <sub.h>
#include <mul.h>
#include <div.h>

#include <conversion.h>
#include <comparison.h>
#include <other.h>

#include <functions.h>

/* Arithmetic functions */

float __addsf3(float a, float b)
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	if (fa.parts.sign != fb.parts.sign) {
		if (fa.parts.sign) {
			fa.parts.sign = 0;
			return subFloat32(fb, fa).f;
		};
		fb.parts.sign = 0;
		return subFloat32(fa, fb).f;
	}
	return addFloat32(fa, fb).f;
}

double __adddf3(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;
	if (da.parts.sign != db.parts.sign) {
		if (da.parts.sign) {
			da.parts.sign = 0;
			return subFloat64(db, da).d;
		};
		db.parts.sign = 0;
		return subFloat64(da, db).d;
	}
	return addFloat64(da, db).d;
}

long double __addtf3(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	if (ta.parts.sign != tb.parts.sign) {
		if (ta.parts.sign) {
			ta.parts.sign = 0;
			return subFloat128(tb, ta).ld;
		};
		tb.parts.sign = 0;
		return subFloat128(ta, tb).ld;
	}
	return addFloat128(ta, tb).ld;
}

float __subsf3(float a, float b)
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	if (fa.parts.sign != fb.parts.sign) {
		fb.parts.sign = !fb.parts.sign;
		return addFloat32(fa, fb).f;
	}
	return subFloat32(fa, fb).f;
}

double __subdf3(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;
	if (da.parts.sign != db.parts.sign) {
		db.parts.sign = !db.parts.sign;
		return addFloat64(da, db).d;
	}
	return subFloat64(da, db).d;
}

long double __subtf3(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	if (ta.parts.sign != tb.parts.sign) {
		tb.parts.sign = !tb.parts.sign;
		return addFloat128(ta, tb).ld;
	}
	return subFloat128(ta, tb).ld;
}

float __mulsf3(float a, float b) 
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	return 	mulFloat32(fa, fb).f;
}

double __muldf3(double a, double b) 
{
	float64 da, db;
	da.d = a;
	db.d = b;
	return 	mulFloat64(da, db).d;
}

long double __multf3(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	return 	mulFloat128(ta, tb).ld;
}

float __divsf3(float a, float b) 
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	return 	divFloat32(fa, fb).f;
}

double __divdf3(double a, double b) 
{
	float64 da, db;
	da.d = a;
	db.d = b;
	return 	divFloat64(da, db).d;
}

long double __divtf3(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	return 	divFloat128(ta, tb).ld;
}

float __negsf2(float a)
{
	float32 fa;
	fa.f = a;
	fa.parts.sign = !fa.parts.sign;
	return fa.f;
}

double __negdf2(double a)
{
	float64 da;
	da.d = a;
	da.parts.sign = !da.parts.sign;
	return da.d;
}

long double __negtf2(long double a)
{
	float128 ta;
	ta.ld = a;
	ta.parts.sign = !ta.parts.sign;
	return ta.ld;
}

/* Conversion functions */

double __extendsfdf2(float a) 
{
	float32 fa;
	fa.f = a;
	return convertFloat32ToFloat64(fa).d;
}

long double __extendsftf2(float a)
{
	float32 fa;
	fa.f = a;
	return convertFloat32ToFloat128(fa).ld;
}

long double __extenddftf2(double a)
{
	float64 da;
	da.d = a;
	return convertFloat64ToFloat128(da).ld;
}

float __truncdfsf2(double a) 
{
	float64 da;
	da.d = a;
	return convertFloat64ToFloat32(da).f;
}

float __trunctfsf2(long double a)
{
	float128 ta;
	ta.ld = a;
	return convertFloat128ToFloat32(ta).f;
}

double __trunctfdf2(long double a)
{
	float128 ta;
	ta.ld = a;
	return convertFloat128ToFloat64(ta).d;
}

int __fixsfsi(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_int(fa);
}

int __fixdfsi(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_int(da);
}

int __fixtfsi(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_int(ta);
}
 
long __fixsfdi(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_long(fa);
}

long __fixdfdi(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_long(da);
}

long __fixtfdi(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_long(ta);
}
 
long long __fixsfti(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_longlong(fa);
}

long long __fixdfti(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_longlong(da);
}

long long __fixtfti(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_longlong(ta);
}

unsigned int __fixunssfsi(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_uint(fa);
}

unsigned int __fixunsdfsi(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_uint(da);
}

unsigned int __fixunstfsi(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_uint(ta);
}
 
unsigned long __fixunssfdi(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_ulong(fa);
}

unsigned long __fixunsdfdi(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_ulong(da);
}

unsigned long __fixunstfdi(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_ulong(ta);
}
 
unsigned long long __fixunssfti(float a)
{
	float32 fa;
	fa.f = a;
	
	return float32_to_ulonglong(fa);
}

unsigned long long __fixunsdfti(double a)
{
	float64 da;
	da.d = a;
	
	return float64_to_ulonglong(da);
}

unsigned long long __fixunstfti(long double a)
{
	float128 ta;
	ta.ld = a;

	return float128_to_ulonglong(ta);
}
 
float __floatsisf(int i)
{
	float32 fa;
	
	fa = int_to_float32(i);
	return fa.f;
}

double __floatsidf(int i)
{
	float64 da;
	
	da = int_to_float64(i);
	return da.d;
}

long double __floatsitf(int i)
{
	float128 ta;

	ta = int_to_float128(i);
	return ta.ld;
}
 
float __floatdisf(long i)
{
	float32 fa;
	
	fa = long_to_float32(i);
	return fa.f;
}

double __floatdidf(long i)
{
	float64 da;
	
	da = long_to_float64(i);
	return da.d;
}

long double __floatditf(long i)
{
	float128 ta;

	ta = long_to_float128(i);
	return ta.ld;
}
 
float __floattisf(long long i)
{
	float32 fa;
	
	fa = longlong_to_float32(i);
	return fa.f;
}

double __floattidf(long long i)
{
	float64 da;
	
	da = longlong_to_float64(i);
	return da.d;
}

long double __floattitf(long long i)
{
	float128 ta;

	ta = longlong_to_float128(i);
	return ta.ld;
}

float __floatunsisf(unsigned int i)
{
	float32 fa;
	
	fa = uint_to_float32(i);
	return fa.f;
}

double __floatunsidf(unsigned int i)
{
	float64 da;
	
	da = uint_to_float64(i);
	return da.d;
}

long double __floatunsitf(unsigned int i)
{
	float128 ta;

	ta = uint_to_float128(i);
	return ta.ld;
}
 
float __floatundisf(unsigned long i)
{
	float32 fa;
	
	fa = ulong_to_float32(i);
	return fa.f;
}

double __floatundidf(unsigned long i)
{
	float64 da;
	
	da = ulong_to_float64(i);
	return da.d;
}

long double __floatunditf(unsigned long i)
{
	float128 ta;

	ta = ulong_to_float128(i);
	return ta.ld;
}
 
float __floatuntisf(unsigned long long i)
{
	float32 fa;
	
	fa = ulonglong_to_float32(i);
	return fa.f;
}

double __floatuntidf(unsigned long long i)
{
	float64 da;
	
	da = ulonglong_to_float64(i);
	return da.d;
}

long double __floatuntitf(unsigned long long i)
{
	float128 ta;

	ta = ulonglong_to_float128(i);
	return ta.ld;
}

/* Comparison functions */

int __cmpsf2(float a, float b) 
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;

	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		return 1; /* no special constant for unordered - maybe signaled? */
	}
	
	if (isFloat32eq(fa, fb)) {
		return 0;
	}
	
	if (isFloat32lt(fa, fb)) {
		return -1;
	}

	return 1;
}

int __cmpdf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;

	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		return 1; /* no special constant for unordered - maybe signaled? */
	}

	if (isFloat64eq(da, db)) {
		return 0;
	}

	if (isFloat64lt(da, db)) {
		return -1;
	}

	return 1;
}

int __cmptf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 1; /* no special constant for unordered - maybe signaled? */
	}

	if (isFloat128eq(ta, tb)) {
		return 0;
	}

	if (isFloat128lt(ta, tb)) {
		return -1;
	}

	return 1;
}

int __unordsf2(float a, float b) 
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	return ((isFloat32NaN(fa)) || (isFloat32NaN(fb)));
}

int __unorddf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;
	return ((isFloat64NaN(da)) || (isFloat64NaN(db)));
}

int __unordtf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	return ((isFloat128NaN(ta)) || (isFloat128NaN(tb)));
}

int __eqsf2(float a, float b) 
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;
	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		/* TODO: sigNaNs */
		return 1;
	}
	return isFloat32eq(fa, fb) - 1;
}

int __eqdf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;
	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		/* TODO: sigNaNs */
		return 1;
	}
	return isFloat64eq(da, db) - 1;
}

int __eqtf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;
	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		/* TODO: sigNaNs */
		return 1;
	}
	return isFloat128eq(ta, tb) - 1;
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
	float32 fa, fb;
	fa.f = a;
	fb.f = b;

	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		/* TODO: sigNaNs */
		return -1;
	}
	
	if (isFloat32eq(fa, fb)) {
		return 0;
	}
	
	if (isFloat32gt(fa, fb)) {
		return 1;
	}
	
	return -1;
}

int __gedf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;

	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		/* TODO: sigNaNs */
		return -1;
	}

	if (isFloat64eq(da, db)) {
		return 0;
	}

	if (isFloat64gt(da, db)) {
		return 1;
	}

	return -1;
}

int __getf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		/* TODO: sigNaNs */
		return -1;
	}

	if (isFloat128eq(ta, tb)) {
		return 0;
	}

	if (isFloat128gt(ta, tb)) {
		return 1;
	}

	return -1;
}

int __ltsf2(float a, float b)
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;

	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		/* TODO: sigNaNs */
		return 1;
	}

	if (isFloat32lt(fa, fb)) {
		return -1;
	}

	return 0;
}

int __ltdf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;

	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		/* TODO: sigNaNs */
		return 1;
	}

	if (isFloat64lt(da, db)) {
		return -1;
	}

	return 0;
}

int __lttf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		/* TODO: sigNaNs */
		return 1;
	}

	if (isFloat128lt(ta, tb)) {
		return -1;
	}

	return 0;
}

int __lesf2(float a, float b)
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;

	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		/* TODO: sigNaNs */
		return 1;
	}
	
	if (isFloat32eq(fa, fb)) {
		return 0;
	}
	
	if (isFloat32lt(fa, fb)) {
		return -1;
	}
	
	return 1;
}

int __ledf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;

	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		/* TODO: sigNaNs */
		return 1;
	}

	if (isFloat64eq(da, db)) {
		return 0;
	}

	if (isFloat64lt(da, db)) {
		return -1;
	}

	return 1;
}

int __letf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		/* TODO: sigNaNs */
		return 1;
	}

	if (isFloat128eq(ta, tb)) {
		return 0;
	}

	if (isFloat128lt(ta, tb)) {
		return -1;
	}

	return 1;
}

int __gtsf2(float a, float b)
{
	float32 fa, fb;
	fa.f = a;
	fb.f = b;

	if ((isFloat32NaN(fa)) || (isFloat32NaN(fb))) {
		/* TODO: sigNaNs */
		return -1;
	}

	if (isFloat32gt(fa, fb)) {
		return 1;
	}

	return 0;
}

int __gtdf2(double a, double b)
{
	float64 da, db;
	da.d = a;
	db.d = b;

	if ((isFloat64NaN(da)) || (isFloat64NaN(db))) {
		/* TODO: sigNaNs */
		return -1;
	}

	if (isFloat64gt(da, db)) {
		return 1;
	}

	return 0;
}

int __gttf2(long double a, long double b)
{
	float128 ta, tb;
	ta.ld = a;
	tb.ld = b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		/* TODO: sigNaNs */
		return -1;
	}

	if (isFloat128gt(ta, tb)) {
		return 1;
	}

	return 0;
}



#ifdef SPARC_SOFTFLOAT

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
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 3;
	}

	if (isFloat128eq(ta, tb)) {
		return 0;
	}

	if (isFloat128lt(ta, tb)) {
		return 1;
	}

	return 2;
}

int _Qp_cmpe(long double *a, long double *b)
{
	/* strange, but is defined this way in SPARC Compliance Definition */
	return _Qp_cmp(a, b);
}

int _Qp_feq(long double *a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 0;
	}

	return isFloat128eq(ta, tb);
}

int _Qp_fge(long double *a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 0;
	}

	return isFloat128eq(ta, tb) || isFloat128gt(ta, tb);
}

int _Qp_fgt(long double *a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 0;
	}

	return isFloat128gt(ta, tb);
}

int _Qp_fle(long double*a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 0;
	}

	return isFloat128eq(ta, tb) || isFloat128lt(ta, tb);
}

int _Qp_flt(long double *a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 0;
	}

	return isFloat128lt(ta, tb);
}

int _Qp_fne(long double *a, long double *b)
{
	float128 ta, tb;
	ta.ld = *a;
	tb.ld = *b;

	if ((isFloat128NaN(ta)) || (isFloat128NaN(tb))) {
		return 1;
	}

	return !isFloat128eq(ta, tb);
}

#endif

/** @}
 */
