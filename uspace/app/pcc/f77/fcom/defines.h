/*	$Id: defines.h,v 1.15 2008/05/10 07:53:41 ragge Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>

#ifdef FCOM
#include "pass2.h"
#endif
#include "ftypes.h"

#define INTERDATA 2
#define GCOS 3
#define PDP11 4
#define IBM 5
#define CMACH 6
#define VAX 7

#define DMR 2
#define SCJ 3

#define BINARY 2
#define ASCII 3

#define PREFIX 2
#define POSTFIX 3

#define M(x) (1<<x)
#define ALLOC(x) ckalloc(sizeof(struct x))
#define	BALLO()	(bigptr)ckalloc(sizeof(struct bigblock))
typedef void *ptr;
typedef FILE *FILEP;
typedef short flag;
typedef long int ftnint;
#define LOCAL static

#define NO 0
#define YES 1



/* block tag values */

#define TNAME 1
#define TCONST 2
#define TEXPR 3
#define TADDR 4
#define TPRIM 5
#define TLIST 6
#define TIMPLDO 7
#define TERROR 8


/* parser states */

#define OUTSIDE 0
#define INSIDE 1
#define INDCL 2
#define INDATA 3
#define INEXEC 4

/* procedure classes */

#define PROCMAIN 1
#define PROCBLOCK 2
#define PROCSUBR 3
#define PROCFUNCT 4


/* storage classes */

#define STGUNKNOWN 0
#define STGARG 1
#define STGAUTO 2
#define STGBSS 3
#define STGINIT 4
#define STGCONST 5
#define STGEXT 6
#define STGINTR 7
#define STGSTFUNCT 8
#define STGCOMMON 9
#define STGEQUIV 10
#define STGREG 11
#define STGLENG 12

/* name classes */

#define CLUNKNOWN 0
#define CLPARAM 1
#define CLVAR 2
#define CLENTRY 3
#define CLMAIN 4
#define CLBLOCK 5
#define CLPROC 6


/* vproclass values */

#define PUNKNOWN 0
#define PEXTERNAL 1
#define PINTRINSIC 2
#define PSTFUNCT 3
#define PTHISPROC 4

/* control stack codes */

#define CTLDO 1
#define CTLIF 2
#define CTLELSE 3


/* operators */

#define OPPLUS 1
#define OPMINUS 2
#define OPSTAR 3
#define OPSLASH 4
#define OPPOWER 5
#define OPNEG 6
#define OPOR 7
#define OPAND 8
#define OPEQV 9
#define OPNEQV 10
#define OPNOT 11
#define OPCONCAT 12
#define OPLT 13
#define OPEQ 14
#define OPGT 15
#define OPLE 16
#define OPNE 17
#define OPGE 18
#define OPCALL 19
#define OPCCALL 20
#define OPASSIGN 21
/* #define OPPLUSEQ 22 */
/* #define OPSTAREQ 23 */
#define OPCONV 24
#define OPLSHIFT 25
#define OPMOD 26
#define OPCOMMA 27
/* #define OPQUEST 28 */
/* #define OPCOLON 29 */
#define OPABS 30
#define OPMIN 31
#define OPMAX 32
#define OPADDR 33
#define OPINDIRECT 34
#define OPBITOR 35
#define OPBITAND 36
#define OPBITXOR 37
#define OPBITNOT 38
#define OPRSHIFT 39


/* memory regions */

#define REGARG 1
#define REGAUTO 2
#define REGBSS 3
#define REGINIT 4
#define REGCONST 5
#define REGEXT 6
#define REGPROG 7

/* label type codes */

#define LABUNKNOWN 0
#define LABEXEC 1
#define LABFORMAT 2
#define LABOTHER 3


/* INTRINSIC function codes*/

#define INTREND 0
#define INTRCONV 1
#define INTRMIN 2
#define INTRMAX 3
#define INTRGEN 4
#define INTRSPEC 5
#define INTRBOOL 6
#define INTRCNST 7


/* I/O statement codes */

#define IOSTDIN MKICON(5)
#define IOSTDOUT MKICON(6)

#define IOSBAD (-1)
#define IOSPOSITIONAL 0
#define IOSUNIT 1
#define IOSFMT 2

#define IOINQUIRE 1
#define IOOPEN 2
#define IOCLOSE 3
#define IOREWIND 4
#define IOBACKSPACE 5
#define IOENDFILE 6
#define IOREAD 7
#define IOWRITE 8


/* type masks */

#define MSKLOGICAL	M(TYLOGICAL)
#define MSKADDR	M(TYADDR)
#define MSKCHAR	M(TYCHAR)
#define MSKINT	M(TYSHORT)|M(TYLONG)
#define MSKREAL	M(TYREAL)|M(TYDREAL)
#define MSKCOMPLEX	M(TYCOMPLEX)|M(TYDCOMPLEX)

/* miscellaneous macros */

#define ONEOF(x,y) (M(x) & (y))
#define ISCOMPLEX(z) ONEOF(z, MSKCOMPLEX)
#define ISREAL(z) ONEOF(z, MSKREAL)
#define ISNUMERIC(z) ONEOF(z, MSKINT|MSKREAL|MSKCOMPLEX)
#define ISICON(z) (z->tag==TCONST && ISINT(z->vtype))
#define ISCHAR(z) (z->vtype==TYCHAR)
#define ISINT(z)   ONEOF(z, MSKINT)
#define ISCONST(z) (z->tag==TCONST)
#define ISERROR(z) (z->tag==TERROR)
#define ISPLUSOP(z) (z->tag==TEXPR && z->b_expr.opcode==OPPLUS)
#define ISSTAROP(z) (z->tag==TEXPR && z->b_expr.opcode==OPSTAR)
#define ISONE(z) (ISICON(z) && z->b_const.fconst.ci==1)
/* #define INT(z) ONEOF(z, MSKINT|MSKCHAR) */
#define MKICON(z) mkintcon( (ftnint)(z) )
#define CHCON(z) mkstrcon(strlen(z), z)

/* round a up to a multiple of b */
#define roundup(a,b)    ( b * ( (a+b-1)/b) )

/* prototypes for cpu-specific functions */
void prchars(int *);

