/*	$Id: exec.c,v 1.14 2008/05/11 15:28:03 ragge Exp $	*/
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
#include <string.h>

#include "defines.h"
#include "defs.h"

/*   Logical IF codes
*/
LOCAL void exar2(int, bigptr, int, int);
LOCAL void pushctl(int code);
LOCAL void popctl(void);
LOCAL void poplab(void);
LOCAL void mkstfunct(struct bigblock *, bigptr);

void
exif(p)
bigptr p;
{
pushctl(CTLIF);
ctlstack->elselabel = newlabel();
putif(p, ctlstack->elselabel);
}


void
exelif(p)
bigptr p;
{
if(ctlstack->ctltype == CTLIF)
	{
	if(ctlstack->endlabel == 0)
		ctlstack->endlabel = newlabel();
	putgoto(ctlstack->endlabel);
	putlabel(ctlstack->elselabel);
	ctlstack->elselabel = newlabel();
	putif(p, ctlstack->elselabel);
	}

else	execerr("elseif out of place", 0);
}




void
exelse()
{
if(ctlstack->ctltype==CTLIF)
	{
	if(ctlstack->endlabel == 0)
		ctlstack->endlabel = newlabel();
	putgoto( ctlstack->endlabel );
	putlabel(ctlstack->elselabel);
	ctlstack->ctltype = CTLELSE;
	}

else	execerr("else out of place", 0);
}

void
exendif()
{
if(ctlstack->ctltype == CTLIF)
	{
	putlabel(ctlstack->elselabel);
	if(ctlstack->endlabel)
		putlabel(ctlstack->endlabel);
	popctl();
	}
else if(ctlstack->ctltype == CTLELSE)
	{
	putlabel(ctlstack->endlabel);
	popctl();
	}

else	execerr("endif out of place", 0);
}



LOCAL void
pushctl(code)
int code;
{
register int i;

if(++ctlstack >= lastctl)
	fatal("nesting too deep");
ctlstack->ctltype = code;
for(i = 0 ; i < 4 ; ++i)
	ctlstack->ctlabels[i] = 0;
++blklevel;
}


LOCAL void
popctl()
{
if( ctlstack-- < ctls )
	fatal("control stack empty");
--blklevel;
poplab();
}



LOCAL void
poplab()
{
register struct labelblock  *lp;

for(lp = labeltab ; lp < highlabtab ; ++lp)
	if(lp->labdefined)
		{
		/* mark all labels in inner blocks unreachable */
		if(lp->blklevel > blklevel)
			lp->labinacc = YES;
		}
	else if(lp->blklevel > blklevel)
		{
		/* move all labels referred to in inner blocks out a level */
		lp->blklevel = blklevel;
		}
}



/*  BRANCHING CODE
*/
void
exgoto(lab)
struct labelblock *lab;
{
putgoto(lab->labelno);
}




/*
 * Found an assignment expression.
 */
void
exequals(struct bigblock *lp, bigptr rp)
{
	if(lp->tag != TPRIM) {
		err("assignment to a non-variable");
		frexpr(lp);
		frexpr(rp);
	} else if(lp->b_prim.namep->vclass!=CLVAR && lp->b_prim.argsp) {
		if(parstate >= INEXEC)
			err("statement function amid executables");
		else
			mkstfunct(lp, rp);
	} else {
		if(parstate < INDATA)
			enddcl();
		puteq(mklhs(lp), rp);
	}
}

/*
 * Create a statement function; e.g. like "f(i)=i*i"
 */
void
mkstfunct(struct bigblock *lp, bigptr rp)
{
	struct bigblock *p;
	struct bigblock *np;
	chainp args;

	np = lp->b_prim.namep;
	if(np->vclass == CLUNKNOWN)
		np->vclass = CLPROC;
	else {
		dclerr("redeclaration of statement function", np);
		return;
	}

	np->b_name.vprocclass = PSTFUNCT;
	np->vstg = STGSTFUNCT;
	impldcl(np);
	args = (lp->b_prim.argsp ? lp->b_prim.argsp->b_list.listp : NULL);
	np->b_name.vardesc.vstfdesc = mkchain((void *)args, (void *)rp);

	for( ; args ; args = args->chain.nextp)
		if( (p = args->chain.datap)->tag!=TPRIM ||
		    p->b_prim.argsp || p->b_prim.fcharp || p->b_prim.lcharp)
			err("non-variable argument in statement function definition");
		else {
			vardcl(args->chain.datap = p->b_prim.namep);
			ckfree(p);
		}
}


void
excall(name, args, nstars, labels)
struct bigblock *name;
struct bigblock *args;
int nstars;
struct labelblock *labels[ ];
{
register bigptr p;

settype(name, TYSUBR, 0);
p = mkfunct( mkprim(name, args, NULL, NULL) );
p->vtype = p->b_expr.leftp->vtype = TYINT;
if(nstars > 0)
	putcmgo(p, nstars, labels);
else putexpr(p);
}


void
exstop(stop, p)
int stop;
register bigptr p;
{
char *q;
int n;

if(p)
	{
	if( ! ISCONST(p) )
		{
		execerr("pause/stop argument must be constant", 0);
		frexpr(p);
		p = mkstrcon(0, 0);
		}
	else if( ISINT(p->vtype) )
		{
		q = convic(p->b_const.fconst.ci);
		n = strlen(q);
		if(n > 0)
			{
			p->b_const.fconst.ccp = copyn(n, q);
			p->vtype = TYCHAR;
			p->vleng = MKICON(n);
			}
		else
			p = mkstrcon(0, 0);
		}
	else if(p->vtype != TYCHAR)
		{
		execerr("pause/stop argument must be integer or string", 0);
		p = mkstrcon(0, 0);
		}
	}
else	p = mkstrcon(0, 0);

putexpr( call1(TYSUBR, (stop ? "s_stop" : "s_paus"), p) );
}

/* DO LOOP CODE */

#define DOINIT	par[0]
#define DOLIMIT	par[1]
#define DOINCR	par[2]

#define VARSTEP	0
#define POSSTEP	1
#define NEGSTEP	2

void
exdo(range, spec)
int range;
chainp spec;
{
register bigptr p, q;
bigptr q1;
register struct bigblock *np;
chainp cp;
register int i;
int dotype, incsign = 0; /* XXX gcc */
struct bigblock *dovarp, *dostgp;
bigptr par[3];

pushctl(CTLDO);
dorange = ctlstack->dolabel = range;
np = spec->chain.datap;
ctlstack->donamep = NULL;
if(np->b_name.vdovar)
	{
	err1("nested loops with variable %s", varstr(VL,np->b_name.varname));
	ctlstack->donamep = NULL;
	return;
	}

dovarp = mklhs( mkprim(np, 0,0,0) );
if( ! ONEOF(dovarp->vtype, MSKINT|MSKREAL) )
	{
	err("bad type on do variable");
	return;
	}
ctlstack->donamep = np;

np->b_name.vdovar = YES;
if( enregister(np) )
	{
	/* stgp points to a storage version, varp to a register version */
	dostgp = dovarp;
	dovarp = mklhs( mkprim(np, 0,0,0) );
	}
else
	dostgp = NULL;
dotype = dovarp->vtype;

for(i=0 , cp = spec->chain.nextp ; cp!=NULL && i<3 ; cp = cp->chain.nextp)
	{
	p = par[i++] = fixtype(cp->chain.datap);
	if( ! ONEOF(p->vtype, MSKINT|MSKREAL) )
		{
		err("bad type on DO parameter");
		return;
		}
	}

frchain(&spec);
switch(i)
	{
	case 0:
	case 1:
		err("too few DO parameters");
		return;

	default:
		err("too many DO parameters");
		return;

	case 2:
		DOINCR = MKICON(1);

	case 3:
		break;
	}

ctlstack->endlabel = newlabel();
ctlstack->dobodylabel = newlabel();

if( ISCONST(DOLIMIT) )
	ctlstack->domax = mkconv(dotype, DOLIMIT);
else
	ctlstack->domax = fmktemp(dotype, NULL);

if( ISCONST(DOINCR) )
	{
	ctlstack->dostep = mkconv(dotype, DOINCR);
	if( (incsign = conssgn(ctlstack->dostep)) == 0)
		err("zero DO increment");
	ctlstack->dostepsign = (incsign > 0 ? POSSTEP : NEGSTEP);
	}
else
	{
	ctlstack->dostep = fmktemp(dotype, NULL);
	ctlstack->dostepsign = VARSTEP;
	ctlstack->doposlabel = newlabel();
	ctlstack->doneglabel = newlabel();
	}

if( ISCONST(ctlstack->domax) && ISCONST(DOINIT) && ctlstack->dostepsign!=VARSTEP)
	{
	puteq(cpexpr(dovarp), cpexpr(DOINIT));
	if( onetripflag )
		frexpr(DOINIT);
	else
		{
		q = mkexpr(OPPLUS, MKICON(1),
			mkexpr(OPMINUS, cpexpr(ctlstack->domax), cpexpr(DOINIT)) );
		if(incsign != conssgn(q))
			{
			warn("DO range never executed");
			putgoto(ctlstack->endlabel);
			}
		frexpr(q);
		}
	}
else if(ctlstack->dostepsign!=VARSTEP && !onetripflag)
	{
	if( ISCONST(ctlstack->domax) )
		q = cpexpr(ctlstack->domax);
	else
		q = mkexpr(OPASSIGN, cpexpr(ctlstack->domax), DOLIMIT);

	q1 = mkexpr(OPASSIGN, cpexpr(dovarp), DOINIT);
	q = mkexpr( (ctlstack->dostepsign==POSSTEP ? OPLE : OPGE), q1, q);
	putif(q, ctlstack->endlabel);
	}
else
	{
	if(! ISCONST(ctlstack->domax) )
		puteq( cpexpr(ctlstack->domax), DOLIMIT);
	q = DOINIT;
	if( ! onetripflag )
		q = mkexpr(OPMINUS, q,
			mkexpr(OPASSIGN, cpexpr(ctlstack->dostep), DOINCR) );
	puteq( cpexpr(dovarp), q);
	if(onetripflag && ctlstack->dostepsign==VARSTEP)
		puteq( cpexpr(ctlstack->dostep), DOINCR);
	}

if(ctlstack->dostepsign == VARSTEP)
	{
	if(onetripflag)
		putgoto(ctlstack->dobodylabel);
	else
		putif( mkexpr(OPGE, cpexpr(ctlstack->dostep), MKICON(0)),
			ctlstack->doneglabel );
	putlabel(ctlstack->doposlabel);

	p = cpexpr(dovarp);
	putif( mkexpr(OPLE, mkexpr(OPASSIGN, p,
	    mkexpr(OPPLUS, cpexpr(dovarp), cpexpr(ctlstack->dostep))),
	    cpexpr(ctlstack->domax)), ctlstack->endlabel);
	}
putlabel(ctlstack->dobodylabel);
if(dostgp)
	puteq(dostgp, cpexpr(dovarp));
frexpr(dovarp);
}

/*
 * Reached the end of a DO statement.
 */
void
enddo(int here)
{
	register struct ctlframe *q;
	register bigptr t;
	struct bigblock *np;
	struct bigblock *ap;
	register int i;

	while(here == dorange) {
		if((np = ctlstack->donamep)) {

			t = mklhs(mkprim(ctlstack->donamep, 0,0 ,0));
			t = mkexpr(OPASSIGN, cpexpr(t), 
			    mkexpr(OPPLUS, t, cpexpr(ctlstack->dostep)));

			if(ctlstack->dostepsign == VARSTEP) {
				putif( mkexpr(OPLE, cpexpr(ctlstack->dostep),
				    MKICON(0)), ctlstack->doposlabel);
				putlabel(ctlstack->doneglabel);
				putif( mkexpr(OPLT, t, ctlstack->domax),
				    ctlstack->dobodylabel);
			} else
				putif( mkexpr( (ctlstack->dostepsign==POSSTEP ?
					OPGT : OPLT), t, ctlstack->domax),
					ctlstack->dobodylabel);
			putlabel(ctlstack->endlabel);
			if((ap = memversion(np)))
				puteq(ap, mklhs( mkprim(np,0,0,0)) );
			for(i = 0 ; i < 4 ; ++i)
				ctlstack->ctlabels[i] = 0;
			deregister(ctlstack->donamep);
			ctlstack->donamep->b_name.vdovar = NO;
			frexpr(ctlstack->dostep);
		}

		popctl();
		dorange = 0;
		for(q = ctlstack ; q>=ctls ; --q)
			if(q->ctltype == CTLDO) {
				dorange = q->dolabel;
				break;
			}
	}
}

void
exassign(vname, labelval)
struct bigblock *vname;
struct labelblock *labelval;
{
struct bigblock *p;

p = mklhs(mkprim(vname,0,0,0));
if( ! ONEOF(p->vtype, MSKINT|MSKADDR) )
	err("noninteger assign variable");
else
	puteq(p, mkaddcon(labelval->labelno) );
}


void
exarif(expr, neglab, zerlab, poslab)
bigptr expr;
struct labelblock *neglab, *zerlab, *poslab;
{
register int lm, lz, lp;

lm = neglab->labelno;
lz = zerlab->labelno;
lp = poslab->labelno;
expr = fixtype(expr);

if( ! ONEOF(expr->vtype, MSKINT|MSKREAL) )
	{
	err("invalid type of arithmetic if expression");
	frexpr(expr);
	}
else
	{
	if(lm == lz)
		exar2(OPLE, expr, lm, lp);
	else if(lm == lp)
		exar2(OPNE, expr, lm, lz);
	else if(lz == lp)
		exar2(OPGE, expr, lz, lm);
	else
		prarif(expr, lm, lz, lp);
	}
}



LOCAL void exar2(op, e, l1, l2)
int op;
bigptr e;
int l1, l2;
{
putif( mkexpr(op, e, MKICON(0)), l2);
putgoto(l1);
}

void
exreturn(p)
register bigptr p;
{
if(p && (proctype!=TYSUBR || procclass!=CLPROC) )
	{
	err("alternate return in nonsubroutine");
	p = 0;
	}

if(p)
	{
	putforce(TYINT, p);
	putgoto(retlabel);
	}
else
	putgoto(procclass==TYSUBR ? ret0label : retlabel);
}


void
exasgoto(labvar)
bigptr labvar;
{
register struct bigblock *p;

p = mklhs( mkprim(labvar,0,0,0) );
if( ! ISINT(p->vtype) )
	err("assigned goto variable must be integer");
else
	putbranch(p);
}
