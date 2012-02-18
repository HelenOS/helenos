/*	$Id: table.c,v 1.45 2011/02/18 17:08:31 ragge Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
 * Copyright (c) 2008 Anders Magnusson (ragge@ludd.ltu.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


# include "pass2.h"

#define TLL TLONG|TULONG
# define ANYSIGNED TINT|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED
# define TSWORD TINT
# define TWORD	TUWORD|TSWORD
#define	TANYINT	TLL|ANYFIXED
#define	 SHINT	SAREG	/* Any integer */
#define	 ININT	INAREG
#define	 SHFL	SCREG	/* shape for long double */
#define	 INFL	INCREG	/* shape for long double */

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	TLL|TPOINT,
	SAREG,	TLL|TPOINT,
		0,	RLEFT,
		"", },

{ PCONV,	INAREG,
	SAREG|SOREG|SNAME,	TUWORD,
	SAREG,	TPOINT,
		NASL|NAREG,	RESC1,
		"	movl AL,Z1\n", },/* amd64 zero-extends 32-bit movl */
	

/*
 * On amd64 casts from larger to smaller integer type in register do nothing.
 */
/* 64-bit to smaller */
{ SCONV,	INAREG,
	SAREG,	TLL|TPOINT,
	SAREG,	TANYINT,
		0,	RLEFT,
		"", },

/* 32-bit to smaller */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SAREG,	ANYFIXED,
		0,	RLEFT,
		"", },

/* 16-bit to smaller */
{ SCONV,	INAREG,
	SAREG,	TSHORT|TUSHORT,
	SAREG,	TUSHORT|TUCHAR|TSHORT|TCHAR,
		0,	RLEFT,
		"", },

/* 8-bit to 8-bit */
{ SCONV,	INAREG,
	SAREG,	TCHAR|TUCHAR,
	SAREG,	TUCHAR|TCHAR,
		0,	RLEFT,
		"", },

/*
 * Casts from memory to same or smaller register is equally simple.
 */
/* 64-bit to smaller */
{ SCONV,	INAREG,
	SNAME|SOREG,	TLL|TPOINT,
	SAREG,		TANYINT,
		NAREG,	RESC1,
		"	movZR AL,A1\n", },

/* 32-bit to smaller */
{ SCONV,	INAREG,
	SNAME|SOREG,	TWORD,
	SAREG,		ANYFIXED,
		NAREG,	RESC1,
		"	movZR AL,A1\n", },

/* 16-bit to smaller */
{ SCONV,	INAREG,
	SNAME|SOREG,	TSHORT|TUSHORT,
	SAREG,		TUSHORT|TUCHAR|TSHORT|TCHAR,
		NAREG,	RESC1,
		"	movZR AL,A1\n", },

/* 8-bit to 8-bit */
{ SCONV,	INAREG,
	SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG,		TUCHAR|TCHAR,
		NAREG,	RESC1,
		"	movZR AL,A1\n", },


/* char to something */

/* convert char to (unsigned) short. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TCHAR,
	SAREG,	TSHORT|TUSHORT,
		NASL|NAREG,	RESC1,
		"	movsbw AL,A1\n", },

/* convert unsigned char to (u)short. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TUCHAR,
	SAREG,	TSHORT|TUSHORT,
		NASL|NAREG,	RESC1,
		"	movzbw AL,A1\n", },

/* convert signed char to int (or pointer). */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TCHAR,
	SAREG,	TWORD|TPOINT,
		NASL|NAREG,	RESC1,
		"	movsbl AL,A1\n", },

/* convert unsigned char to (u)int. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TUCHAR,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzbl AL,A1\n", },

/* convert char to (u)long long */
{ SCONV,	INAREG,
	SAREG|SOREG|SNAME,	TCHAR,
	SANY,	TLL,
		NAREG|NASL,	RESC1,
		"	movsbq AL,A1\n", },

/* convert unsigned char to (u)long long */
{ SCONV,	INAREG,
	SAREG|SOREG|SNAME,	TUCHAR,
	SANY,			TLL,
		NAREG|NASL,	RESC1,
		"	movzbq AL,A1\n", },

/* short to something */

/* convert short to (u)int. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TSHORT,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movswl AL,A1\n", },

/* convert unsigned short to (u)int. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TUSHORT,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzwl AL,A1\n", },

/* convert short to (u)long long */
{ SCONV,	INAREG,
	SAREG|SOREG|SNAME,	TSHORT,
	SAREG,			TLL,
		NAREG|NASL,	RESC1,
		"	movswq AL,A1\n", },

/* convert unsigned short to (u)long long */
{ SCONV,	INAREG,
	SAREG|SOREG|SNAME,	TUSHORT,
	SAREG,			TLL,
		NAREG|NASL,	RESC1,
		"	movzwq AL,A1\n", },

/* int to something */

/* convert signed int to (u)long long */
{ SCONV,	INAREG,
	SAREG,	TSWORD,
	SAREG,	TLL,
		NASL|NAREG,	RESC1,
		"	movslq AL,A1\n", },

/* convert unsigned int to (u)long long */
{ SCONV,	INAREG,
	SAREG|SOREG|SNAME,	TUWORD,
	SAREG,	TLL,
		NASL|NAREG,	RESC1,
		"	movl AL,Z1\n", },/* amd64 zero-extends 32-bit movl */

/*
 * Floating point casts.  amd64 uses xmm for float/double and x87
 * for long double calculations.
 *
 * Types smaller than int are casted to int/(unsigned).
 */
/* no casts */
{ SCONV,	INBREG,
	SBREG,	TFLOAT,
	SBREG,	TFLOAT,
		0,	RLEFT,
		"", },

{ SCONV,	INBREG,
	SBREG,	TDOUBLE,
	SBREG,	TDOUBLE,
		0,	RLEFT,
		"", },

{ SCONV,	INCREG,
	SCREG,	TLDOUBLE,
	SCREG,	TLDOUBLE,
		0,	RLEFT,
		"", },


/* convert int/long to float/double */
{ SCONV,	INBREG,
	SAREG|SOREG|SNAME,	TINT|TLONG,
	SBREG,			TFLOAT|TDOUBLE,
		NBREG,	RESC1,
		"	cvtsi2sZfZq AL,A1\n", },

/* convert unsigned int to float/double */
{ SCONV,	INBREG,
	SAREG|SOREG|SNAME,	TUNSIGNED,
	SBREG,			TFLOAT|TDOUBLE,
		NAREG|NBREG,	RESC2,
		"	movl AL,Z1\n	cvtsi2sZfq A1,A2\n", },

/* convert unsigned long to float/double */
{ SCONV,	INBREG,
	SAREG|SOREG|SNAME,	TULONG,
	SBREG,			TFLOAT|TDOUBLE,
		NAREG*2|NASL|NBREG,	RESC3,
		"Zj", },

/* convert float/double to (u)char/(u)short/int */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TFLOAT|TDOUBLE,
	SAREG,			TCHAR|TUCHAR|TSHORT|TUSHORT|INT,
		NAREG,		RESC1,
		"	cvttsZg2si AL,A1\n", },

/* convert float/double to  unsigned int/long */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TFLOAT|TDOUBLE,
	SAREG,			TUNSIGNED|TLONG,
		NAREG,		RESC1,
		"	cvttsZg2siq AL,Z8\n", },

/* convert float to double */
{ SCONV,	INBREG,
	SBREG|SNAME|SOREG,	TFLOAT,
	SBREG,	TDOUBLE,
		NBREG|NBSL,	RESC1,
		"	cvtss2sd AL,A1\n", },

/* convert double to float */
{ SCONV,	INBREG,
	SBREG|SNAME|SOREG,	TDOUBLE,
	SBREG,	TFLOAT,
		NBREG|NBSL,	RESC1,
		"	cvtsd2ss AL,A1\n", },

/* x87 conversions */
/* float -> ldouble */
{ SCONV,	INCREG,
	SBREG,	TFLOAT,
	SCREG,	TLDOUBLE,
		NCREG,		RESC1,
		"\tsubq $4,%rsp\n\tmovss AL,(%rsp)\n"
		"\tflds (%rsp)\n\taddq $4,%rsp\n", },

/* double -> ldouble */
{ SCONV,	INCREG,
	SBREG,	TDOUBLE,
	SCREG,	TLDOUBLE,
		NCREG,		RESC1,
		"\tsubq $8,%rsp\n\tmovsd AL,(%rsp)\n"
		"\tfldl (%rsp)\n\taddq $8,%rsp\n", },

/* ldouble -> double */
{ SCONV,	INBREG,
	SCREG,	TLDOUBLE,
	SBREG,	TDOUBLE,
		NBREG,		RESC1,
		"\tsubq $8,%rsp\n\tfstpl (%rsp)\n"
		"\tmovsd (%rsp),A1\n\taddq $8,%rsp\n", },

/* ldouble -> float */
{ SCONV,	INBREG,
	SCREG,	TLDOUBLE,
	SBREG,	TFLOAT,
		NBREG,		RESC1,
		"\tsubq $4,%rsp\n\tfstps (%rsp)\n"
		"\tmovss (%rsp),A1\n\taddq $4,%rsp\n", },

/* convert int (in memory) to long double */
{ SCONV,	INCREG,
	SOREG|SNAME,	TSWORD,
	SCREG,	 TLDOUBLE,
		NCREG,	RESC1,
		"	fildl AL\n", },

/* convert unsigned int to long double */
{ SCONV,	INCREG,
	SAREG,	TUWORD,
	SCREG,	TLDOUBLE,
		NAREG|NASL|NCREG,	RESC2,
		"	subq $16,%rsp\n"
		"	movl AL,Z1\n"
		"	movq A1,(%rsp)\n"
		"	fildll (%rsp)\n"
		"	addq $16,%rsp\n", },

/* convert int (in register) to long double */
{ SCONV,	INCREG,
	SAREG,	TSWORD,
	SCREG,	TLDOUBLE,
		NCREG,	RESC1,
		"	subq $4,%rsp\n"
		"	movl AL,(%rsp)\n"
		"	fildl (%rsp)\n"
		"	addq $4,%rsp\n", },

/* unsigned long (in reg) to long double */
{ SCONV,	INCREG,
	SAREG,		TULONG,
	SCREG,		TLDOUBLE,
		NCREG,	RESC1,
		"	subq $16,%rsp\n"
		"	movq AL,(%rsp)\n"
		"	fildll (%rsp)\n"
		"	cmpq $0,AL\n"
		"	jns 1f\n"
		"	movl $1602224128,(%rsp)\n"
		"	fadds (%rsp)\n"
		"	addq $16,%rsp\n"
		"1:\n", },

/* unsigned long (in mem) to long double */
{ SCONV,	INCREG,
	SNAME|SOREG,	TULONG,
	SCREG,		TLDOUBLE,
		NCREG,	RESC1,
		"	fildll AL\n"
		"	cmpq $0,AL\n"
		"	jns 1f\n"
		"	push $1602224128\n"
		"	fadds (%rsp)\n"
		"	addq $8,%rsp\n"
		"1:\n", },

/* convert float/double to  unsigned long */
{ SCONV,	INAREG,
	SBREG,		TFLOAT|TDOUBLE,
	SAREG,		TULONG,
		(NAREG*2)|NBREG,	RESC1,
		"Zb\n", },

/* long double to unsigned long */
{ SCONV,	INAREG,
	SCREG|SNAME|SOREG,	TLDOUBLE,
	SAREG,			TULONG,
		NAREG,	RESC1,
		"ZB", },

/* ldouble -> long  XXX merge with int */
{ SCONV,	INAREG,
	SCREG,	TLDOUBLE,
	SAREG,	TLONG,
		NAREG,	RESC1,
		"	subq $16,%rsp\n"
		"	fnstcw (%rsp)\n"
		"	fnstcw 4(%rsp)\n"
		"	movb $12,1(%rsp)\n"
		"	fldcw (%rsp)\n"
		"	fistpll 8(%rsp)\n"
		"	movq 8(%rsp),A1\n"
		"	fldcw 4(%rsp)\n"
		"	addq $16,%rsp\n", },

/* ldouble -> (u)int */
{ SCONV,	INAREG,
	SCREG,	TLDOUBLE,
	SAREG,	TINT|TUNSIGNED,
		NAREG,	RESC1,
		"	subq $16,%rsp\n"
		"	fnstcw (%rsp)\n"
		"	fnstcw 4(%rsp)\n"
		"	movb $12,1(%rsp)\n"
		"	fldcw (%rsp)\n"
		"	fistpl 8(%rsp)\n"
		"	movl 8(%rsp),A1\n"
		"	fldcw 4(%rsp)\n"
		"	addq $16,%rsp\n", },

/* long (in mem) -> ldouble */
{ SCONV,	INCREG,
	SNAME|SOREG,	TLONG,
	SCREG,		TLDOUBLE,
		NCREG,	RESC1,
		"	fildll AL\n", },

/* long (in reg) -> ldouble */
{ SCONV,	INCREG,
	SAREG,		TLONG,
	SCREG,		TLDOUBLE,
		NCREG,	RESC1,
		"	subq $16,%rsp\n"
		"	movq AL,(%rsp)\n"
		"	fildll (%rsp)\n"
		"	addq $16,%rsp\n", },



/* slut sconv */

/*
 * Subroutine calls.
 */

{ CALL,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	call CL\nZC", },

{ UCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	call CL\n", },

{ CALL,	INAREG,
	SCON,	TANY,
	SAREG,	TLL|ANYFIXED|TPOINT,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INAREG,
	SCON,	TANY,
	SAREG,	TLL|ANYFIXED|TPOINT,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\n", },

{ CALL,	INBREG,
	SCON,	TANY,
	SBREG,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INBREG,
	SCON,	TANY,
	SBREG,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL, INCREG,
	SCON,	TANY,
	SCREG,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INCREG,
	SCON,	TANY,
	SCREG,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },


{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	call *AL\nZC", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	call *AL\nZC", },

{ CALL,		INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INBREG,
	SAREG,	TANY,
	SANY,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INBREG,
	SAREG,	TANY,
	SANY,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INCREG,
	SAREG,	TANY,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INCREG,
	SAREG,	TANY,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/* struct return */
{ USTCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	0,
		"ZP	call CL\nZC", },

{ USTCALL,	INAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"ZP	call CL\nZC", },

{ USTCALL,	INAREG,
	SNAME|SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"ZP	call *AL\nZC", },

{ STCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	0,
		"ZP	call CL\nZC", },

{ STCALL,	INAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"ZP	call CL\nZC", },

{ STCALL,	INAREG,
	SNAME|SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"ZP	call *AL\nZC", },

/*
 * The next rules handle all binop-style operators.
 */
/* floating point add */
{ PLUS,		INBREG,
	SBREG,			TFLOAT|TDOUBLE,
	SBREG|SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"	addsZf AR,AL\n", },

{ PLUS,		INCREG|FOREFF,
	SHFL,	TLDOUBLE,
	SHFL,	TLDOUBLE,
		0,	RLEFT,
		"	faddp\n", },


{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
	SONE,	TANY,
		0,	RLEFT,
		"	incq AL\n", },

{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SONE,	TANY,
		0,	RLEFT,
		"	incl AL\n", },

{ PLUS,		INAREG,
	SAREG,	TLL|TPOINT,
	SCON,	TWORD,
		NAREG|NASL,	RESC1,
		"	leaq CR(AL),A1\n", },

#ifdef notdef
/* older binutils are missing leal */
{ PLUS,		INAREG,
	SAREG,	TWORD,
	SCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leal CR(AL),A1\n", },
#endif

{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SONE,	TANY,
		0,	RLEFT,
		"	incw AL\n", },

{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,	TANY,
		0,	RLEFT,
		"	incb AL\n", },

#ifdef notdef
/* older binutils are missing leal */
{ PLUS,		INAREG,
	SAREG,	TWORD,
	SAREG,	TWORD,
		NAREG|NASL|NASR,	RESC1,
		"	leal (AL,AR),A1\n", },
#endif

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
	SONE,			TANY,
		0,	RLEFT,
		"	decq AL\n", },

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SONE,			TANY,
		0,	RLEFT,
		"	decl AL\n", },

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SONE,			TANY,
		0,	RLEFT,
		"	decw AL\n", },

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,	TANY,
		0,	RLEFT,
		"	decb AL\n", },

/* address as register offset, negative */
{ MINUS,	INAREG,
	SAREG,	TLL|TPOINT,
	SPCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leaq -CR(AL),A1\n", },

{ MINUS,	INBREG|FOREFF,
	SBREG,			TDOUBLE|TFLOAT,
	SBREG|SNAME|SOREG,	TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	subsZf AR,AL\n", },

{ MINUS,	INCREG|FOREFF,
	SHFL,	TLDOUBLE,
	SHFL,	TLDOUBLE,
		0,	RLEFT,
		"	fsubZAp\n", },

/* Simple r/m->reg ops */
/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
	SAREG,			TLL|TPOINT,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,			TLL|TPOINT,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD,
	SAREG,			TWORD,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,			TWORD,
	SAREG|SNAME|SOREG,	TWORD,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
	SHINT,		TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT,		TSHORT|TUSHORT,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,		TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,		TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/* m/r |= const */
#ifdef notdef	/* amd64 lacks immediate 64-bit simple ops */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },
#endif

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/*
 * The next rules handle all shift operators.
 */
/* r/m <<= r */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLL,
	SAREG,		TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	salq AR,AL\n", },

/* r/m <<= const */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLL,
	SCON,	TANY,
		0,	RLEFT,
		"	salq AR,AL\n", },

/* r/m <<= r */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SAREG,		TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sall AR,AL\n", },

/* r/m <<= const */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT,
		"	sall AR,AL\n", },

/* r/m <<= r */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shlw AR,AL\n", },

/* r/m <<= const */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	shlw AR,AL\n", },

{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	salb AR,AL\n", },

{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,			TANY,
		0,	RLEFT,
		"	salb AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLONG|TLONGLONG,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarq AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TLONG|TLONGLONG,
	SCON,			TANY,
		0,		RLEFT,
		"	sarq AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TULONG|TULONGLONG,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrq AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TULONG|TULONGLONG,
	SCON,			TANY,
		0,		RLEFT,
		"	shrq AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSWORD,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSWORD,
	SCON,			TANY,
		0,		RLEFT,
		"	sarl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUWORD,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUWORD,
	SCON,			TANY,
		0,		RLEFT,
		"	shrl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT,
	SCON,			TANY,
		0,		RLEFT,
		"	sarw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUSHORT,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUSHORT,
	SCON,			TANY,
		0,		RLEFT,
		"	shrw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarb AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TCHAR,
	SCON,			TANY,
		0,		RLEFT,
		"	sarb AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUCHAR,
	SAREG,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrb AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUCHAR,
	SCON,			TANY,
		0,		RLEFT,
		"	shrb AR,AL\n", },

/*
 * The next rules takes care of assignments. "=".
 */
{ ASSIGN,	FORCC|FOREFF|INAREG,
	SAREG,		TLL|TPOINT,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorq AL,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,		TLL|TPOINT,
	SCON,		TANY,
		0,	RDEST,
		"	movabs AR,AL\n", },

{ ASSIGN,	FORCC|FOREFF|INAREG,
	SAREG,		TWORD,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorl AL,AL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,		TANY,
		0,	0,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TWORD,
	SCON,		TANY,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FORCC|FOREFF|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorw AL,AL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,		TANY,
		0,	0,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SCON,		TANY,
		0,	RDEST,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,		TANY,
		0,	0,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,		TCHAR|TUCHAR,
	SCON,		TANY,
		0,	RDEST,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
	SAREG,			TLL|TPOINT,
		0,	RDEST,
		"	movq AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TWORD,
	SAREG,		TWORD,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,			TWORD,
	SAREG|SNAME|SOREG,	TWORD,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,			TPOINT,
	SAREG|SNAME|SOREG,	TPOINT,
		0,	RDEST,
		"	movq AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SAREG,		TSHORT|TUSHORT,
		0,	RDEST,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG,		TCHAR|TUCHAR|TWORD,
		0,	RDEST,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TCHAR|TUCHAR,
	SAREG|SCON,	TCHAR|TUCHAR,
		NAREG*2,	RDEST,
		"	movb AR,A2\n"
		"	movzbl A2,ZN\n"
		"	andl $N,AL\n"
		"	sall $H,ZN\n"
		"	andl $M,ZN\n"
		"	orl ZN,AL\n"
		"F	movb AR,AD\n"
		"FZE", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TSHORT|TUSHORT,
	SAREG|SCON,	TSHORT|TUSHORT,
		NAREG,	RDEST,
		"	movw AR,A1\n"
		"	movzwl A1,ZN\n"
		"	andl $N,AL\n"
		"	sall $H,ZN\n"
		"	andl $M,ZN\n"
		"	orl ZN,AL\n"
		"F	movw AR,AD\n"
		"FZE", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TWORD,
	SAREG|SNAME|SOREG|SCON,	TWORD,
		NAREG,	RDEST,
		"	movl AR,A1\n"
		"	andl $N,AL\n"
		"	sall $H,A1\n"
		"	andl $M,A1\n"
		"	orl A1,AL\n"
		"F	movl AR,AD\n"
		"FZE", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TLL,
	SAREG|SNAME|SOREG|SCON,	TLL,
		NAREG*2,	RDEST,
		"	movq AR,A1\n"
		"	movq $N,A2\n"
		"	andq A2,AL\n"
		"	salq $H,A1\n"
		"	movq $M,A2\n"
		"	andq A2,A1\n"
		"	orq A1,AL\n"
		"F	movq AR,AD\n"
		"FZE", },

{ ASSIGN,	INBREG|FOREFF,
	SBREG,			TFLOAT|TDOUBLE,
	SBREG|SOREG|SNAME,	TFLOAT|TDOUBLE,
		0,	RDEST,
		"	movsZf AR,AL\n", },

{ ASSIGN,	INBREG|FOREFF,
	SBREG|SOREG|SNAME,	TFLOAT|TDOUBLE,
	SBREG,			TFLOAT|TDOUBLE,
		0,	RDEST,
		"	movsZf AR,AL\n", },

/* x87 entries */
{ ASSIGN,	INDREG|FOREFF,
	SHFL,	TLDOUBLE,
	SHFL,	TLDOUBLE,
		0,	RDEST,
		"", }, /* This will always be in the correct register */

/* order of table entries is very important here! */
{ ASSIGN,	INFL,
	SNAME|SOREG,	TLDOUBLE,
	SHFL,	TLDOUBLE,
		0,	RDEST,
		"	fstpt AL\n	fldt AL\n", }, /* XXX */

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLDOUBLE,
	SHFL,	TLDOUBLE,
		0,	0,
		"	fstpt AL\n", },

/* end very important order */

{ ASSIGN,	INFL|FOREFF,
	SHFL,		TLDOUBLE,
	SHFL|SOREG|SNAME,	TLDOUBLE,
		0,	RDEST,
		"	fldt AR\n", },

/* end x87 */

/* Do not generate memcpy if return from funcall */
#if 0
{ STASG,	INAREG|FOREFF,
	SOREG|SNAME|SAREG,	TPTRTO|TSTRUCT,
	SFUNCALL,	TPTRTO|TSTRUCT,
		0,	RRIGHT,
		"", },
#endif

{ STASG,	INAREG|FOREFF,
	SOREG|SNAME,	TANY,
	SAREG,		TPTRTO|TANY,
		NSPECIAL,	RDEST,
		"ZQ", },

/*
 * DIV/MOD/MUL 
 */
{ DIV,	INAREG,
	SAREG,			TLONG,
	SAREG|SNAME|SOREG,	TLL,
		NSPECIAL,	RDEST,
		"	cqto\n	idivq AR\n", },

{ DIV,	INAREG,
	SAREG,			TULONG|TPOINT,
	SAREG|SNAME|SOREG,	TLL|TPOINT,
		NSPECIAL,	RDEST,
		"	xorq %rdx,%rdx\n	divq AR\n", },

{ DIV,	INAREG,
	SAREG,			TSWORD,
	SAREG|SNAME|SOREG,	TWORD,
		NSPECIAL,	RDEST,
		"	cltd\n	idivl AR\n", },

{ DIV,	INAREG,
	SAREG,			TUWORD,
	SAREG|SNAME|SOREG,	TWORD,
		NSPECIAL,	RDEST,
		"	xorl %edx,%edx\n	divl AR\n", },

{ DIV,	INAREG,
	SAREG,			TUSHORT,
	SAREG|SNAME|SOREG,	TUSHORT,
		NSPECIAL,	RDEST,
		"	xorl %edx,%edx\n	divw AR\n", },

{ DIV,	INAREG,
	SAREG,			TUCHAR,
	SAREG|SNAME|SOREG,	TUCHAR,
		NSPECIAL,	RDEST,
		"	xorb %ah,%ah\n	divb AR\n", },

{ DIV,	INBREG,
	SBREG,			TFLOAT|TDOUBLE,
	SBREG|SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"	divsZf AR,AL\n", },

{ DIV,	INCREG,
	SHFL,		TLDOUBLE,
	SHFL,		TLDOUBLE,
		0,	RLEFT,
		"	fdivZAp\n", },

{ MOD,	INAREG,
	SAREG,			TLONG,
	SAREG|SNAME|SOREG,	TLONG,
		NAREG|NSPECIAL,	RESC1,
		"	cqto\n	idivq AR\n", },

{ MOD,	INAREG,
	SAREG,			TLL|TPOINT,
	SAREG|SNAME|SOREG,	TULONG|TPOINT,
		NAREG|NSPECIAL,	RESC1,
		"	xorq %rdx,%rdx\n	divq AR\n", },

{ MOD,	INAREG,
	SAREG,			TSWORD,
	SAREG|SNAME|SOREG,	TSWORD,
		NAREG|NSPECIAL,	RESC1,
		"	cltd\n	idivl AR\n", },

{ MOD,	INAREG,
	SAREG,			TWORD,
	SAREG|SNAME|SOREG,	TUWORD,
		NAREG|NSPECIAL,	RESC1,
		"	xorl %edx,%edx\n	divl AR\n", },

{ MOD,	INAREG,
	SAREG,			TUSHORT,
	SAREG|SNAME|SOREG,	TUSHORT,
		NAREG|NSPECIAL,	RESC1,
		"	xorl %edx,%edx\n	divw AR\n", },

{ MOD,	INAREG,
	SAREG,			TUCHAR,
	SAREG|SNAME|SOREG,	TUCHAR,
		NAREG|NSPECIAL,	RESC1,
		"	xorb %ah,%ah\n	divb AR\n	movb %ah,%al\n", },

{ MUL,	INAREG,
	SAREG,				TLL|TPOINT,
	SAREG|SNAME|SOREG,		TLL|TPOINT,
		0,	RLEFT,
		"	imulq AR,AL\n", },

{ MUL,	INAREG,
	SAREG,				TWORD,
	SAREG|SNAME|SOREG|SCON,		TWORD,
		0,	RLEFT,
		"	imull AR,AL\n", },

{ MUL,	INAREG,
	SAREG,			TSHORT|TUSHORT,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	imulw AR,AL\n", },

{ MUL,	INAREG,
	SAREG,			TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	imulb AR\n", },

{ MUL,	INBREG,
	SBREG,			TFLOAT|TDOUBLE,
	SBREG|SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"	mulsZf AR,AL\n", },

{ MUL,	INCREG,
	SHFL,		TLDOUBLE,
	SHFL,		TLDOUBLE,
		0,	RLEFT,
		"	fmulp\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TLL|TPOINT,
		NAREG,	RESC1,
		"	movq AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TWORD,
	SOREG,	TWORD,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"	movb AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw AL,A1\n", },

{ UMUL,	INBREG,
	SANY,	TANY,
	SOREG,	TFLOAT|TDOUBLE,
		NBREG|NBSL,	RESC1,
		"	movsZf AL,A1\n", },

{ UMUL,	INCREG,
	SANY,	TANY,
	SOREG,	TLDOUBLE,
		NCREG|NCSL,	RESC1,
		"	fldt AL\n", },

/*
 * Logical/branching operators
 */

/* Comparisions, take care of everything */

{ OPLOG,	FORCC,
	SAREG,			TLL|TPOINT,
	SAREG|SOREG|SNAME,	TLL|TPOINT,
		0, 	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TLL|TPOINT,
	SAREG,			TLL|TPOINT,
		0,	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TLL|TPOINT,
	SCON32,			TANY,
		0,	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TWORD,
	SCON|SAREG,	TWORD,
		0, 	RESCC,
		"	cmpl AR,AL\n", },

{ OPLOG,	FORCC,
	SCON|SAREG,	TWORD,
	SAREG|SOREG|SNAME,	TWORD,
		0, 	RESCC,
		"	cmpl AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG,	TANY,
		0, 	RESCC,
		"	cmpw AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SAREG,	TANY,
		0, 	RESCC,
		"	cmpb AR,AL\n", },

{ OPLOG,	FORCC,
	SBREG,			TDOUBLE|TFLOAT,
	SBREG|SNAME|SOREG,	TDOUBLE|TFLOAT,
		0,	 	RNOP,
		"	ucomisZg AR,AL\nZU\n", },

/* x87 */
{ OPLOG,	FORCC,
	SCREG,	TLDOUBLE,
	SCREG,	TLDOUBLE,
		0,	RNOP,
		"ZG", },

{ OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	0,
		"diediedie!", },

/* AND/OR/ER/NOT */
{ AND,	INAREG|FOREFF,
	SAREG|SOREG|SNAME,	TLL,
	SCON,			TWORD,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG|SOREG|SNAME,	TLL,
	SAREG,			TLL,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG,			TLL,
	SAREG|SOREG|SNAME,	TLL,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG|SOREG|SNAME,	TWORD,
	SCON|SAREG,		TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG,			TWORD,
	SAREG|SOREG|SNAME,	TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INAREG|FOREFF,  
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw AR,AL\n", },

{ AND,	INAREG|FOREFF,  
	SAREG,			TSHORT|TUSHORT,
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SAREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG,			TCHAR|TUCHAR,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb AR,AL\n", },
/* AND/OR/ER/NOT */

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp LL\n", },

#if defined(GCC_COMPAT) || defined(LANG_F77)
{ GOTO, 	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp *AL\n", },
#endif

/*
 * Convert LTYPE to reg.
 */
{ OPLTYPE,	FORCC|INAREG,
	SAREG,	TLL|TPOINT,
	SMIXOR,	TANY,
		NAREG,	RESC1,
		"	xorq A1,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TLL|TPOINT,
		NAREG,	RESC1,
		"	movq AL,A1\n", },

{ OPLTYPE,	FORCC|INAREG,
	SAREG,	TWORD,
	SMIXOR,	TANY,
		NAREG|NASL,	RESC1,
		"	xorl A1,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TWORD,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
		NAREG,	RESC1,
		"	movb AL,A1\n", },

{ OPLTYPE,	FORCC|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SMIXOR,	TANY,
		NAREG,	RESC1,
		"	xorw A1,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME|SCON,	TSHORT|TUSHORT,
		NAREG,	RESC1,
		"	movw AL,A1\n", },

{ OPLTYPE,	INBREG,
	SANY,		TFLOAT|TDOUBLE,
	SOREG|SNAME|SBREG,	TFLOAT|TDOUBLE,
		NBREG,	RESC1,
		"	movsZf AL,A1\n", },

/* x87 entry */
{ OPLTYPE,	INCREG,
	SANY,		TLDOUBLE,
	SOREG|SNAME,	TLDOUBLE,
		NCREG,	RESC1,
		"	fldt AL\n", },

/*
 * Negate a word.
 */

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TLL|TPOINT,
	SAREG,	TLL|TPOINT,
		0,	RLEFT,
		"	negq AL\n", },

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TWORD,
	SAREG,	TWORD,
		0,	RLEFT,
		"	negl AL\n", },

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TSHORT|TUSHORT,
	SAREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	negw AL\n", },

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TCHAR|TUCHAR,
	SAREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	negb AL\n", },

{ UMINUS,	INBREG,
	SBREG,		TDOUBLE|TFLOAT,
	SBREG,		TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	xorpZf LC(%rip),AL\n", },

{ UMINUS,	INCREG,
	SCREG,	TLDOUBLE,
	SCREG,	TLDOUBLE,
		0,	RLEFT,
		"	fchs\n", },

{ COMPL,	INAREG,
	SAREG,	TLL,
	SANY,	TANY,
		0,	RLEFT,
		"	notq AL\n", },

{ COMPL,	INAREG,
	SAREG,	TWORD,
	SANY,	TANY,
		0,	RLEFT,
		"	notl AL\n", },

{ COMPL,	INAREG,
	SAREG,	TSHORT|TUSHORT,
	SANY,	TANY,
		0,	RLEFT,
		"	notw AL\n", },

{ COMPL,	INAREG,
	SAREG,	TCHAR|TUCHAR,
	SANY,	TANY,
		0,	RLEFT,
		"	notb AL\n", },

{ STARG,	FOREFF,
	SAREG|SOREG|SNAME|SCON, TANY,
	SANY,	TSTRUCT,
		NSPECIAL, 0,
		"ZF", },

{ ADDROF,	INAREG,
	SNAME,	TANY,
	SANY,	TANY,
		NAREG, RESC1,
		"	leaq AL,A1\n", },

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

/* { INIT, DF(INIT), }, */

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
