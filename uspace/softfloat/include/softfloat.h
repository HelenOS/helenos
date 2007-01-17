/*
 * Copyright (c) 2005 Josef Cejka
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
/** @file
 */

#ifndef __SOFTFLOAT_H__
#define __SOFTFLOAT_H__

float __addsf3(float a, float b);
double __adddf3(double a, double b);
long double __addtf3(long double a, long double b);
long double __addxf3(long double a, long double b);
 
float __subsf3(float a, float b);
double __subdf3(double a, double b);
long double __subtf3(long double a, long double b);
long double __subxf3(long double a, long double b);
 
float __mulsf3(float a, float b);
double __muldf3(double a, double b);
long double __multf3(long double a, long double b);
long double __mulxf3(long double a, long double b);
 
float __divsf3(float a, float b);
double __divdf3(double a, double b);
long double __divtf3(long double a, long double b);
long double __divxf3(long double a, long double b);
 
float __negsf2(float a);
double __negdf2(double a);
long double __negtf2(long double a);
long double __negxf2(long double a);
 
double __extendsfdf2(float a);
long double __extendsftf2(float a);
long double __extendsfxf2(float a);
long double __extenddftf2(double a);
long double __extenddfxf2(double a);
 
double __truncxfdf2(long double a);
double __trunctfdf2(long double a);
float __truncxfsf2(long double a);
float __trunctfsf2(long double a);
float __truncdfsf2(double a);
 
int __fixsfsi(float a);
int __fixdfsi(double a);
int __fixtfsi(long double a);
int __fixxfsi(long double a);
 
long __fixsfdi(float a);
long __fixdfdi(double a);
long __fixtfdi(long double a);
long __fixxfdi(long double a);
 
long long __fixsfti(float a);
long long __fixdfti(double a);
long long __fixtfti(long double a);
long long __fixxfti(long double a);
 
unsigned int __fixunssfsi(float a);
unsigned int __fixunsdfsi(double a);
unsigned int __fixunstfsi(long double a);
unsigned int __fixunsxfsi(long double a);
 
unsigned long __fixunssfdi(float a);
unsigned long __fixunsdfdi(double a);
unsigned long __fixunstfdi(long double a);
unsigned long __fixunsxfdi(long double a);
 
unsigned long long __fixunssfti(float a);
unsigned long long __fixunsdfti(double a);
unsigned long long __fixunstfti(long double a);
unsigned long long __fixunsxfti(long double a);
 
float __floatsisf(int i);
double __floatsidf(int i);
long double __floatsitf(int i);
long double __floatsixf(int i);
 
float __floatdisf(long i);
double __floatdidf(long i);
long double __floatditf(long i);
long double __floatdixf(long i);
 
float __floattisf(long long i);
double __floattidf(long long i);
long double __floattitf(long long i);
long double __floattixf(long long i);
 
float __floatunsisf(unsigned int i);
double __floatunsidf(unsigned int i);
long double __floatunsitf(unsigned int i);
long double __floatunsixf(unsigned int i);
 
float __floatundisf(unsigned long i);
double __floatundidf(unsigned long i);
long double __floatunditf(unsigned long i);
long double __floatundixf(unsigned long i);
 
float __floatuntisf(unsigned long long i);
double __floatuntidf(unsigned long long i);
long double __floatuntitf(unsigned long long i);
long double __floatuntixf(unsigned long long i);
 
int __cmpsf2(float a, float b);
int __cmpdf2(double a, double b);
int __cmptf2(long double a, long double b);
 
int __unordsf2(float a, float b);
int __unorddf2(double a, double b);
int __unordtf2(long double a, long double b);
 
int __eqsf2(float a, float b);
int __eqdf2(double a, double b);
int __eqtf2(long double a, long double b);
 
int __nesf2(float a, float b);
int __nedf2(double a, double b);
int __netf2(long double a, long double b);
 
int __gesf2(float a, float b);
int __gedf2(double a, double b);
int __getf2(long double a, long double b);
 
int __ltsf2(float a, float b);
int __ltdf2(double a, double b);
int __lttf2(long double a, long double b);
int __lesf2(float a, float b);
int __ledf2(double a, double b);
int __letf2(long double a, long double b);
 
int __gtsf2(float a, float b);
int __gtdf2(double a, double b);
int __gttf2(long double a, long double b);
 
/* Not implemented yet*/ 
float __powisf2(float a, int b);

#endif


 /** @}
 */

