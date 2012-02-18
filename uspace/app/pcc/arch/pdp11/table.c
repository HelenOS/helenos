/*	$Id: table.c,v 1.5 2008/10/19 15:25:25 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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

# define TLL TLONGLONG|TULONGLONG
# define ANYSIGNED TINT|TLONG|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TULONG|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED
# define TSWORD TINT
# define TWORD TUWORD|TSWORD
# define ANYSH	SCON|SAREG|SOREG|SNAME
# define ARONS	SAREG|SOREG|SNAME|STARNM

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	TWORD|TPOINT,
	SAREG,	TWORD|TPOINT,
		0,	RLEFT,
		"", },

/* convert char to int or unsigned */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TINT|TUNSIGNED,
		NAREG|NASL,	RESC1,
		"", }, /* chars are stored as ints in registers */

{ SCONV,	INAREG,
	SOREG|SCON|SNAME,	TCHAR,
	SAREG,	TINT,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\n", },

/* convert uchar to int or unsigned */
{ SCONV,	INAREG,
	SAREG|SOREG|SCON|SNAME,	TUCHAR,
	SAREG,	TINT|TUNSIGNED,
		NAREG,	RESC1,
		"clr	A1\nbisb	AL,A1\n", },

/* convert (u)int to (u)char.  Nothing to do. */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TCHAR|TUCHAR,
		0,	RLEFT,
		"", },

/* convert (u)int to (u)int */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* convert pointer to (u)int */
{ SCONV,	INAREG,
	SAREG,	TPOINT,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* convert int to long from memory */
{ SCONV,	INBREG,
	SNAME|SOREG,	TINT,
	SANY,	TLONG,
		NBREG,	RESC1,
		"mov	AL,U1\nsxt	A1\n", },

/* int -> (u)long. XXX - only in r0 and r1 */
{ SCONV,	INBREG,
	SAREG,	TINT,
	SANY,	TLONG|TULONG,
		NSPECIAL|NBREG|NBSL,	RESC1,
		"tst	AL\nsxt	r0\n", },

/* unsigned -> (u)long. XXX - only in r0 and r1 */
{ SCONV,	INBREG,
	SAREG,	TUNSIGNED,
	SANY,	TLONG|TULONG,
		NSPECIAL|NBREG|NBSL,	RESC1,
		"clr	r0\n", },

/* uint -> double */
{ SCONV,	INCREG,
	SAREG|SNAME|SOREG|SCON,	TUNSIGNED,
	SANY,			TFLOAT|TDOUBLE,
		NCREG|NCSL,	RESC1,
		"mov	AL,-(sp)\nclr	-(sp)\n"
		"setl\nmovif	(sp)+,A1\nseti\n", },

/* long -> int */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TWORD,
		NAREG|NASL,	RESC1,
		"mov	UL,A1\n", },


/* (u)long -> (u)long, nothing */
{ SCONV,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"", },

/* long -> double */
{ SCONV,	INCREG,
	SBREG|SNAME|SOREG|SCON,	TLONG,
	SANY,		TFLOAT|TDOUBLE,
		NCREG|NCSL,	RESC1,
		"mov	UL,-(sp)\nmov	AL,-(sp)\n"
		"setl\nmovif	(sp)+,A1\nseti\n", },

/*
 * Subroutine calls.
 */
{ CALL,		INBREG,
	SCON,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"jsr	pc,*CL\nZC", },

{ UCALL,	INBREG,
	SCON,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"jsr	pc,*CL\n", },

{ CALL,		FOREFF,
	SCON|SNAME|SOREG,	TANY,
	SANY,	TANY,
		0,	0,
		"jsr	pc,*AL\nZC", },

{ UCALL,	FOREFF,
	SCON|SNAME|SOREG,	TANY,
	SANY,	TANY,
		0,	0,
		"jsr	pc,*AL\n", },

{ CALL,		INAREG,
	SCON|SOREG|SNAME,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"jsr	pc,*AL\nZC", },

{ UCALL,	INAREG,
	SCON|SOREG|SNAME,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"jsr	pc,*AL\n", },

{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"jsr	pc,(AL)\nZC", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"jsr	pc,(AL)\n", },

{ CALL,		INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"jsr	pc,(AL)\nZC", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"jsr	pc,(AL)\n", },

/*
 * The next rules handle all binop-style operators.
 */
/* Add one to anything left but use only for side effects */
{ PLUS,		FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"inc	AL\n", },

/* add one for char to reg, special handling */
{ PLUS,		FOREFF|INAREG|FORCC,
	SAREG,	TCHAR|TUCHAR,
	SONE,		TANY,
		0,	RLEFT|RESCC,
		"inc	AL\n", },

/* add one for char to memory */
{ PLUS,		FOREFF|FORCC,
	SNAME|SOREG|STARNM,	TCHAR|TUCHAR,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"incb	AL\n", },

{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG,
	SBREG|SNAME|SOREG|SCON,	TLONG,
		0,	RLEFT,
		"add	AR,AL\nadd	UR,UL\nadc	AL\n", },

/* Add to reg left and reclaim reg */
{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"add	AR,AL\n", },

/* Add to anything left but use only for side effects */
{ PLUS,		FOREFF|FORCC,
	SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"add	AR,AL\n", },

{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG,			TCHAR|TUCHAR,
	SAREG|SNAME|SOREG|SCON,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"add	AR,AL\n", },

/* Post-increment read, byte */
{ MINUS,	INAREG,
	SINCB,	TCHAR|TUCHAR,
	SONE,	TANY,
		NAREG,	RESC1,
		"movb	ZG,A1\nincb	ZG\n", },

/* Post-increment read, int */
{ MINUS,	INAREG,
	SINCB,	TWORD|TPOINT,
	SONE,	TANY,
		NAREG,	RESC1,
		"mov	ZG,A1\ninc	ZG\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SBREG|SNAME|SOREG|SCON,	TLONG|TULONG,
		0,	RLEFT,
		"sub	AR,AL\nsub	UR,UL\nsbc	AL\n", },

/* Sub one from anything left */
{ MINUS,	FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"dec	AL\n", },

{ MINUS,		INAREG|FOREFF,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT,
		"sub	AR,AL\n", },

/* Sub from anything left but use only for side effects */
{ MINUS,	FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"sub	AR,AL\n", },

/* Sub one left but use only for side effects */
{ MINUS,	FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"decb	AL\n", },

/* Sub from anything left but use only for side effects */
{ MINUS,		FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG|SNAME|SOREG|SCON,	TCHAR|TUCHAR|TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"subb	AR,AL\n", },

/*
 * The next rules handle all shift operators.
 */
{ LS,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SANY,	TANY,
		0,	RLEFT,
		"ashc	AR,AL\n", },

{ LS,	INAREG|FOREFF,
	SAREG,	TWORD,
	SONE,	TANY,
		0,	RLEFT,
		"asl	AL\n", },

{ LS,	INAREG|FOREFF,
	SAREG,	TWORD,
	ANYSH,	TWORD,
		0,	RLEFT,
		"ash	AR,AL\n", },

/*
 * The next rules takes care of assignments. "=".
 */

/* First optimizations, in lack of weight it uses first found */
/* Start with class A registers */

/* Clear word at address */
{ ASSIGN,	FOREFF|FORCC,
	ARONS,	TWORD|TPOINT,
	SZERO,		TANY,
		0,	RESCC,
		"clr	AL\n", },

/* Clear word at reg */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TWORD|TPOINT,
	SZERO,		TANY,
		0,	RDEST,
		"clr	AL\n", },

/* Clear byte at address.  No reg here. */
{ ASSIGN,	FOREFF,
	SNAME|SOREG|STARNM,	TCHAR|TUCHAR,
	SZERO,		TANY,
		0,	RDEST,
		"clrb	AL\n", },

/* Clear byte in reg. must clear the whole register. */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TCHAR|TUCHAR,
	SZERO,	TANY,
		0,	RDEST,
		"clr	AL\n", },

/* The next is class B regs */

/* Clear long at address or in reg */
{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG|SBREG,	TLONG|TULONG,
	SZERO,			TANY,
		0,	RDEST,
		"clr	AL\nclr	UL\n", },

/* Save 2 bytes if high-order bits are zero */
{ ASSIGN,	FOREFF|INBREG,
	SBREG,	TLONG|TULONG,
	SSCON,	TLONG,
		0,	RDEST,
		"mov	UR,UL\nsxt	AL\n", },

/* Must have multiple rules for long otherwise regs may be trashed */
{ ASSIGN,	FOREFF|INBREG,
	SBREG,			TLONG|TULONG,
	SCON|SNAME|SOREG,	TLONG|TULONG,
		0,	RDEST,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG,	TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		0,	RDEST,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLONG|TULONG,
	SCON|SNAME|SOREG,	TLONG|TULONG,
		0,	0,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SBREG,	TLONG|TULONG,
		0,	RDEST,
		"ZE\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RDEST|RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	ARONS,	TWORD|TPOINT,
	SAREG,	TWORD|TPOINT,
		0,	RDEST|RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG,		TWORD|TPOINT,
	SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,		TCHAR|TUCHAR,
	ARONS|SCON,	TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	ARONS,	TCHAR|TUCHAR,
	SAREG,	TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG|STARNM,		TCHAR|TUCHAR,
	SNAME|SOREG|SCON|STARNM,	TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|INCREG,
	SCREG,		TDOUBLE,
	SNAME|SOREG|SCON,	TDOUBLE,
		0,	RDEST,
		"movf	AR,AL\n", },

{ ASSIGN,	FOREFF|INCREG,
	SCREG,		TFLOAT,
	SNAME|SOREG|SCON,	TFLOAT,
		0,	RDEST,
		"movof	AR,AL\n", },

{ ASSIGN,	FOREFF|INCREG,
	SNAME|SOREG|SCREG,	TDOUBLE,
	SCREG,			TDOUBLE,
		0,	RDEST,
		"movf	AR,AL\n", },

{ ASSIGN,	FOREFF|INCREG,
	SNAME|SOREG|SCREG,	TFLOAT,
	SCREG,			TFLOAT,
		0,	RDEST,
		"movfo	AR,AL\n", },

/*
 * DIV/MOD/MUL 
 */
/* XXX - mul may use any odd register, only r1 for now */
{ MUL,	INAREG,
	SAREG,				TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,		TWORD|TPOINT,
		NSPECIAL,	RLEFT,
		"mul	AR,AL\n", },

{ MUL,	INBREG,
	SBREG|SNAME|SCON|SOREG,		TLONG|TULONG,
	SBREG|SNAME|SCON|SOREG,		TLONG|TULONG,
		NSPECIAL|NBREG|NBSL|NBSR,		RESC1,
		"mov	UR,-(sp)\nmov	AR,-(sp)\n"
		"mov	UL,-(sp)\nmov	AL,-(sp)\n"
		"jsr	pc,lmul\nadd	$10,sp\n", },

{ MUL,	INCREG,
	SCREG,				TFLOAT|TDOUBLE,
	SCREG|SNAME|SOREG,		TFLOAT|TDOUBLE,
		0,	RLEFT,
		"mulf	AR,AL\n", },

/* need extra move to be sure N flag is correct for sxt */
{ DIV,	INAREG,
	ANYSH,		TINT|TPOINT,
	ANYSH,		TINT|TPOINT,
		NSPECIAL,	RDEST,
		"mov	AL,r1\nsxt	r0\ndiv	AR,r0\n", },

/* udiv uses args in registers */
{ DIV,	INAREG,
	SAREG,		TUNSIGNED,
	SAREG,		TUNSIGNED,
		NSPECIAL|NAREG|NASL|NASR,		RESC1,
		"jsr	pc,udiv\n", },

{ DIV,	INBREG,
	SBREG|SNAME|SCON|SOREG,		TLONG|TULONG,
	SBREG|SNAME|SCON|SOREG,		TLONG|TULONG,
		NSPECIAL|NBREG|NBSL|NBSR,		RESC1,
		"mov	UR,-(sp)\nmov	AR,-(sp)\n"
		"mov	UL,-(sp)\nmov	AL,-(sp)\n"
		"jsr	pc,ldiv\nadd	$10,sp\n", },

{ DIV,	INCREG,
	SCREG,			TFLOAT|TDOUBLE,
	SCREG|SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"divf	AR,AL\n", },

/* XXX merge the two below to one */
{ MOD,	INBREG,
	SBREG|SNAME|SCON|SOREG,		TLONG,
	SBREG|SNAME|SCON|SOREG,		TLONG,
		NSPECIAL|NBREG|NBSL|NBSR,		RESC1,
		"mov	UR,-(sp)\nmov	AR,-(sp)\n"
		"mov	UL,-(sp)\nmov	AL,-(sp)\n"
		"jsr	pc,lrem\nadd	$10,sp\n", },

{ MOD,	INBREG,
	SBREG|SNAME|SCON|SOREG,		TULONG,
	SBREG|SNAME|SCON|SOREG,		TULONG,
		NSPECIAL|NBREG|NBSL|NBSR,		RESC1,
		"mov	UR,-(sp)\nmov	AR,-(sp)\n"
		"mov	UL,-(sp)\nmov	AL,-(sp)\n"
		"jsr	pc,ulrem\nadd	$10,sp\n", },

/* urem uses args in registers */
{ MOD,	INAREG,
	SAREG,		TUNSIGNED,
	SAREG,		TUNSIGNED,
		NSPECIAL|NAREG|NASL|NASR,		RESC1,
		"jsr	pc,urem\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INBREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TLONG|TULONG,
		NBREG,	RESC1, /* |NBSL - may overwrite index reg */
		"mov	AR,A1\nmov	UR,U1\n", },

{ UMUL,	INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"mov	AR,A1\n", },

{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"movb	AR,A1\n", },

/*
 * Logical/branching operators
 */
{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TWORD|TPOINT,
	SZERO,	TANY,
		0, 	RESCC,
		"tst	AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
	SZERO,	TANY,
		0, 	RESCC,
		"tstb	AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TWORD|TPOINT,
	SAREG|SOREG|SNAME|SCON,	TWORD|TPOINT,
		0, 	RESCC,
		"cmp	AL,AR\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
		0, 	RESCC,
		"cmpb	AL,AR\n", },

{ OPLOG,	FORCC,
	SBREG|SOREG|SNAME|SCON,	TLONG|TULONG,
	SZERO,	TANY,
		0,	RNULL,
		"ZD", },

{ OPLOG,	FORCC,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RNULL,
		"ZF", },

/* AND/OR/ER/NOT */
/* Optimize if high order bits are zero */
{ AND,	FOREFF|INBREG|FORCC,
	SOREG|SNAME|SBREG,	TLONG|TULONG,
	SANDSCON,		TLONG|TULONG,
		0,	RLEFT|RESCC,
		"clr	AL\nbic	UR,UL\n", },

{ AND,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SCON|SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"bic	AR,AL\nbic	UR,UL\n", },

/* set status bits */
{ AND,	FORCC,
	ARONS|SCON,	TWORD|TPOINT,
	ARONS|SCON,	TWORD|TPOINT,
		0,	RESCC,
		"bit	AR,AL\n", },

/* AND with int */
{ AND,	INAREG|FORCC|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON|SAREG|SOREG|SNAME,	TWORD,
		0,	RLEFT|RESCC,
		"bic	AR,AL\n", },

/* AND with char */
{ AND,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	ARONS|SCON,		TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"bicb	AR,AL\n", },

{ OR,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SCON|SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"bis	AR,AL\nbis	UR,UL\n", },

/* OR with int */
{ OR,	FOREFF|INAREG|FORCC,
	ARONS,		TWORD,
	ARONS|SCON,	TWORD,
		0,	RLEFT|RESCC,
		"bis	AR,AL\n", },

/* OR with char */
{ OR,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	ARONS|SCON,		TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"bisb	AR,AL\n", },

/* XOR with int (extended insn)  */
{ ER,	INAREG|FORCC,
	ARONS,	TWORD,
	SAREG,	TWORD,
		0,	RLEFT|RESCC,
		"xor	AR,AL\n", },

/* XOR with char (extended insn)  */
{ ER,	INAREG|FORCC,
	SAREG,	TCHAR|TUCHAR,
	SAREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"xor	AR,AL\n", },

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"jbr	LL\n", },

/*
 * Convert LTYPE to reg.
 */
/* Two bytes less if high half of constant is zero */
{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SSCON,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	UL,U1\nsxt	A1\n", },

/* XXX - avoid OREG index register to be overwritten */
{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SCON|SBREG|SNAME|SOREG,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	AL,A1\nmov	UL,U1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TWORD|TPOINT,
		NAREG|NASR,	RESC1,
		"mov	AL,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TCHAR,
		NAREG,		RESC1,
		"movb	AR,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TUCHAR,
		NAREG,		RESC1,
		"clr	A1\nbisb	AL,A1\n", },

{ OPLTYPE,	INCREG,
	SANY,	TANY,
	SCREG|SCON|SOREG|SNAME,	TDOUBLE,
		NCREG,		RESC1,
		"movf	AL,A1\n", },

{ OPLTYPE,	INCREG,
	SANY,	TANY,
	SCREG|SCON|SOREG|SNAME,	TFLOAT,
		NCREG,		RESC1,
		"movof	AL,A1\n", },

/*
 * Negate a word.
 */
{ UMINUS,	INAREG|FOREFF,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
	SANY,	TANY,
		0,	RLEFT,
		"neg	AL\n", },

{ UMINUS,	INBREG|FOREFF,
	SBREG|SOREG|SNAME,	TLONG,
	SANY,			TANY,
		0,	RLEFT,
		"neg	AL\nneg	UL\nsbc	AL\n", },


{ COMPL,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TANY,
		0,	RLEFT,
		"com	AL\ncom	UL\n", },

{ COMPL,	INAREG,
	SAREG,	TWORD,
	SANY,	TANY,
		0,	RLEFT,
		"com	AL\n", },

/*
 * Arguments to functions.
 */
{ FUNARG,	FOREFF,
	SCON|SBREG|SNAME|SOREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		0,	RNULL,
		"mov	UL,ZA(sp)\nmov	AL,-(sp)\n", },

{ FUNARG,	FOREFF,
	SZERO,	TANY,
	SANY,	TANY,
		0,	RNULL,
		"clr	ZA(sp)\n", },

{ FUNARG,	FOREFF,
	SARGSUB,	TWORD|TPOINT,
	SANY,		TWORD|TPOINT,
		0,	RNULL,
		"ZB", },

{ FUNARG,	FOREFF,
	SARGINC,	TWORD|TPOINT,
	SANY,		TWORD|TPOINT,
		0,	RNULL,
		"ZH", },

{ FUNARG,	FOREFF,
	SCON|SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SANY,	TWORD|TPOINT,
		0,	RNULL,
		"mov	AL,ZA(sp)\n", },

{ FUNARG,	FOREFF,
	SCON,	TCHAR|TUCHAR,
	SANY,	TANY,
		0,	RNULL,
		"mov	AL,ZA(sp)\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TCHAR,
	SANY,		TCHAR,
		NAREG,	RNULL,
		"movb	AL,A1\nmov	A1,ZA(sp)\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TUCHAR,
	SANY,		TUCHAR,
		NAREG,	RNULL,
		"clr	ZA(sp)\nbisb	AL,(sp)\n", },

{ FUNARG,	FOREFF,
	SAREG,	TUCHAR|TCHAR,
	SANY,	TUCHAR|TCHAR,
		0,	RNULL,
		"mov	AL,ZA(sp)\n", },

{ FUNARG,	FOREFF,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,		TANY,
		0,	RNULL,
		"movf	AL,ZA(sp)\n", },

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
