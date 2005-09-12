/*
 * Copyright (C) 2005 Josef Cejka
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

#ifndef __mips32_FMATH_H__
#define __mips32_FMATH_H__

#include <arch/types.h>
 
#define FMATH_EXPONENT_BIAS 1023

typedef unsigned char fmath_ld_descr_t[8];
 
typedef union { double bf; unsigned char ldd[8]; }  fmath_ld_union_t;

/**returns exponent in binary encoding*/
signed short fmath_get_binary_exponent(double num);

/**returns exponent in decimal encoding*/
double fmath_get_decimal_exponent(double num);

/**returns mantisa in binary encoding */
__u64 fmath_get_binary_mantisa(double num) ;

/** Function for extract integer part from double
* @param num input value
* @param intp integer part of num
* @return non-integer part
*/
double fmath_fint(double num, double *intp);

/** count base^exponent from positive exponent
* @param base
* @param exponent - Must be > 0.0 
* @return base^exponent or 0.0 (if exponent <=0.0)
*/
double fmath_dpow(double base, double exponent) ;

/** return 1, if num is NaN */
int fmath_is_nan(double num);

/** return 1, if fmath is a infinity */
int fmath_is_infinity(double num);

#endif
