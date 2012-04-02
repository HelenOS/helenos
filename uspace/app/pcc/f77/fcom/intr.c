/*	$Id: intr.c,v 1.13 2008/05/11 15:28:03 ragge Exp $	*/
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
 * notice, this list of conditions and the following disclaimer in the
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

#include "defines.h"
#include "defs.h"


static struct bigblock *finline(int, int, chainp);

union
	{
	int ijunk;
	struct intrpacked bits;
	} packed;

struct intrbits
	{
	int intrgroup /* :3 */;
	int intrstuff /* result type or number of generics */;
	int intrno /* :7 */;
	};

LOCAL struct intrblock
	{
	char intrfname[VL];
	struct intrbits intrval;
	} intrtab[ ] =
{
{ "int", 		{ INTRCONV, TYLONG }, },
{ "real", 	{ INTRCONV, TYREAL }, },
{ "dble", 	{ INTRCONV, TYDREAL }, },
{ "cmplx", 	{ INTRCONV, TYCOMPLEX }, },
{ "dcmplx", 	{ INTRCONV, TYDCOMPLEX }, },
{ "ifix", 	{ INTRCONV, TYLONG }, },
{ "idint", 	{ INTRCONV, TYLONG }, },
{ "float", 	{ INTRCONV, TYREAL }, },
{ "dfloat",	{ INTRCONV, TYDREAL }, },
{ "sngl", 	{ INTRCONV, TYREAL }, },
{ "ichar", 	{ INTRCONV, TYLONG }, },
{ "char", 	{ INTRCONV, TYCHAR }, },

{ "max", 		{ INTRMAX, TYUNKNOWN }, },
{ "max0", 	{ INTRMAX, TYLONG }, },
{ "amax0", 	{ INTRMAX, TYREAL }, },
{ "max1", 	{ INTRMAX, TYLONG }, },
{ "amax1", 	{ INTRMAX, TYREAL }, },
{ "dmax1", 	{ INTRMAX, TYDREAL }, },

{ "and",		{ INTRBOOL, TYUNKNOWN, OPBITAND }, },
{ "or",		{ INTRBOOL, TYUNKNOWN, OPBITOR }, },
{ "xor",		{ INTRBOOL, TYUNKNOWN, OPBITXOR }, },
{ "not",		{ INTRBOOL, TYUNKNOWN, OPBITNOT }, },
{ "lshift",	{ INTRBOOL, TYUNKNOWN, OPLSHIFT }, },
{ "rshift",	{ INTRBOOL, TYUNKNOWN, OPRSHIFT }, },

{ "min", 		{ INTRMIN, TYUNKNOWN }, },
{ "min0", 	{ INTRMIN, TYLONG }, },
{ "amin0", 	{ INTRMIN, TYREAL }, },
{ "min1", 	{ INTRMIN, TYLONG }, },
{ "amin1", 	{ INTRMIN, TYREAL }, },
{ "dmin1", 	{ INTRMIN, TYDREAL }, },

{ "aint", 	{ INTRGEN, 2, 0 }, },
{ "dint", 	{ INTRSPEC, TYDREAL, 1 }, },

{ "anint", 	{ INTRGEN, 2, 2 }, },
{ "dnint", 	{ INTRSPEC, TYDREAL, 3 }, },

{ "nint", 	{ INTRGEN, 4, 4 }, },
{ "idnint", 	{ INTRGEN, 2, 6 }, },

{ "abs", 		{ INTRGEN, 6, 8 }, },
{ "iabs", 	{ INTRGEN, 2, 9 }, },
{ "dabs", 	{ INTRSPEC, TYDREAL, 11 }, },
{ "cabs", 	{ INTRSPEC, TYREAL, 12 }, },
{ "zabs", 	{ INTRSPEC, TYDREAL, 13 }, },

{ "mod", 		{ INTRGEN, 4, 14 }, },
{ "amod", 	{ INTRSPEC, TYREAL, 16 }, },
{ "dmod", 	{ INTRSPEC, TYDREAL, 17 }, },

{ "sign", 	{ INTRGEN, 4, 18 }, },
{ "isign", 	{ INTRGEN, 2, 19 }, },
{ "dsign", 	{ INTRSPEC, TYDREAL, 21 }, },

{ "dim", 		{ INTRGEN, 4, 22 }, },
{ "idim", 	{ INTRGEN, 2, 23 }, },
{ "ddim", 	{ INTRSPEC, TYDREAL, 25 }, },

{ "dprod", 	{ INTRSPEC, TYDREAL, 26 }, },

{ "len", 		{ INTRSPEC, TYLONG, 27 }, },
{ "index", 	{ INTRSPEC, TYLONG, 29 }, },

{ "imag", 	{ INTRGEN, 2, 31 }, },
{ "aimag", 	{ INTRSPEC, TYREAL, 31 }, },
{ "dimag", 	{ INTRSPEC, TYDREAL, 32 }, },

{ "conjg", 	{ INTRGEN, 2, 33 }, },
{ "dconjg", 	{ INTRSPEC, TYDCOMPLEX, 34 }, },

{ "sqrt", 	{ INTRGEN, 4, 35 }, },
{ "dsqrt", 	{ INTRSPEC, TYDREAL, 36 }, },
{ "csqrt", 	{ INTRSPEC, TYCOMPLEX, 37 }, },
{ "zsqrt", 	{ INTRSPEC, TYDCOMPLEX, 38 }, },

{ "exp", 		{ INTRGEN, 4, 39 }, },
{ "dexp", 	{ INTRSPEC, TYDREAL, 40 }, },
{ "cexp", 	{ INTRSPEC, TYCOMPLEX, 41 }, },
{ "zexp", 	{ INTRSPEC, TYDCOMPLEX, 42 }, },

{ "log", 		{ INTRGEN, 4, 43 }, },
{ "alog", 	{ INTRSPEC, TYREAL, 43 }, },
{ "dlog", 	{ INTRSPEC, TYDREAL, 44 }, },
{ "clog", 	{ INTRSPEC, TYCOMPLEX, 45 }, },
{ "zlog", 	{ INTRSPEC, TYDCOMPLEX, 46 }, },

{ "log10", 	{ INTRGEN, 2, 47 }, },
{ "alog10", 	{ INTRSPEC, TYREAL, 47 }, },
{ "dlog10", 	{ INTRSPEC, TYDREAL, 48 }, },

{ "sin", 		{ INTRGEN, 4, 49 }, },
{ "dsin", 	{ INTRSPEC, TYDREAL, 50 }, },
{ "csin", 	{ INTRSPEC, TYCOMPLEX, 51 }, },
{ "zsin", 	{ INTRSPEC, TYDCOMPLEX, 52 }, },

{ "cos", 		{ INTRGEN, 4, 53 }, },
{ "dcos", 	{ INTRSPEC, TYDREAL, 54 }, },
{ "ccos", 	{ INTRSPEC, TYCOMPLEX, 55 }, },
{ "zcos", 	{ INTRSPEC, TYDCOMPLEX, 56 }, },

{ "tan", 		{ INTRGEN, 2, 57 }, },
{ "dtan", 	{ INTRSPEC, TYDREAL, 58 }, },

{ "asin", 	{ INTRGEN, 2, 59 }, },
{ "dasin", 	{ INTRSPEC, TYDREAL, 60 }, },

{ "acos", 	{ INTRGEN, 2, 61 }, },
{ "dacos", 	{ INTRSPEC, TYDREAL, 62 }, },

{ "atan", 	{ INTRGEN, 2, 63 }, },
{ "datan", 	{ INTRSPEC, TYDREAL, 64 }, },

{ "atan2", 	{ INTRGEN, 2, 65 }, },
{ "datan2", 	{ INTRSPEC, TYDREAL, 66 }, },

{ "sinh", 	{ INTRGEN, 2, 67 }, },
{ "dsinh", 	{ INTRSPEC, TYDREAL, 68 }, },

{ "cosh", 	{ INTRGEN, 2, 69 }, },
{ "dcosh", 	{ INTRSPEC, TYDREAL, 70 }, },

{ "tanh", 	{ INTRGEN, 2, 71 }, },
{ "dtanh", 	{ INTRSPEC, TYDREAL, 72 }, },

{ "lge",		{ INTRSPEC, TYLOGICAL, 73}, },
{ "lgt",		{ INTRSPEC, TYLOGICAL, 75}, },
{ "lle",		{ INTRSPEC, TYLOGICAL, 77}, },
{ "llt",		{ INTRSPEC, TYLOGICAL, 79}, },

{ "" }, };


LOCAL struct specblock
	{
	char atype;
	char rtype;
	char nargs;
	char spxname[XL];
	char othername;	/* index into callbyvalue table */
	} spectab[ ] =
{
	{ TYREAL,TYREAL,1,"r_int" },
	{ TYDREAL,TYDREAL,1,"d_int" },

	{ TYREAL,TYREAL,1,"r_nint" },
	{ TYDREAL,TYDREAL,1,"d_nint" },

	{ TYREAL,TYSHORT,1,"h_nint" },
	{ TYREAL,TYLONG,1,"i_nint" },

	{ TYDREAL,TYSHORT,1,"h_dnnt" },
	{ TYDREAL,TYLONG,1,"i_dnnt" },

	{ TYREAL,TYREAL,1,"r_abs" },
	{ TYSHORT,TYSHORT,1,"h_abs" },
	{ TYLONG,TYLONG,1,"i_abs" },
	{ TYDREAL,TYDREAL,1,"d_abs" },
	{ TYCOMPLEX,TYREAL,1,"c_abs" },
	{ TYDCOMPLEX,TYDREAL,1,"z_abs" },

	{ TYSHORT,TYSHORT,2,"h_mod" },
	{ TYLONG,TYLONG,2,"i_mod" },
	{ TYREAL,TYREAL,2,"r_mod" },
	{ TYDREAL,TYDREAL,2,"d_mod" },

	{ TYREAL,TYREAL,2,"r_sign" },
	{ TYSHORT,TYSHORT,2,"h_sign" },
	{ TYLONG,TYLONG,2,"i_sign" },
	{ TYDREAL,TYDREAL,2,"d_sign" },

	{ TYREAL,TYREAL,2,"r_dim" },
	{ TYSHORT,TYSHORT,2,"h_dim" },
	{ TYLONG,TYLONG,2,"i_dim" },
	{ TYDREAL,TYDREAL,2,"d_dim" },

	{ TYREAL,TYDREAL,2,"d_prod" },

	{ TYCHAR,TYSHORT,1,"h_len" },
	{ TYCHAR,TYLONG,1,"i_len" },

	{ TYCHAR,TYSHORT,2,"h_indx" },
	{ TYCHAR,TYLONG,2,"i_indx" },

	{ TYCOMPLEX,TYREAL,1,"r_imag" },
	{ TYDCOMPLEX,TYDREAL,1,"d_imag" },
	{ TYCOMPLEX,TYCOMPLEX,1,"r_cnjg" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"d_cnjg" },

	{ TYREAL,TYREAL,1,"r_sqrt", 1 },
	{ TYDREAL,TYDREAL,1,"d_sqrt", 1 },
	{ TYCOMPLEX,TYCOMPLEX,1,"c_sqrt" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_sqrt" },

	{ TYREAL,TYREAL,1,"r_exp", 2 },
	{ TYDREAL,TYDREAL,1,"d_exp", 2 },
	{ TYCOMPLEX,TYCOMPLEX,1,"c_exp" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_exp" },

	{ TYREAL,TYREAL,1,"r_log", 3 },
	{ TYDREAL,TYDREAL,1,"d_log", 3 },
	{ TYCOMPLEX,TYCOMPLEX,1,"c_log" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_log" },

	{ TYREAL,TYREAL,1,"r_lg10" },
	{ TYDREAL,TYDREAL,1,"d_lg10" },

	{ TYREAL,TYREAL,1,"r_sin", 4 },
	{ TYDREAL,TYDREAL,1,"d_sin", 4 },
	{ TYCOMPLEX,TYCOMPLEX,1,"c_sin" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_sin" },

	{ TYREAL,TYREAL,1,"r_cos", 5 },
	{ TYDREAL,TYDREAL,1,"d_cos", 5 },
	{ TYCOMPLEX,TYCOMPLEX,1,"c_cos" },
	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_cos" },

	{ TYREAL,TYREAL,1,"r_tan", 6 },
	{ TYDREAL,TYDREAL,1,"d_tan", 6 },

	{ TYREAL,TYREAL,1,"r_asin", 7 },
	{ TYDREAL,TYDREAL,1,"d_asin", 7 },

	{ TYREAL,TYREAL,1,"r_acos", 8 },
	{ TYDREAL,TYDREAL,1,"d_acos", 8 },

	{ TYREAL,TYREAL,1,"r_atan", 9 },
	{ TYDREAL,TYDREAL,1,"d_atan", 9 },

	{ TYREAL,TYREAL,2,"r_atn2", 10 },
	{ TYDREAL,TYDREAL,2,"d_atn2", 10 },

	{ TYREAL,TYREAL,1,"r_sinh", 11 },
	{ TYDREAL,TYDREAL,1,"d_sinh", 11 },

	{ TYREAL,TYREAL,1,"r_cosh", 12 },
	{ TYDREAL,TYDREAL,1,"d_cosh", 12 },

	{ TYREAL,TYREAL,1,"r_tanh", 13 },
	{ TYDREAL,TYDREAL,1,"d_tanh", 13 },

	{ TYCHAR,TYLOGICAL,2,"hl_ge" },
	{ TYCHAR,TYLOGICAL,2,"l_ge" },

	{ TYCHAR,TYLOGICAL,2,"hl_gt" },
	{ TYCHAR,TYLOGICAL,2,"l_gt" },

	{ TYCHAR,TYLOGICAL,2,"hl_le" },
	{ TYCHAR,TYLOGICAL,2,"l_le" },

	{ TYCHAR,TYLOGICAL,2,"hl_lt" },
	{ TYCHAR,TYLOGICAL,2,"l_lt" }
} ;






char callbyvalue[ ][XL] =
	{
	"sqrt",
	"exp",
	"log",
	"sin",
	"cos",
	"tan",
	"asin",
	"acos",
	"atan",
	"atan2",
	"sinh",
	"cosh",
	"tanh"
	};

struct bigblock *
intrcall(np, argsp, nargs)
struct bigblock *np;
struct bigblock *argsp;
int nargs;
{
int i, rettype;
struct bigblock *ap;
register struct specblock *sp;
struct bigblock *q;
register chainp cp;
bigptr ep;
int mtype;
int op;

packed.ijunk = np->b_name.vardesc.varno;
if(nargs == 0)
	goto badnargs;

mtype = 0;
for(cp = argsp->b_list.listp ; cp ; cp = cp->chain.nextp)
	{
/* TEMPORARY */ ep = cp->chain.datap;
/* TEMPORARY */	if( ISCONST(ep) && ep->vtype==TYSHORT )
/* TEMPORARY */		cp->chain.datap = mkconv(tyint, ep);
	mtype = maxtype(mtype, ep->vtype);
	}

switch(packed.bits.f1)
	{
	case INTRBOOL:
		op = packed.bits.f3;
		if( ! ONEOF(mtype, MSKINT|MSKLOGICAL) )
			goto badtype;
		if(op == OPBITNOT)
			{
			if(nargs != 1)
				goto badnargs;
			q = mkexpr(OPBITNOT, argsp->b_list.listp->chain.datap, NULL);
			}
		else
			{
			if(nargs != 2)
				goto badnargs;
			q = mkexpr(op, argsp->b_list.listp->chain.datap,
				argsp->b_list.listp->chain.nextp->chain.datap);
			}
		frchain( &(argsp->b_list.listp) );
		ckfree(argsp);
		return(q);

	case INTRCONV:
		rettype = packed.bits.f2;
		if(rettype == TYLONG)
			rettype = tyint;
		if( ISCOMPLEX(rettype) && nargs==2)
			{
			bigptr qr, qi;
			qr = argsp->b_list.listp->chain.datap;
			qi = argsp->b_list.listp->chain.nextp->chain.datap;
			if(ISCONST(qr) && ISCONST(qi))
				q = mkcxcon(qr,qi);
			else	q = mkexpr(OPCONV,mkconv(rettype-2,qr),
					mkconv(rettype-2,qi));
			}
		else if(nargs == 1)
			q = mkconv(rettype, argsp->b_list.listp->chain.datap);
		else goto badnargs;

		q->vtype = rettype;
		frchain(&(argsp->b_list.listp));
		ckfree(argsp);
		return(q);


	case INTRGEN:
		sp = spectab + packed.bits.f3;
		for(i=0; i<packed.bits.f2 ; ++i)
			if(sp->atype == mtype) {
				if (tyint == TYLONG &&
				    sp->rtype == TYSHORT && 
				    sp[1].atype == mtype)
					sp++; /* use long int */
				goto specfunct;
			} else
				++sp;
		goto badtype;

	case INTRSPEC:
		sp = spectab + packed.bits.f3;
		if(tyint==TYLONG && sp->rtype==TYSHORT)
			++sp;

	specfunct:
		if(nargs != sp->nargs)
			goto badnargs;
		if(mtype != sp->atype)
			goto badtype;
		fixargs(YES, argsp);
		if((q = finline(sp-spectab, mtype, argsp->b_list.listp)))
			{
			frchain( &(argsp->b_list.listp) );
			ckfree(argsp);
			}
		else if(sp->othername)
			{
			ap = builtin(sp->rtype,
				varstr(XL, callbyvalue[sp->othername-1]) );
			q = fixexpr( mkexpr(OPCCALL, ap, argsp) );
			}
		else
			{
			ap = builtin(sp->rtype, varstr(XL, sp->spxname) );
			q = fixexpr( mkexpr(OPCALL, ap, argsp) );
			}
		return(q);

	case INTRMIN:
	case INTRMAX:
		if(nargs < 2)
			goto badnargs;
		if( ! ONEOF(mtype, MSKINT|MSKREAL) )
			goto badtype;
		argsp->vtype = mtype;
		q = mkexpr( (packed.bits.f1==INTRMIN ? OPMIN : OPMAX), argsp, NULL);

		q->vtype = mtype;
		rettype = packed.bits.f2;
		if(rettype == TYLONG)
			rettype = tyint;
		else if(rettype == TYUNKNOWN)
			rettype = mtype;
		return( mkconv(rettype, q) );

	default:
		fatal1("intrcall: bad intrgroup %d", packed.bits.f1);
	}
badnargs:
	err1("bad number of arguments to intrinsic %s",
		varstr(VL,np->b_name.varname) );
	goto bad;

badtype:
	err1("bad argument type to intrinsic %s", varstr(VL, np->b_name.varname) );

bad:
	return( errnode() );
}



int
intrfunct(s)
char s[VL];
{
register struct intrblock *p;
char nm[VL];
register int i;

for(i = 0 ; i<VL ; ++s)
	nm[i++] = (*s==' ' ? '\0' : *s);

for(p = intrtab; p->intrval.intrgroup!=INTREND ; ++p)
	{
	if( eqn(VL, nm, p->intrfname) )
		{
		packed.bits.f1 = p->intrval.intrgroup;
		packed.bits.f2 = p->intrval.intrstuff;
		packed.bits.f3 = p->intrval.intrno;
		return(packed.ijunk);
		}
	}

return(0);
}





struct bigblock *
intraddr(np)
struct bigblock *np;
{
struct bigblock *q;
struct specblock *sp;

if(np->vclass!=CLPROC || np->b_name.vprocclass!=PINTRINSIC)
	fatal1("intraddr: %s is not intrinsic", varstr(VL,np->b_name.varname));
packed.ijunk = np->b_name.vardesc.varno;

switch(packed.bits.f1)
	{
	case INTRGEN:
		/* imag, log, and log10 arent specific functions */
		if(packed.bits.f3==31 || packed.bits.f3==43 || packed.bits.f3==47)
			goto bad;

	case INTRSPEC:
		sp = spectab + packed.bits.f3;
		if(tyint==TYLONG && sp->rtype==TYSHORT)
			++sp;
		q = builtin(sp->rtype, varstr(XL,sp->spxname) );
		return(q);

	case INTRCONV:
	case INTRMIN:
	case INTRMAX:
	case INTRBOOL:
	bad:
		err1("cannot pass %s as actual",
			varstr(VL,np->b_name.varname));
		return( errnode() );
	}
fatal1("intraddr: impossible f1=%d\n", packed.bits.f1);
/* NOTREACHED */
return 0; /* XXX gcc */
}




/*
 * Try to inline simple function calls.
 */
struct bigblock *
finline(int fno, int type, chainp args)
{
	register struct bigblock *q, *t;
	struct bigblock *x1;
	int l1;

	switch(fno) {
	case 8:	/* real abs */
	case 9:	/* short int abs */
	case 10:	/* long int abs */
	case 11:	/* double precision abs */
		t = fmktemp(type, NULL);
		putexpr(mkexpr(OPASSIGN, cpexpr(t), args->chain.datap));
		/* value now in t */

		/* if greater, jump to return */
		x1 = mkexpr(OPLE, cpexpr(t), mkconv(type,MKICON(0)));
		l1 = newlabel();
		putif(x1, l1);

		/* negate */
		putexpr(mkexpr(OPASSIGN, cpexpr(t),
		    mkexpr(OPNEG, cpexpr(t), NULL)));
		putlabel(l1);
		return(t);
		
	case 26:	/* dprod */
		q = mkexpr(OPSTAR, args->chain.datap, args->chain.nextp->chain.datap);
		q->vtype = TYDREAL;
		return(q);

	case 27:	/* len of character string */
		q = cpexpr(args->chain.datap->vleng);
		frexpr(args->chain.datap);
		return(q);

	case 14:	/* half-integer mod */
	case 15:	/* mod */
		return( mkexpr(OPMOD, args->chain.datap, args->chain.nextp->chain.datap) );
	}
return(NULL);
}
