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

/** @addtogroup softfloat
 * @{
 */
/** @file Softfloat API.
 */

#ifndef __SOFTFLOAT_H__
#define __SOFTFLOAT_H__

extern float __addsf3(float, float);
extern double __adddf3(double, double);
extern long double __addtf3(long double, long double);
extern long double __addxf3(long double, long double);

extern float __subsf3(float, float);
extern double __subdf3(double, double);
extern long double __subtf3(long double, long double);
extern long double __subxf3(long double, long double);

extern float __mulsf3(float, float);
extern double __muldf3(double, double);
extern long double __multf3(long double, long double);
extern long double __mulxf3(long double, long double);

extern float __divsf3(float, float);
extern double __divdf3(double, double);
extern long double __divtf3(long double, long double);
extern long double __divxf3(long double, long double);

extern float __negsf2(float);
extern double __negdf2(double);
extern long double __negtf2(long double);
extern long double __negxf2(long double);

extern double __extendsfdf2(float);
extern long double __extendsftf2(float);
extern long double __extendsfxf2(float);
extern long double __extenddftf2(double);
extern long double __extenddfxf2(double);

extern double __truncxfdf2(long double);
extern double __trunctfdf2(long double);
extern float __truncxfsf2(long double);
extern float __trunctfsf2(long double);
extern float __truncdfsf2(double);

extern int __fixsfsi(float);
extern int __fixdfsi(double);
extern int __fixtfsi(long double);
extern int __fixxfsi(long double);

extern long __fixsfdi(float);
extern long __fixdfdi(double);
extern long __fixtfdi(long double);
extern long __fixxfdi(long double);

extern long long __fixsfti(float);
extern long long __fixdfti(double);
extern long long __fixtfti(long double);
extern long long __fixxfti(long double);

extern unsigned int __fixunssfsi(float);
extern unsigned int __fixunsdfsi(double);
extern unsigned int __fixunstfsi(long double);
extern unsigned int __fixunsxfsi(long double);

extern unsigned long __fixunssfdi(float);
extern unsigned long __fixunsdfdi(double);
extern unsigned long __fixunstfdi(long double);
extern unsigned long __fixunsxfdi(long double);

extern unsigned long long __fixunssfti(float);
extern unsigned long long __fixunsdfti(double);
extern unsigned long long __fixunstfti(long double);
extern unsigned long long __fixunsxfti(long double);

extern float __floatsisf(int);
extern double __floatsidf(int);
extern long double __floatsitf(int);
extern long double __floatsixf(int);

extern float __floatdisf(long);
extern double __floatdidf(long);
extern long double __floatditf(long);
extern long double __floatdixf(long);

extern float __floattisf(long long);
extern double __floattidf(long long);
extern long double __floattitf(long long);
extern long double __floattixf(long long);

extern float __floatunsisf(unsigned int);
extern double __floatunsidf(unsigned int);
extern long double __floatunsitf(unsigned int);
extern long double __floatunsixf(unsigned int);

extern float __floatundisf(unsigned long);
extern double __floatundidf(unsigned long);
extern long double __floatunditf(unsigned long);
extern long double __floatundixf(unsigned long);

extern float __floatuntisf(unsigned long long);
extern double __floatuntidf(unsigned long long);
extern long double __floatuntitf(unsigned long long);
extern long double __floatuntixf(unsigned long long);

extern int __cmpsf2(float, float);
extern int __cmpdf2(double, double);
extern int __cmptf2(long double, long double);

extern int __unordsf2(float, float);
extern int __unorddf2(double, double);
extern int __unordtf2(long double, long double);

extern int __eqsf2(float, float);
extern int __eqdf2(double, double);
extern int __eqtf2(long double, long double);

extern int __nesf2(float, float);
extern int __nedf2(double, double);
extern int __netf2(long double, long double);

extern int __gesf2(float, float);
extern int __gedf2(double, double);
extern int __getf2(long double, long double);

extern int __ltsf2(float, float);
extern int __ltdf2(double, double);
extern int __lttf2(long double, long double);

extern int __lesf2(float, float);
extern int __ledf2(double, double);
extern int __letf2(long double, long double);

extern int __gtsf2(float, float);
extern int __gtdf2(double, double);
extern int __gttf2(long double, long double);

/* Not implemented yet */
extern float __powisf2(float, int);
extern double __powidf2 (double, int);
extern long double __powitf2(long double, int);
extern long double __powixf2(long double, int);

/* SPARC quadruple-precision wrappers */
extern void _Qp_add(long double *, long double *, long double *);
extern void _Qp_sub(long double *, long double *, long double *);
extern void _Qp_mul(long double *, long double *, long double *);
extern void _Qp_div(long double *, long double *, long double *);
extern void _Qp_neg(long double *, long double *);

extern void _Qp_stoq(long double *, float);
extern void _Qp_dtoq(long double *, double);
extern float _Qp_qtos(long double *);
extern double _Qp_qtod(long double *);

extern int _Qp_qtoi(long double *);
extern unsigned int _Qp_qtoui(long double *);
extern long _Qp_qtox(long double *);
extern unsigned long _Qp_qtoux(long double *);

extern void _Qp_itoq(long double *, int);
extern void _Qp_uitoq(long double *, unsigned int);
extern void _Qp_xtoq(long double *, long);
extern void _Qp_uxtoq(long double *, unsigned long);

extern int _Qp_cmp(long double *, long double *);
extern int _Qp_cmpe(long double *, long double *);
extern int _Qp_feq(long double *, long double *);
extern int _Qp_fge(long double *, long double *);
extern int _Qp_fgt(long double *, long double *);
extern int _Qp_fle(long double*, long double *);
extern int _Qp_flt(long double *, long double *);
extern int _Qp_fne(long double *, long double *);

/* ARM EABI */
extern float __aeabi_d2f(double);
extern double __aeabi_f2d(float);
extern float __aeabi_i2f(int);
extern float __aeabi_ui2f(int);
extern double __aeabi_i2d(int);
extern double __aeabi_ui2d(unsigned int);
extern double __aeabi_l2d(long long);
extern float __aeabi_l2f(long long);
extern float __aeabi_ul2f(unsigned long long);
extern unsigned int __aeabi_d2uiz(double);
extern long long __aeabi_d2lz(double);

extern int __aeabi_f2iz(float);
extern int __aeabi_f2uiz(float);
extern int __aeabi_d2iz(double);

extern int __aeabi_fcmpge(float, float);
extern int __aeabi_fcmpgt(float, float);
extern int __aeabi_fcmplt(float, float);
extern int __aeabi_fcmpeq(float, float);

extern int __aeabi_dcmpge(double, double);
extern int __aeabi_dcmpgt(double, double);
extern int __aeabi_dcmplt(double, double);
extern int __aeabi_dcmple(double, double);
extern int __aeabi_dcmpeq(double, double);

extern float __aeabi_fadd(float, float);
extern float __aeabi_fsub(float, float);
extern float __aeabi_fmul(float, float);
extern float __aeabi_fdiv(float, float);

extern double __aeabi_dadd(double, double);
extern double __aeabi_dsub(double, double);
extern double __aeabi_dmul(double, double);
extern double __aeabi_ddiv(double, double);

/* Not implemented yet */
extern void _Qp_sqrt(long double *, long double *);

#endif

/** @}
 */
