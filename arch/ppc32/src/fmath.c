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

#include <arch/fmath.h>
#include <print.h>

	//TODO: 
#define FMATH_MANTISA_MASK ( 0x000fffffffffffffLL )

signed short fmath_get_binary_exponent(double num) 
{	//TODO:
/*	fmath_ld_union_t fmath_ld_union;
	fmath_ld_union.bf = num;
	return (signed short)((((fmath_ld_union.ldd[7])&0x7f)<<4) + (((fmath_ld_union.ldd[6])&0xf0)>>4)) -FMATH_EXPONENT_BIAS; // exponent is 11 bits lenght, so sevent bits is in 8th byte and 4 bits in 7th 
*/
	return 0;
}

double fmath_get_decimal_exponent(double num) 
{	//TODO:
	return 0;	
}

__u64 fmath_get_binary_mantisa(double num) 
{	//TODO:
/*	union { __u64 _u; double _d;} un = { _d : num };
	un._u=un._u &(FMATH_MANTISA_MASK); // mask 52 bits of mantisa
	return un._u;
	*/
	return 0;
}

double fmath_fint(double num, double *intp) 
{	//TODO:
/*	fmath_ld_union_t fmath_ld_union_num;
	fmath_ld_union_t fmath_ld_union_int;
	signed short exp;
	__u64 mask,mantisa;
	int i;
	
	exp=fmath_get_binary_exponent(num);
	
	if (exp<0) {
		*intp = 0.0;
		*intp = fmath_set_sign(0.0L,fmath_is_negative(num));
		return num;
		}
		

	if (exp>51) {
		*intp=num;
		num=0.0;
		num= fmath_set_sign(0.0L,fmath_is_negative(*intp));
		return num;
	}
	
	fmath_ld_union_num.bf = num;
	
	mask = FMATH_MANTISA_MASK>>exp;
	//mantisa = (fmath_get-binary_mantisa(num))&(~mask);
	
	for (i=0;i<7;i++) {
		// Ugly construction for obtain sign, exponent and integer part from num 
		fmath_ld_union_int.ldd[i]=fmath_ld_union_num.ldd[i]&(((~mask)>>(i*8))&0xff);
	}
	
	fmath_ld_union_int.ldd[6]|=((fmath_ld_union_num.ldd[6])&(0xf0));
	fmath_ld_union_int.ldd[7]=fmath_ld_union_num.ldd[7];
	
	*intp=fmath_ld_union_int.bf;
	return fmath_ld_union_num.bf-fmath_ld_union_int.bf;
*/
	
	return 0.0;
};
	

double fmath_dpow(double base, double exponent) 
{	//TODO:
/*	double value=1.0;
	if (base<=0.0) return base; 
	
	//2^(x*log2(10)) = 2^y = 10^x
	
	__asm__ __volatile__ (		\
		"fyl2x # ST(1):=ST(1)*log2(ST(0)), pop st(0) \n\t "		\
		"fld	%%st(0) \n\t"	\
		"frndint \n\t"		\
		"fxch %%st(1) \n\t"		\
		"fsub %%st(1),%%st(0) \n\t"	\
		"f2xm1	# ST := 2^ST -1\n\t"			\
		"fld1 \n\t"			\
		"faddp %%st(0),%%st(1) \n\t"	\
		"fscale #ST:=ST*2^(ST(1))\n\t"		\
		"fstp %%st(1) \n\t"		\
	"" : "=t" (value) :  "0" (base), "u" (exponent) );
	return value;
*/
	return 1.0;
}


int fmath_is_nan(double num) 
{
/*	__u16 exp;
	fmath_ld_union_t fmath_ld_union;
	fmath_ld_union.bf = num;
	exp=(((fmath_ld_union.ldd[7])&0x7f)<<4) + (((fmath_ld_union.ldd[6])&0xf0)>>4); // exponent is 11 bits lenght, so sevent bits is in 8th byte and 4 bits in 7th 

	if (exp!=0x07ff) return 0;
	if (fmath_get_binary_mantisa(num)>=FMATH_NAN) return 1;
	
*/		
	return 0;
}

int fmath_is_infinity(double num)
{
/*	__u16 exp;
	fmath_ld_union_t fmath_ld_union;
	fmath_ld_union.bf = num;
	exp=(((fmath_ld_union.ldd[7])&0x7f)<<4) + (((fmath_ld_union.ldd[6])&0xf0)>>4); // exponent is 11 bits lenght, so sevent bits is in 8th byte and 4 bits in 7th 

	if (exp!=0x07ff) return 0;
	if (fmath_get_binary_mantisa(num)==0x0) return 1;
*/	return 0;
}

