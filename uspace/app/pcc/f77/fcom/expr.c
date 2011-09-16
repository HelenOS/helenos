/*	$Id: expr.c,v 1.20 2008/05/11 15:28:03 ragge Exp $	*/
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
#include <string.h>

#include "defines.h"
#include "defs.h"

/* little routines to create constant blocks */
LOCAL int letter(int c);
LOCAL void conspower(union constant *, struct bigblock *, ftnint);
LOCAL void consbinop(int, int, union constant *, union constant *,
	union constant *);
LOCAL void zdiv(struct dcomplex *, struct dcomplex *, struct dcomplex *);
LOCAL struct bigblock *stfcall(struct bigblock *, struct bigblock *);
LOCAL bigptr mkpower(struct bigblock *p);
LOCAL bigptr fold(struct bigblock *e);
LOCAL bigptr subcheck(struct bigblock *, bigptr);

struct bigblock *mkconst(t)
register int t;
{
register struct bigblock *p;

p = BALLO();
p->tag = TCONST;
p->vtype = t;
return(p);
}


struct bigblock *mklogcon(l)
register int l;
{
register struct bigblock * p;

p = mkconst(TYLOGICAL);
p->b_const.fconst.ci = l;
return(p);
}



struct bigblock *mkintcon(l)
ftnint l;
{
register struct bigblock *p;

p = mkconst(TYLONG);
p->b_const.fconst.ci = l;
#ifdef MAXSHORT
	if(l >= -MAXSHORT   &&   l <= MAXSHORT)
		p->vtype = TYSHORT;
#endif
return(p);
}



struct bigblock *mkaddcon(l)
register int l;
{
register struct bigblock *p;

p = mkconst(TYADDR);
p->b_const.fconst.ci = l;
return(p);
}



struct bigblock *mkrealcon(t, d)
register int t;
double d;
{
register struct bigblock *p;

p = mkconst(t);
p->b_const.fconst.cd[0] = d;
return(p);
}


struct bigblock *mkbitcon(shift, leng, s)
int shift;
int leng;
char *s;
{
register struct bigblock *p;

p = mkconst(TYUNKNOWN);
p->b_const.fconst.ci = 0;
while(--leng >= 0)
	if(*s != ' ')
		p->b_const.fconst.ci = (p->b_const.fconst.ci << shift) | hextoi(*s++);
return(p);
}





struct bigblock *mkstrcon(l,v)
int l;
register char *v;
{
register struct bigblock *p;
register char *s;

p = mkconst(TYCHAR);
p->vleng = MKICON(l);
p->b_const.fconst.ccp = s = (char *) ckalloc(l);
while(--l >= 0)
	*s++ = *v++;
return(p);
}


struct bigblock *mkcxcon(realp,imagp)
register bigptr realp, imagp;
{
int rtype, itype;
register struct bigblock *p;

rtype = realp->vtype;
itype = imagp->vtype;

if( ISCONST(realp) && ISNUMERIC(rtype) && ISCONST(imagp) && ISNUMERIC(itype) )
	{
	p = mkconst( (rtype==TYDREAL||itype==TYDREAL) ? TYDCOMPLEX : TYCOMPLEX );
	if( ISINT(rtype) )
		p->b_const.fconst.cd[0] = realp->b_const.fconst.ci;
	else	p->b_const.fconst.cd[0] = realp->b_const.fconst.cd[0];
	if( ISINT(itype) )
		p->b_const.fconst.cd[1] = imagp->b_const.fconst.ci;
	else	p->b_const.fconst.cd[1] = imagp->b_const.fconst.cd[0];
	}
else
	{
	err("invalid complex constant");
	p = errnode();
	}

frexpr(realp);
frexpr(imagp);
return(p);
}


struct bigblock *errnode()
{
struct bigblock *p;
p = BALLO();
p->tag = TERROR;
p->vtype = TYERROR;
return(p);
}





bigptr mkconv(t, p)
register int t;
register bigptr p;
{
register bigptr q;

if(t==TYUNKNOWN || t==TYERROR)
	fatal1("mkconv of impossible type %d", t);
if(t == p->vtype)
	return(p);

else if( ISCONST(p) && p->vtype!=TYADDR)
	{
	q = mkconst(t);
	consconv(t, &(q->b_const.fconst), p->vtype, &(p->b_const.fconst));
	frexpr(p);
	}
else
	{
	q = mkexpr(OPCONV, p, 0);
	q->vtype = t;
	}
return(q);
}



struct bigblock *addrof(p)
bigptr p;
{
return( mkexpr(OPADDR, p, NULL) );
}



bigptr
cpexpr(p)
register bigptr p;
{
register bigptr e;
int tag;
register chainp ep, pp;

#if 0
static int blksize[ ] = { 0, sizeof(struct nameblock), sizeof(struct constblock),
		 sizeof(struct exprblock), sizeof(struct addrblock),
		 sizeof(struct primblock), sizeof(struct listblock),
		 sizeof(struct errorblock)
	};
#endif

if(p == NULL)
	return(NULL);

if( (tag = p->tag) == TNAME)
	return(p);

#if 0
e = cpblock( blksize[p->tag] , p);
#else
e = cpblock( sizeof(struct bigblock) , p);
#endif

switch(tag)
	{
	case TCONST:
		if(e->vtype == TYCHAR)
			{
			e->b_const.fconst.ccp = copyn(1+strlen(e->b_const.fconst.ccp), e->b_const.fconst.ccp);
			e->vleng = cpexpr(e->vleng);
			}
	case TERROR:
		break;

	case TEXPR:
		e->b_expr.leftp = cpexpr(p->b_expr.leftp);
		e->b_expr.rightp = cpexpr(p->b_expr.rightp);
		break;

	case TLIST:
		if((pp = p->b_list.listp))
			{
			ep = e->b_list.listp = mkchain( cpexpr(pp->chain.datap), NULL);
			for(pp = pp->chain.nextp ; pp ; pp = pp->chain.nextp)
				ep = ep->chain.nextp = mkchain( cpexpr(pp->chain.datap), NULL);
			}
		break;

	case TADDR:
		e->vleng = cpexpr(e->vleng);
		e->b_addr.memoffset = cpexpr(e->b_addr.memoffset);
		e->b_addr.istemp = NO;
		break;

	case TPRIM:
		e->b_prim.argsp = cpexpr(e->b_prim.argsp);
		e->b_prim.fcharp = cpexpr(e->b_prim.fcharp);
		e->b_prim.lcharp = cpexpr(e->b_prim.lcharp);
		break;

	default:
		fatal1("cpexpr: impossible tag %d", tag);
	}

return(e);
}

void
frexpr(p)
register bigptr p;
{
register chainp q;

if(p == NULL)
	return;

switch(p->tag)
	{
	case TCONST:
		if( ISCHAR(p) )
			{
			ckfree(p->b_const.fconst.ccp);
			frexpr(p->vleng);
			}
		break;

	case TADDR:
		if(p->b_addr.istemp)
			{
			frtemp(p);
			return;
			}
		frexpr(p->vleng);
		frexpr(p->b_addr.memoffset);
		break;

	case TERROR:
		break;

	case TNAME:
		return;

	case TPRIM:
		frexpr(p->b_prim.argsp);
		frexpr(p->b_prim.fcharp);
		frexpr(p->b_prim.lcharp);
		break;

	case TEXPR:
		frexpr(p->b_expr.leftp);
		if(p->b_expr.rightp)
			frexpr(p->b_expr.rightp);
		break;

	case TLIST:
		for(q = p->b_list.listp ; q ; q = q->chain.nextp)
			frexpr(q->chain.datap);
		frchain( &(p->b_list.listp) );
		break;

	default:
		fatal1("frexpr: impossible tag %d", p->tag);
	}

ckfree(p);
}

/* fix up types in expression; replace subtrees and convert
   names to address blocks */

bigptr fixtype(p)
register bigptr p;
{

if(p == 0)
	return(0);

switch(p->tag)
	{
	case TCONST:
		if( ! ONEOF(p->vtype, MSKINT|MSKLOGICAL|MSKADDR) )
			p = putconst(p);
		return(p);

	case TADDR:
		p->b_addr.memoffset = fixtype(p->b_addr.memoffset);
		return(p);

	case TERROR:
		return(p);

	default:
		fatal1("fixtype: impossible tag %d", p->tag);

	case TEXPR:
		return( fixexpr(p) );

	case TLIST:
		return( p );

	case TPRIM:
		if(p->b_prim.argsp && p->b_prim.namep->vclass!=CLVAR)
			return( mkfunct(p) );
		else	return( mklhs(p) );
	}
}





/* special case tree transformations and cleanups of expression trees */

bigptr fixexpr(p)
register struct bigblock *p;
{
bigptr lp;
register bigptr rp;
register bigptr q;
int opcode, ltype, rtype, ptype, mtype;

if(p->tag == TERROR)
	return(p);
else if(p->tag != TEXPR)
	fatal1("fixexpr: invalid tag %d", p->tag);
opcode = p->b_expr.opcode;
lp = p->b_expr.leftp = fixtype(p->b_expr.leftp);
ltype = lp->vtype;
if(opcode==OPASSIGN && lp->tag!=TADDR)
	{
	err("left side of assignment must be variable");
	frexpr(p);
	return( errnode() );
	}

if(p->b_expr.rightp)
	{
	rp = p->b_expr.rightp = fixtype(p->b_expr.rightp);
	rtype = rp->vtype;
	}
else
	{
	rp = NULL;
	rtype = 0;
	}

/* force folding if possible */
if( ISCONST(lp) && (rp==NULL || ISCONST(rp)) )
	{
	q = mkexpr(opcode, lp, rp);
	if( ISCONST(q) )
		return(q);
	ckfree(q);	/* constants did not fold */
	}

if( (ptype = cktype(opcode, ltype, rtype)) == TYERROR)
	{
	frexpr(p);
	return( errnode() );
	}

switch(opcode)
	{
	case OPCONCAT:
		if(p->vleng == NULL)
			p->vleng = mkexpr(OPPLUS, cpexpr(lp->vleng),
				cpexpr(rp->vleng) );
		break;

	case OPASSIGN:
		if(ltype == rtype)
			break;
		if( ! ISCONST(rp) && ISREAL(ltype) && ISREAL(rtype) )
			break;
		if( ISCOMPLEX(ltype) || ISCOMPLEX(rtype) )
			break;
		if( ONEOF(ltype, MSKADDR|MSKINT) && ONEOF(rtype, MSKADDR|MSKINT)
		    && typesize[ltype]>=typesize[rtype] )
			break;
		p->b_expr.rightp = fixtype( mkconv(ptype, rp) );
		break;

	case OPSLASH:
		if( ISCOMPLEX(rtype) )
			{
			p = call2(ptype, ptype==TYCOMPLEX? "c_div" : "z_div",
				mkconv(ptype, lp), mkconv(ptype, rp) );
			break;
			}
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
	case OPMOD:
		if(ptype==TYDREAL && ( (ltype==TYREAL && ! ISCONST(lp) ) ||
		    (rtype==TYREAL && ! ISCONST(rp) ) ))
			break;
		if( ISCOMPLEX(ptype) )
			break;
		if(ltype != ptype)
			p->b_expr.leftp = fixtype(mkconv(ptype,lp));
		if(rtype != ptype)
			p->b_expr.rightp = fixtype(mkconv(ptype,rp));
		break;

	case OPPOWER:
		return( mkpower(p) );

	case OPLT:
	case OPLE:
	case OPGT:
	case OPGE:
	case OPEQ:
	case OPNE:
		if(ltype == rtype)
			break;
		mtype = cktype(OPMINUS, ltype, rtype);
		if(mtype==TYDREAL && ( (ltype==TYREAL && ! ISCONST(lp)) ||
		    (rtype==TYREAL && ! ISCONST(rp)) ))
			break;
		if( ISCOMPLEX(mtype) )
			break;
		if(ltype != mtype)
			p->b_expr.leftp = fixtype(mkconv(mtype,lp));
		if(rtype != mtype)
			p->b_expr.rightp = fixtype(mkconv(mtype,rp));
		break;


	case OPCONV:
		ptype = cktype(OPCONV, p->vtype, ltype);
		if(lp->tag==TEXPR && lp->b_expr.opcode==OPCOMMA)
			{
			lp->b_expr.rightp = fixtype( mkconv(ptype, lp->b_expr.rightp) );
			ckfree(p);
			p = lp;
			}
		break;

	case OPADDR:
		if(lp->tag==TEXPR && lp->b_expr.opcode==OPADDR)
			fatal("addr of addr");
		break;

	case OPCOMMA:
		break;

	case OPMIN:
	case OPMAX:
		ptype = p->vtype;
		break;

	default:
		break;
	}

p->vtype = ptype;
return(p);
}

#if SZINT < SZLONG
/*
   for efficient subscripting, replace long ints by shorts
   in easy places
*/

bigptr shorten(p)
register bigptr p;
{
register bigptr q;

if(p->vtype != TYLONG)
	return(p);

switch(p->tag)
	{
	case TERROR:
	case TLIST:
		return(p);

	case TCONST:
	case TADDR:
		return( mkconv(TYINT,p) );

	case TEXPR:
		break;

	default:
		fatal1("shorten: invalid tag %d", p->tag);
	}

switch(p->opcode)
	{
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
		q = shorten( cpexpr(p->rightp) );
		if(q->vtype == TYINT)
			{
			p->leftp = shorten(p->leftp);
			if(p->leftp->vtype == TYLONG)
				frexpr(q);
			else
				{
				frexpr(p->rightp);
				p->rightp = q;
				p->vtype = TYINT;
				}
			}
		break;

	case OPNEG:
		p->leftp = shorten(p->leftp);
		if(p->leftp->vtype == TYINT)
			p->vtype = TYINT;
		break;

	case OPCALL:
	case OPCCALL:
		p = mkconv(TYINT,p);
		break;
	default:
		break;
	}

return(p);
}
#endif

int
fixargs(doput, p0)
int doput;
struct bigblock *p0;
{
register chainp p;
register bigptr q, t;
register int qtag;
int nargs;

nargs = 0;
if(p0)
    for(p = p0->b_list.listp ; p ; p = p->chain.nextp)
	{
	++nargs;
	q = p->chain.datap;
	qtag = q->tag;
	if(qtag == TCONST)
		{
		if(q->vtype == TYSHORT)
			q = mkconv(tyint, q);
		if(doput)
			p->chain.datap = putconst(q);
		else
			p->chain.datap = q;
		}
	else if(qtag==TPRIM && q->b_prim.argsp==0 && q->b_prim.namep->vclass==CLPROC)
		p->chain.datap = mkaddr(q->b_prim.namep);
	else if(qtag==TPRIM && q->b_prim.argsp==0 && q->b_prim.namep->b_name.vdim!=NULL)
		p->chain.datap = mkscalar(q->b_prim.namep);
	else if(qtag==TPRIM && q->b_prim.argsp==0 && q->b_prim.namep->b_name.vdovar && 
		(t = memversion(q->b_prim.namep)) )
			p->chain.datap = fixtype(t);
	else	p->chain.datap = fixtype(q);
	}
return(nargs);
}

struct bigblock *
mkscalar(np)
register struct bigblock *np;
{
register struct bigblock *ap;

vardcl(np);
ap = mkaddr(np);

#ifdef __vax__
	/* on the VAX, prolog causes array arguments
	   to point at the (0,...,0) element, except when
	   subscript checking is on
	*/
	if( !checksubs && np->vstg==STGARG)
		{
		register struct dimblock *dp;
		dp = np->vdim;
		frexpr(ap->memoffset);
		ap->memoffset = mkexpr(OPSTAR, MKICON(typesize[np->vtype]),
					cpexpr(dp->baseoffset) );
		}
#endif
return(ap);
}





bigptr mkfunct(p)
register struct bigblock * p;
{
chainp ep;
struct bigblock *ap;
struct extsym *extp;
register struct bigblock *np;
register struct bigblock *q;
int k, nargs;
int class;

np = p->b_prim.namep;
class = np->vclass;

if(class == CLUNKNOWN)
	{
	np->vclass = class = CLPROC;
	if(np->vstg == STGUNKNOWN)
		{
		if((k = intrfunct(np->b_name.varname)))
			{
			np->vstg = STGINTR;
			np->b_name.vardesc.varno = k;
			np->b_name.vprocclass = PINTRINSIC;
			}
		else
			{
			extp = mkext( varunder(VL,np->b_name.varname) );
			extp->extstg = STGEXT;
			np->vstg = STGEXT;
			np->b_name.vardesc.varno = extp - extsymtab;
			np->b_name.vprocclass = PEXTERNAL;
			}
		}
	else if(np->vstg==STGARG)
		{
		if(np->vtype!=TYCHAR && !ftn66flag)
		    warn("Dummy procedure not declared EXTERNAL. Code may be wrong.");
		np->b_name.vprocclass = PEXTERNAL;
		}
	}

if(class != CLPROC)
	fatal1("invalid class code for function", class);
if(p->b_prim.fcharp || p->b_prim.lcharp)
	{
	err("no substring of function call");
	goto error;
	}
impldcl(np);
nargs = fixargs( np->b_name.vprocclass!=PINTRINSIC,  p->b_prim.argsp);

switch(np->b_name.vprocclass)
	{
	case PEXTERNAL:
		ap = mkaddr(np);
	call:
		q = mkexpr(OPCALL, ap, p->b_prim.argsp);
		q->vtype = np->vtype;
		if(np->vleng)
			q->vleng = cpexpr(np->vleng);
		break;

	case PINTRINSIC:
		q = intrcall(np, p->b_prim.argsp, nargs);
		break;

	case PSTFUNCT:
		q = stfcall(np, p->b_prim.argsp);
		break;

	case PTHISPROC:
		warn("recursive call");
		for(ep = entries ; ep ; ep = ep->entrypoint.nextp)
			if(ep->entrypoint.enamep == np)
				break;
		if(ep == NULL)
			fatal("mkfunct: impossible recursion");
		ap = builtin(np->vtype, varstr(XL, ep->entrypoint.entryname->extname) );
		goto call;

	default:
		fatal1("mkfunct: impossible vprocclass %d", np->b_name.vprocclass);
		q = 0; /* XXX gcc */
	}
ckfree(p);
return(q);

error:
	frexpr(p);
	return( errnode() );
}



LOCAL struct bigblock *
stfcall(struct bigblock *np, struct bigblock *actlist)
{
	register chainp actuals;
	int nargs;
	chainp oactp, formals;
	int type;
	struct bigblock *q, *rhs;
	bigptr ap;
	register chainp rp;
	chainp tlist;

	if(actlist) {
		actuals = actlist->b_list.listp;
		ckfree(actlist);
	} else
		actuals = NULL;
	oactp = actuals;

	nargs = 0;
	tlist = NULL;
	type = np->vtype;

	formals = (chainp)np->b_name.vardesc.vstfdesc->chain.datap;
	rhs = (bigptr)np->b_name.vardesc.vstfdesc->chain.nextp;

	/* copy actual arguments into temporaries */
	while(actuals!=NULL && formals!=NULL) {
		rp = ALLOC(rplblock);
		rp->rplblock.rplnp = q = formals->chain.datap;
		ap = fixtype(actuals->chain.datap);
		if(q->vtype==ap->vtype && q->vtype!=TYCHAR
		   && (ap->tag==TCONST || ap->tag==TADDR) ) {
			rp->rplblock.rplvp = ap;
			rp->rplblock.rplxp = NULL;
			rp->rplblock.rpltag = ap->tag;
		} else	{
			rp->rplblock.rplvp = fmktemp(q->vtype, q->vleng);
			rp->rplblock.rplxp = fixtype( mkexpr(OPASSIGN,
			    cpexpr(rp->rplblock.rplvp), ap) );
			if( (rp->rplblock.rpltag =
			    rp->rplblock.rplxp->tag) == TERROR)
				err("disagreement of argument types in statement function call");
		}
		rp->rplblock.nextp = tlist;
		tlist = rp;
		actuals = actuals->chain.nextp;
		formals = formals->chain.nextp;
		++nargs;
	}

	if(actuals!=NULL || formals!=NULL)
		err("statement function definition and argument list differ");

	/*
	   now push down names involved in formal argument list, then
	   evaluate rhs of statement function definition in this environment
	*/
	rpllist = hookup(tlist, rpllist);
	q = mkconv(type, fixtype(cpexpr(rhs)) );

	/* now generate the tree ( t1=a1, (t2=a2,... , f))))) */
	while(--nargs >= 0) {
		if(rpllist->rplblock.rplxp)
			q = mkexpr(OPCOMMA, rpllist->rplblock.rplxp, q);
		rp = rpllist->rplblock.nextp;
		frexpr(rpllist->rplblock.rplvp);
		ckfree(rpllist);
		rpllist = rp;
	}

	frchain( &oactp );
	return(q);
}




struct bigblock *
mklhs(struct bigblock *p)
{
	struct bigblock *s;
	struct bigblock *np;
	chainp rp;
	int regn;

	/* first fixup name */

	if(p->tag != TPRIM)
		return(p);

	np = p->b_prim.namep;

	/* is name on the replace list? */

	for(rp = rpllist ; rp ; rp = rp->rplblock.nextp) {
		if(np == rp->rplblock.rplnp) {
			if(rp->rplblock.rpltag == TNAME) {
				np = p->b_prim.namep = rp->rplblock.rplvp;
				break;
			} else
				return( cpexpr(rp->rplblock.rplvp) );
		}
	}

	/* is variable a DO index in a register ? */

	if(np->b_name.vdovar && ( (regn = inregister(np)) >= 0) ) {
		if(np->vtype == TYERROR)
			return( errnode() );
		else {
			s = BALLO();
			s->tag = TADDR;
			s->vstg = STGREG;
			s->vtype = TYIREG;
			s->b_addr.memno = regn;
			s->b_addr.memoffset = MKICON(0);
			return(s);
		}
	}

	vardcl(np);
	s = mkaddr(np);
	s->b_addr.memoffset = mkexpr(OPPLUS, s->b_addr.memoffset, suboffset(p) );
	frexpr(p->b_prim.argsp);
	p->b_prim.argsp = NULL;

	/* now do substring part */

	if(p->b_prim.fcharp || p->b_prim.lcharp) {
		if(np->vtype != TYCHAR)
			err1("substring of noncharacter %s",
			    varstr(VL,np->b_name.varname));
		else	{
			if(p->b_prim.lcharp == NULL)
				p->b_prim.lcharp = cpexpr(s->vleng);
			if(p->b_prim.fcharp)
				s->vleng = mkexpr(OPMINUS, p->b_prim.lcharp,
					mkexpr(OPMINUS, p->b_prim.fcharp, MKICON(1) ));
			else	{
				frexpr(s->vleng);
				s->vleng = p->b_prim.lcharp;
			}
		}
	}

	s->vleng = fixtype( s->vleng );
	s->b_addr.memoffset = fixtype( s->b_addr.memoffset );
	ckfree(p);
	return(s);
}




void
deregister(np)
struct bigblock *np;
{
}




struct bigblock *memversion(np)
register struct bigblock *np;
{
register struct bigblock *s;

if(np->b_name.vdovar==NO || (inregister(np)<0) )
	return(NULL);
np->b_name.vdovar = NO;
s = mklhs( mkprim(np, 0,0,0) );
np->b_name.vdovar = YES;
return(s);
}


int
inregister(np)
register struct bigblock *np;
{
return(-1);
}



int
enregister(np)
struct bigblock *np;
{
	return(NO);
}




bigptr suboffset(p)
register struct bigblock *p;
{
int n;
bigptr size;
chainp cp;
bigptr offp, prod;
struct dimblock *dimp;
bigptr sub[8];
register struct bigblock *np;

np = p->b_prim.namep;
offp = MKICON(0);
n = 0;
if(p->b_prim.argsp)
	for(cp = p->b_prim.argsp->b_list.listp ; cp ; cp = cp->chain.nextp)
		{
		sub[n++] = fixtype(cpexpr(cp->chain.datap));
		if(n > 7)
			{
			err("more than 7 subscripts");
			break;
			}
		}

dimp = np->b_name.vdim;
if(n>0 && dimp==NULL)
	err("subscripts on scalar variable");
else if(dimp && dimp->ndim!=n)
	err1("wrong number of subscripts on %s",
		varstr(VL, np->b_name.varname) );
else if(n > 0)
	{
	prod = sub[--n];
	while( --n >= 0)
		prod = mkexpr(OPPLUS, sub[n],
			mkexpr(OPSTAR, prod, cpexpr(dimp->dims[n].dimsize)) );
#ifdef __vax__
	if(checksubs || np->vstg!=STGARG)
		prod = mkexpr(OPMINUS, prod, cpexpr(dimp->baseoffset));
#else
	prod = mkexpr(OPMINUS, prod, cpexpr(dimp->baseoffset));
#endif
	if(checksubs)
		prod = subcheck(np, prod);
	if(np->vtype == TYCHAR)
		size = cpexpr(np->vleng);
	else	size = MKICON( typesize[np->vtype] );
	prod = mkexpr(OPSTAR, prod, size);
	offp = mkexpr(OPPLUS, offp, prod);
	}

if(p->b_prim.fcharp && np->vtype==TYCHAR)
	offp = mkexpr(OPPLUS, offp, mkexpr(OPMINUS, cpexpr(p->b_prim.fcharp), MKICON(1) ));

return(offp);
}


/*
 * Check if an array is addressed out of bounds.
 */
bigptr
subcheck(struct bigblock *np, bigptr p)
{
	struct dimblock *dimp;
	bigptr t, badcall;
	int l1, l2;

	dimp = np->b_name.vdim;
	if(dimp->nelt == NULL)
		return(p);	/* don't check arrays with * bounds */
	if( ISICON(p) ) {
		if(p->b_const.fconst.ci < 0)
			goto badsub;
		if( ISICON(dimp->nelt) ) {
			if(p->b_const.fconst.ci < dimp->nelt->b_const.fconst.ci)
				return(p);
			else
				goto badsub;
		}
	}

	if (p->tag==TADDR && p->vstg==STGREG) {
		t = p;
	} else {
		t = fmktemp(p->vtype, NULL);
		putexpr(mkexpr(OPASSIGN, cpexpr(t), p));
	}
	/* t now cotains evaluated expression */

	l1 = newlabel();
	l2 = newlabel();
	putif(mkexpr(OPLT, cpexpr(t), cpexpr(dimp->nelt)), l1);
	putif(mkexpr(OPGE, cpexpr(t), MKICON(0)), l1);
	putgoto(l2);
	putlabel(l1);
	
	badcall = call4(t->vtype, "s_rnge", mkstrcon(VL, np->b_name.varname),
		mkconv(TYLONG,  cpexpr(t)),
		mkstrcon(XL, procname), MKICON(lineno));
	badcall->b_expr.opcode = OPCCALL;

	putexpr(badcall);
	putlabel(l2);
	return t;

badsub:
	frexpr(p);
	err1("subscript on variable %s out of range",
	    varstr(VL,np->b_name.varname));
	return ( MKICON(0) );
}




struct bigblock *mkaddr(p)
register struct bigblock *p;
{
struct extsym *extp;
register struct bigblock *t;

switch( p->vstg)
	{
	case STGUNKNOWN:
		if(p->vclass != CLPROC)
			break;
		extp = mkext( varunder(VL, p->b_name.varname) );
		extp->extstg = STGEXT;
		p->vstg = STGEXT;
		p->b_name.vardesc.varno = extp - extsymtab;
		p->b_name.vprocclass = PEXTERNAL;

	case STGCOMMON:
	case STGEXT:
	case STGBSS:
	case STGINIT:
	case STGEQUIV:
	case STGARG:
	case STGLENG:
	case STGAUTO:
		t = BALLO();
		t->tag = TADDR;
		t->vclass = p->vclass;
		t->vtype = p->vtype;
		t->vstg = p->vstg;
		t->b_addr.memno = p->b_name.vardesc.varno;
		t->b_addr.memoffset = MKICON(p->b_name.voffset);
		if(p->vleng)
			t->vleng = cpexpr(p->vleng);
		return(t);

	case STGINTR:
		return( intraddr(p) );

	}
/*debug*/ fprintf(diagfile, "mkaddr. vtype=%d, vclass=%d\n", p->vtype, p->vclass);
fatal1("mkaddr: impossible storage tag %d", p->vstg);
/* NOTREACHED */
return 0; /* XXX gcc */
}



struct bigblock *
mkarg(type, argno)
int type, argno;
{
register struct bigblock *p;

p = BALLO();
p->tag = TADDR;
p->vtype = type;
p->vclass = CLVAR;
p->vstg = (type==TYLENG ? STGLENG : STGARG);
p->b_addr.memno = argno;
return(p);
}




bigptr mkprim(v, args, lstr, rstr)
register bigptr v;
struct bigblock *args;
bigptr lstr, rstr;
{
register struct bigblock *p;

if(v->vclass == CLPARAM)
	{
	if(args || lstr || rstr)
		{
		err1("no qualifiers on parameter name", varstr(VL,v->b_name.varname));
		frexpr(args);
		frexpr(lstr);
		frexpr(rstr);
		frexpr(v);
		return( errnode() );
		}
	return( cpexpr(v->b_param.paramval) );
	}

p = BALLO();
p->tag = TPRIM;
p->vtype = v->vtype;
p->b_prim.namep = v;
p->b_prim.argsp = args;
p->b_prim.fcharp = lstr;
p->b_prim.lcharp = rstr;
return(p);
}


void
vardcl(v)
register struct bigblock *v;
{
int nelt;
struct dimblock *t;
struct bigblock *p;
bigptr neltp;

if(v->b_name.vdcldone) return;

if(v->vtype == TYUNKNOWN)
	impldcl(v);
if(v->vclass == CLUNKNOWN)
	v->vclass = CLVAR;
else if(v->vclass!=CLVAR && v->b_name.vprocclass!=PTHISPROC)
	{
	dclerr("used as variable", v);
	return;
	}
if(v->vstg==STGUNKNOWN)
	v->vstg = implstg[ letter(v->b_name.varname[0]) ];

switch(v->vstg)
	{
	case STGBSS:
		v->b_name.vardesc.varno = ++lastvarno;
		break;
	case STGAUTO:
		if(v->vclass==CLPROC && v->b_name.vprocclass==PTHISPROC)
			break;
		nelt = 1;
		if((t = v->b_name.vdim)) {
			if( (neltp = t->nelt) && ISCONST(neltp) )
				nelt = neltp->b_const.fconst.ci;
			else
				dclerr("adjustable automatic array", v);
		}
		p = autovar(nelt, v->vtype, v->vleng);
		v->b_name.voffset = p->b_addr.memoffset->b_const.fconst.ci;
		frexpr(p);
		break;

	default:
		break;
	}
v->b_name.vdcldone = YES;
}



void
impldcl(p)
register struct bigblock *p;
{
register int k;
int type, leng;

if(p->b_name.vdcldone || (p->vclass==CLPROC && p->b_name.vprocclass==PINTRINSIC) )
	return;
if(p->vtype == TYUNKNOWN)
	{
	k = letter(p->b_name.varname[0]);
	type = impltype[ k ];
	leng = implleng[ k ];
	if(type == TYUNKNOWN)
		{
		if(p->vclass == CLPROC)
			return;
		dclerr("attempt to use undefined variable", p);
		type = TYERROR;
		leng = 1;
		}
	settype(p, type, leng);
	}
}




LOCAL int
letter(c)
register int c;
{
if( isupper(c) )
	c = tolower(c);
return(c - 'a');
}

#define ICONEQ(z, c)  (ISICON(z) && z->b_const.fconst.ci==c)
#define COMMUTE	{ e = lp;  lp = rp;  rp = e; }


struct bigblock * 
mkexpr(opcode, lp, rp)
int opcode;
register bigptr lp, rp;
{
register struct bigblock *e, *e1;
int etype;
int ltype, rtype;
int ltag, rtag;

ltype = lp->vtype;
ltag = lp->tag;
if(rp && opcode!=OPCALL && opcode!=OPCCALL)
	{
	rtype = rp->vtype;
	rtag = rp->tag;
	}
else  rtype = rtag = 0;

etype = cktype(opcode, ltype, rtype);
if(etype == TYERROR)
	goto error;

switch(opcode)
	{
	/* check for multiplication by 0 and 1 and addition to 0 */

	case OPSTAR:
		if( ISCONST(lp) )
			COMMUTE

		if( ISICON(rp) )
			{
			if(rp->b_const.fconst.ci == 0)
				goto retright;
			goto mulop;
			}
		break;

	case OPSLASH:
	case OPMOD:
		if( ICONEQ(rp, 0) )
			{
			err("attempted division by zero");
			rp = MKICON(1);
			break;
			}
		if(opcode == OPMOD)
			break;


	mulop:
		if( ISICON(rp) )
			{
			if(rp->b_const.fconst.ci == 1)
				goto retleft;

			if(rp->b_const.fconst.ci == -1)
				{
				frexpr(rp);
				return( mkexpr(OPNEG, lp, 0) );
				}
			}

		if( ISSTAROP(lp) && ISICON(lp->b_expr.rightp) )
			{
			if(opcode == OPSTAR)
				e = mkexpr(OPSTAR, lp->b_expr.rightp, rp);
			else  if(ISICON(rp) && lp->b_expr.rightp->b_const.fconst.ci % rp->b_const.fconst.ci == 0)
				e = mkexpr(OPSLASH, lp->b_expr.rightp, rp);
			else	break;

			e1 = lp->b_expr.leftp;
			ckfree(lp);
			return( mkexpr(OPSTAR, e1, e) );
			}
		break;


	case OPPLUS:
		if( ISCONST(lp) )
			COMMUTE
		goto addop;

	case OPMINUS:
		if( ICONEQ(lp, 0) )
			{
			frexpr(lp);
			return( mkexpr(OPNEG, rp, 0) );
			}

		if( ISCONST(rp) )
			{
			opcode = OPPLUS;
			consnegop(rp);
			}

	addop:
		if( ISICON(rp) )
			{
			if(rp->b_const.fconst.ci == 0)
				goto retleft;
			if( ISPLUSOP(lp) && ISICON(lp->b_expr.rightp) )
				{
				e = mkexpr(OPPLUS, lp->b_expr.rightp, rp);
				e1 = lp->b_expr.leftp;
				ckfree(lp);
				return( mkexpr(OPPLUS, e1, e) );
				}
			}
		break;


	case OPPOWER:
		break;

	case OPNEG:
		if(ltag==TEXPR && lp->b_expr.opcode==OPNEG)
			{
			e = lp->b_expr.leftp;
			ckfree(lp);
			return(e);
			}
		break;

	case OPNOT:
		if(ltag==TEXPR && lp->b_expr.opcode==OPNOT)
			{
			e = lp->b_expr.leftp;
			ckfree(lp);
			return(e);
			}
		break;

	case OPCALL:
	case OPCCALL:
		etype = ltype;
		if(rp!=NULL && rp->b_list.listp==NULL)
			{
			ckfree(rp);
			rp = NULL;
			}
		break;

	case OPAND:
	case OPOR:
		if( ISCONST(lp) )
			COMMUTE

		if( ISCONST(rp) )
			{
			if(rp->b_const.fconst.ci == 0)
				if(opcode == OPOR)
					goto retleft;
				else
					goto retright;
			else if(opcode == OPOR)
				goto retright;
			else
				goto retleft;
			}
	case OPEQV:
	case OPNEQV:

	case OPBITAND:
	case OPBITOR:
	case OPBITXOR:
	case OPBITNOT:
	case OPLSHIFT:
	case OPRSHIFT:

	case OPLT:
	case OPGT:
	case OPLE:
	case OPGE:
	case OPEQ:
	case OPNE:

	case OPCONCAT:
		break;
	case OPMIN:
	case OPMAX:

	case OPASSIGN:

	case OPCONV:
	case OPADDR:

	case OPCOMMA:
		break;

	default:
		fatal1("mkexpr: impossible opcode %d", opcode);
	}

e = BALLO();
e->tag = TEXPR;
e->b_expr.opcode = opcode;
e->vtype = etype;
e->b_expr.leftp = lp;
e->b_expr.rightp = rp;
if(ltag==TCONST && (rp==0 || rtag==TCONST) )
	e = fold(e);
return(e);

retleft:
	frexpr(rp);
	return(lp);

retright:
	frexpr(lp);
	return(rp);

error:
	frexpr(lp);
	if(rp && opcode!=OPCALL && opcode!=OPCCALL)
		frexpr(rp);
	return( errnode() );
}

#define ERR(s)   { errs = s; goto error; }

int
cktype(op, lt, rt)
register int op, lt, rt;
{
char *errs = NULL; /* XXX gcc */

if(lt==TYERROR || rt==TYERROR)
	goto error1;

if(lt==TYUNKNOWN)
	return(TYUNKNOWN);
if(rt==TYUNKNOWN)
	if(op!=OPNOT && op!=OPBITNOT && op!=OPNEG && op!=OPCALL && op!=OPCCALL && op!=OPADDR)
		return(TYUNKNOWN);

switch(op)
	{
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
	case OPSLASH:
	case OPPOWER:
	case OPMOD:
		if( ISNUMERIC(lt) && ISNUMERIC(rt) )
			return( maxtype(lt, rt) );
		ERR("nonarithmetic operand of arithmetic operator")

	case OPNEG:
		if( ISNUMERIC(lt) )
			return(lt);
		ERR("nonarithmetic operand of negation")

	case OPNOT:
		if(lt == TYLOGICAL)
			return(TYLOGICAL);
		ERR("NOT of nonlogical")

	case OPAND:
	case OPOR:
	case OPEQV:
	case OPNEQV:
		if(lt==TYLOGICAL && rt==TYLOGICAL)
			return(TYLOGICAL);
		ERR("nonlogical operand of logical operator")

	case OPLT:
	case OPGT:
	case OPLE:
	case OPGE:
	case OPEQ:
	case OPNE:
		if(lt==TYCHAR || rt==TYCHAR || lt==TYLOGICAL || rt==TYLOGICAL)
			{
			if(lt != rt)
				ERR("illegal comparison")
			}

		else if( ISCOMPLEX(lt) || ISCOMPLEX(rt) )
			{
			if(op!=OPEQ && op!=OPNE)
				ERR("order comparison of complex data")
			}

		else if( ! ISNUMERIC(lt) || ! ISNUMERIC(rt) )
			ERR("comparison of nonarithmetic data")
		return(TYLOGICAL);

	case OPCONCAT:
		if(lt==TYCHAR && rt==TYCHAR)
			return(TYCHAR);
		ERR("concatenation of nonchar data")

	case OPCALL:
	case OPCCALL:
		return(lt);

	case OPADDR:
		return(TYADDR);

	case OPCONV:
		if(rt == 0)
			return(0);
	case OPASSIGN:
		if( ISINT(lt) && rt==TYCHAR)
			return(lt);
		if(lt==TYCHAR || rt==TYCHAR || lt==TYLOGICAL || rt==TYLOGICAL)
			if(op!=OPASSIGN || lt!=rt)
				{
/* debug fprintf(diagfile, " lt=%d, rt=%d, op=%d\n", lt, rt, op); */
/* debug fatal("impossible conversion.  possible compiler bug"); */
				ERR("impossible conversion")
				}
		return(lt);

	case OPMIN:
	case OPMAX:
	case OPBITOR:
	case OPBITAND:
	case OPBITXOR:
	case OPBITNOT:
	case OPLSHIFT:
	case OPRSHIFT:
		return(lt);

	case OPCOMMA:
		return(rt);

	default:
		fatal1("cktype: impossible opcode %d", op);
	}
error:	err(errs);
error1:	return(TYERROR);
}

LOCAL bigptr fold(e)
register struct bigblock *e;
{
struct bigblock *p;
register bigptr lp, rp;
int etype, mtype, ltype, rtype, opcode;
int i, ll, lr;
char *q, *s;
union constant lcon, rcon;

opcode = e->b_expr.opcode;
etype = e->vtype;

lp = e->b_expr.leftp;
ltype = lp->vtype;
rp = e->b_expr.rightp;

if(rp == 0)
	switch(opcode)
		{
		case OPNOT:
			lp->b_const.fconst.ci = ! lp->b_const.fconst.ci;
			return(lp);

		case OPBITNOT:
			lp->b_const.fconst.ci = ~ lp->b_const.fconst.ci;
			return(lp);

		case OPNEG:
			consnegop(lp);
			return(lp);

		case OPCONV:
		case OPADDR:
			return(e);

		default:
			fatal1("fold: invalid unary operator %d", opcode);
		}

rtype = rp->vtype;

p = BALLO();
p->tag = TCONST;
p->vtype = etype;
p->vleng = e->vleng;

switch(opcode)
	{
	case OPCOMMA:
		return(e);

	case OPAND:
		p->b_const.fconst.ci = lp->b_const.fconst.ci && rp->b_const.fconst.ci;
		break;

	case OPOR:
		p->b_const.fconst.ci = lp->b_const.fconst.ci || rp->b_const.fconst.ci;
		break;

	case OPEQV:
		p->b_const.fconst.ci = lp->b_const.fconst.ci == rp->b_const.fconst.ci;
		break;

	case OPNEQV:
		p->b_const.fconst.ci = lp->b_const.fconst.ci != rp->b_const.fconst.ci;
		break;

	case OPBITAND:
		p->b_const.fconst.ci = lp->b_const.fconst.ci & rp->b_const.fconst.ci;
		break;

	case OPBITOR:
		p->b_const.fconst.ci = lp->b_const.fconst.ci | rp->b_const.fconst.ci;
		break;

	case OPBITXOR:
		p->b_const.fconst.ci = lp->b_const.fconst.ci ^ rp->b_const.fconst.ci;
		break;

	case OPLSHIFT:
		p->b_const.fconst.ci = lp->b_const.fconst.ci << rp->b_const.fconst.ci;
		break;

	case OPRSHIFT:
		p->b_const.fconst.ci = lp->b_const.fconst.ci >> rp->b_const.fconst.ci;
		break;

	case OPCONCAT:
		ll = lp->vleng->b_const.fconst.ci;
		lr = rp->vleng->b_const.fconst.ci;
		p->b_const.fconst.ccp = q = (char *) ckalloc(ll+lr);
		p->vleng = MKICON(ll+lr);
		s = lp->b_const.fconst.ccp;
		for(i = 0 ; i < ll ; ++i)
			*q++ = *s++;
		s = rp->b_const.fconst.ccp;
		for(i = 0; i < lr; ++i)
			*q++ = *s++;
		break;


	case OPPOWER:
		if( ! ISINT(rtype) )
			return(e);
		conspower(&(p->b_const.fconst), lp, rp->b_const.fconst.ci);
		break;


	default:
		if(ltype == TYCHAR)
			{
			lcon.ci = cmpstr(lp->b_const.fconst.ccp, rp->b_const.fconst.ccp,
					lp->vleng->b_const.fconst.ci, rp->vleng->b_const.fconst.ci);
			rcon.ci = 0;
			mtype = tyint;
			}
		else	{
			mtype = maxtype(ltype, rtype);
			consconv(mtype, &lcon, ltype, &(lp->b_const.fconst) );
			consconv(mtype, &rcon, rtype, &(rp->b_const.fconst) );
			}
		consbinop(opcode, mtype, &(p->b_const.fconst), &lcon, &rcon);
		break;
	}

frexpr(e);
return(p);
}



/* assign constant l = r , doing coercion */
void
consconv(lt, lv, rt, rv)
int lt, rt;
register union constant *lv, *rv;
{
switch(lt)
	{
	case TYSHORT:
	case TYLONG:
		if( ISINT(rt) )
			lv->ci = rv->ci;
		else	lv->ci = rv->cd[0];
		break;

	case TYCOMPLEX:
	case TYDCOMPLEX:
		switch(rt)
			{
			case TYSHORT:
			case TYLONG:
				/* fall through and do real assignment of
				   first element
				*/
			case TYREAL:
			case TYDREAL:
				lv->cd[1] = 0; break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				lv->cd[1] = rv->cd[1]; break;
			}

	case TYREAL:
	case TYDREAL:
		if( ISINT(rt) )
			lv->cd[0] = rv->ci;
		else	lv->cd[0] = rv->cd[0];
		break;

	case TYLOGICAL:
		lv->ci = rv->ci;
		break;
	}
}


void
consnegop(p)
register struct bigblock *p;
{
switch(p->vtype)
	{
	case TYSHORT:
	case TYLONG:
		p->b_const.fconst.ci = - p->b_const.fconst.ci;
		break;

	case TYCOMPLEX:
	case TYDCOMPLEX:
		p->b_const.fconst.cd[1] = - p->b_const.fconst.cd[1];
		/* fall through and do the real parts */
	case TYREAL:
	case TYDREAL:
		p->b_const.fconst.cd[0] = - p->b_const.fconst.cd[0];
		break;
	default:
		fatal1("consnegop: impossible type %d", p->vtype);
	}
}



LOCAL void
conspower(powp, ap, n)
register union constant *powp;
struct bigblock *ap;
ftnint n;
{
register int type;
union constant x;

switch(type = ap->vtype)	/* pow = 1 */ 
	{
	case TYSHORT:
	case TYLONG:
		powp->ci = 1;
		break;
	case TYCOMPLEX:
	case TYDCOMPLEX:
		powp->cd[1] = 0;
	case TYREAL:
	case TYDREAL:
		powp->cd[0] = 1;
		break;
	default:
		fatal1("conspower: invalid type %d", type);
	}

if(n == 0)
	return;
if(n < 0)
	{
	if( ISINT(type) )
		{
		err("integer ** negative power ");
		return;
		}
	n = - n;
	consbinop(OPSLASH, type, &x, powp, &(ap->b_const.fconst));
	}
else
	consbinop(OPSTAR, type, &x, powp, &(ap->b_const.fconst));

for( ; ; )
	{
	if(n & 01)
		consbinop(OPSTAR, type, powp, powp, &x);
	if(n >>= 1)
		consbinop(OPSTAR, type, &x, &x, &x);
	else
		break;
	}
}



/* do constant operation cp = a op b */


LOCAL void
consbinop(opcode, type, cp, ap, bp)
int opcode, type;
register union constant *ap, *bp, *cp;
{
int k;
double temp;

switch(opcode)
	{
	case OPPLUS:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci + bp->ci;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				cp->cd[1] = ap->cd[1] + bp->cd[1];
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] + bp->cd[0];
				break;
			}
		break;

	case OPMINUS:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci - bp->ci;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				cp->cd[1] = ap->cd[1] - bp->cd[1];
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] - bp->cd[0];
				break;
			}
		break;

	case OPSTAR:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci * bp->ci;
				break;
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] * bp->cd[0];
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				temp = ap->cd[0] * bp->cd[0] -
					    ap->cd[1] * bp->cd[1] ;
				cp->cd[1] = ap->cd[0] * bp->cd[1] +
					    ap->cd[1] * bp->cd[0] ;
				cp->cd[0] = temp;
				break;
			}
		break;
	case OPSLASH:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci / bp->ci;
				break;
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] / bp->cd[0];
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				zdiv(&cp->dc, &ap->dc, &bp->dc);
				break;
			}
		break;

	case OPMOD:
		if( ISINT(type) )
			{
			cp->ci = ap->ci % bp->ci;
			break;
			}
		else
			fatal("inline mod of noninteger");

	default:	  /* relational ops */
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				if(ap->ci < bp->ci)
					k = -1;
				else if(ap->ci == bp->ci)
					k = 0;
				else	k = 1;
				break;
			case TYREAL:
			case TYDREAL:
				if(ap->cd[0] < bp->cd[0])
					k = -1;
				else if(ap->cd[0] == bp->cd[0])
					k = 0;
				else	k = 1;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				if(ap->cd[0] == bp->cd[0] &&
				   ap->cd[1] == bp->cd[1] )
					k = 0;
				else	k = 1;
				break;
			default: /* XXX gcc */
				k = 0;
				break;
			}

		switch(opcode)
			{
			case OPEQ:
				cp->ci = (k == 0);
				break;
			case OPNE:
				cp->ci = (k != 0);
				break;
			case OPGT:
				cp->ci = (k == 1);
				break;
			case OPLT:
				cp->ci = (k == -1);
				break;
			case OPGE:
				cp->ci = (k >= 0);
				break;
			case OPLE:
				cp->ci = (k <= 0);
				break;
			}
		break;
	}
}



int
conssgn(p)
register bigptr p;
{
if( ! ISCONST(p) )
	fatal( "sgn(nonconstant)" );

switch(p->vtype)
	{
	case TYSHORT:
	case TYLONG:
		if(p->b_const.fconst.ci > 0) return(1);
		if(p->b_const.fconst.ci < 0) return(-1);
		return(0);

	case TYREAL:
	case TYDREAL:
		if(p->b_const.fconst.cd[0] > 0) return(1);
		if(p->b_const.fconst.cd[0] < 0) return(-1);
		return(0);

	case TYCOMPLEX:
	case TYDCOMPLEX:
		return(p->b_const.fconst.cd[0]!=0 || p->b_const.fconst.cd[1]!=0);

	default:
		fatal1( "conssgn(type %d)", p->vtype);
	}
/* NOTREACHED */
return 0; /* XXX gcc */
}

char *powint[ ] = { "pow_ii", "pow_ri", "pow_di", "pow_ci", "pow_zi" };


LOCAL bigptr mkpower(p)
register struct bigblock *p;
{
register bigptr q, lp, rp;
int ltype, rtype, mtype;

lp = p->b_expr.leftp;
rp = p->b_expr.rightp;
ltype = lp->vtype;
rtype = rp->vtype;

if(ISICON(rp))
	{
	if(rp->b_const.fconst.ci == 0)
		{
		frexpr(p);
		if( ISINT(ltype) )
			return( MKICON(1) );
		else
			return( putconst( mkconv(ltype, MKICON(1))) );
		}
	if(rp->b_const.fconst.ci < 0)
		{
		if( ISINT(ltype) )
			{
			frexpr(p);
			err("integer**negative");
			return( errnode() );
			}
		rp->b_const.fconst.ci = - rp->b_const.fconst.ci;
		p->b_expr.leftp = lp = fixexpr(mkexpr(OPSLASH, MKICON(1), lp));
		}
	if(rp->b_const.fconst.ci == 1)
		{
		frexpr(rp);
		ckfree(p);
		return(lp);
		}

	if( ONEOF(ltype, MSKINT|MSKREAL) )
		{
		p->vtype = ltype;
		return(p);
		}
	}
if( ISINT(rtype) )
	{
	if(ltype==TYSHORT && rtype==TYSHORT)
		q = call2(TYSHORT, "pow_hh", lp, rp);
	else	{
		if(ltype == TYSHORT)
			{
			ltype = TYLONG;
			lp = mkconv(TYLONG,lp);
			}
		q = call2(ltype, powint[ltype-TYLONG], lp, mkconv(TYLONG, rp));
		}
	}
else if( ISREAL( (mtype = maxtype(ltype,rtype)) ))
	q = call2(mtype, "pow_dd",
		mkconv(TYDREAL,lp), mkconv(TYDREAL,rp));
else	{
	q = call2(TYDCOMPLEX, "pow_zz",
		mkconv(TYDCOMPLEX,lp), mkconv(TYDCOMPLEX,rp));
	if(mtype == TYCOMPLEX)
		q = mkconv(TYCOMPLEX, q);
	}
ckfree(p);
return(q);
}



/* Complex Division.  Same code as in Runtime Library
*/



LOCAL void
zdiv(c, a, b)
register struct dcomplex *a, *b, *c;
{
double ratio, den;
double abr, abi;

if( (abr = b->dreal) < 0.)
	abr = - abr;
if( (abi = b->dimag) < 0.)
	abi = - abi;
if( abr <= abi )
	{
	if(abi == 0)
		fatal("complex division by zero");
	ratio = b->dreal / b->dimag ;
	den = b->dimag * (1 + ratio*ratio);
	c->dreal = (a->dreal*ratio + a->dimag) / den;
	c->dimag = (a->dimag*ratio - a->dreal) / den;
	}

else
	{
	ratio = b->dimag / b->dreal ;
	den = b->dreal * (1 + ratio*ratio);
	c->dreal = (a->dreal + a->dimag*ratio) / den;
	c->dimag = (a->dimag - a->dreal*ratio) / den;
	}

}
