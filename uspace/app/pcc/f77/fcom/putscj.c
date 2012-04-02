/*	$Id: putscj.c,v 1.18 2008/12/19 08:08:48 ragge Exp $	*/
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
/* INTERMEDIATE CODE GENERATION FOR S C JOHNSON C COMPILERS */
/* NEW VERSION USING BINARY POLISH POSTFIX INTERMEDIATE */

#include <unistd.h>
#include <string.h>

#include "defines.h"
#include "defs.h"

#include "scjdefs.h"

LOCAL struct bigblock *putcall(struct bigblock *p);
LOCAL NODE *putmnmx(struct bigblock *p);
LOCAL NODE *putmem(bigptr, int, ftnint);
LOCAL NODE *putaddr(struct bigblock *, int);
LOCAL void putct1(bigptr, struct bigblock *, struct bigblock *, int *);
LOCAL int ncat(bigptr p);
LOCAL NODE *putcat(struct bigblock *, bigptr);
LOCAL NODE *putchcmp(struct bigblock *p);
LOCAL NODE *putcheq(struct bigblock *p);
LOCAL NODE *putcxcmp(struct bigblock *p);
LOCAL struct bigblock *putcx1(bigptr);
LOCAL NODE *putcxop(bigptr p);
LOCAL struct bigblock *putcxeq(struct bigblock *p);
LOCAL NODE *putpower(bigptr p);
LOCAL NODE *putop(bigptr p);
LOCAL NODE *putchop(bigptr p);
LOCAL struct bigblock *putch1(bigptr);
LOCAL struct bigblock *intdouble(struct bigblock *);

extern int ops2[];
extern int types2[];
static char *inproc;
static NODE *callval; /* to get return value right */
extern int negrel[];

#define XINT(z) 	ONEOF(z, MSKINT|MSKCHAR)
#define	P2TYPE(x)	(types2[(x)->vtype])
#define	P2OP(x)		(ops2[(x)->b_expr.opcode])

static void
sendp2(NODE *p)
{
	extern int thisline;

	p2tree(p);
	thisline = lineno;
	if (debugflag)
		fwalk(p, e2print, 0);
	pass2_compile(ipnode(p));
}

static NODE *
putassign(bigptr lp, bigptr rp)
{
	return putx(fixexpr(mkexpr(OPASSIGN, lp, rp)));
}


void
puthead(char *s)
{
	struct interpass_prolog *ipp = ckalloc(sizeof(struct interpass_prolog));
	int olbl, lbl1, lbl2;
	unsigned int i;

	if (s == NULL)
		return;
	if (inproc)
		fatal1("puthead %s in procedure", s);
	inproc = s;
	olbl = lastlabno;
	lbl1 = newlabel();
	lbl2 = newlabel();

	for (i = 0; i < NIPPREGS; i++)
		ipp->ipp_regs[i] = 0;	/* no regs used yet */
	ipp->ipp_autos = 0;		/* no autos used yet */
	ipp->ipp_name = copys(s);		/* function name */
	ipp->ipp_type = INT;		/* type not known yet? */
	ipp->ipp_vis = 1;		/* always visible */
	ipp->ip_tmpnum = 0; 		/* no temp nodes used in F77 yet */
	ipp->ip_lblnum = olbl;		/* # used labels so far */
	ipp->ipp_ip.ip_lbl = lbl1; 	/* first label, for optim */
	ipp->ipp_ip.type = IP_PROLOG;
	pass2_compile((struct interpass *)ipp);

}

/* It is necessary to precede each procedure with a "left bracket"
 * line that tells pass 2 how many register variables and how
 * much automatic space is required for the function.  This compiler
 * does not know how much automatic space is needed until the
 * entire procedure has been processed.  Therefore, "puthead"
 * is called at the begining to record the current location in textfile,
 * then to put out a placeholder left bracket line.  This procedure
 * repositions the file and rewrites that line, then puts the
 * file pointer back to the end of the file.
 */

void
putbracket()
{
	struct interpass_prolog *ipp = ckalloc(sizeof(struct interpass_prolog));
	unsigned int i;

	if (inproc == 0)
		fatal1("puteof outside procedure");
	for (i = 0; i < NIPPREGS; i++)
		ipp->ipp_regs[i] = 0;
	ipp->ipp_autos = autoleng;
	ipp->ipp_name = copys(inproc);
	ipp->ipp_type = INT; /* XXX should set the correct type */
	ipp->ipp_vis = 1;
	ipp->ip_tmpnum = 0;
	ipp->ip_lblnum = lastlabno;
	ipp->ipp_ip.ip_lbl = retlabel;
	ipp->ipp_ip.type = IP_EPILOG;
	printf("\t.text\n"); /* XXX */
	pass2_compile((struct interpass *)ipp);
	inproc = 0;
}



void
putrbrack(int k)
{
}


void
puteof()
{
}


/* put out code for if( ! p) goto l  */
void
putif(bigptr p, int l)
{
	NODE *p1;
	int k;

	if( ( k = (p = fixtype(p))->vtype) != TYLOGICAL) {
		if(k != TYERROR)
			err("non-logical expression in IF statement");
		frexpr(p);
	} else {
		p1 = putex1(p);
		if (p1->n_op == EQ && p1->n_right->n_op == ICON &&
		    p1->n_right->n_lval == 0 && logop(p1->n_left->n_op)) {
			/* created by OPOR */
			NODE *q = p1->n_left;
			q->n_op = negrel[q->n_op - EQ];
			nfree(p1->n_right);
			nfree(p1);
			p1 = q;
		}
		if (logop(p1->n_op) == 0)
			p1 = mkbinode(NE, p1, mklnode(ICON, 0, 0, INT), INT);
		if (p1->n_left->n_op == ICON) {
			/* change constants to right */
			NODE *p2 = p1->n_left;
			p1->n_left = p1->n_right;
			p1->n_right = p2;
			if (p1->n_op != EQ && p1->n_op != NE)
				p1->n_op = negrel[p1->n_op - EQ];
		}
		p1->n_op = negrel[p1->n_op - EQ];
		p1 = mkbinode(CBRANCH, p1, mklnode(ICON, l, 0, INT), INT);
		sendp2(p1);
	}
}

/* Arithmetic IF  */
void
prarif(bigptr p, int neg, int zer, int pos)
{
	bigptr x1 = fmktemp(p->vtype, NULL);

	putexpr(mkexpr(OPASSIGN, cpexpr(x1), p));
	putif(mkexpr(OPGE, cpexpr(x1), MKICON(0)), neg);
	putif(mkexpr(OPLE, x1, MKICON(0)), pos);
	putgoto(zer);
}

/* put out code for  goto l   */
void
putgoto(int label)
{
	NODE *p;

	p = mkunode(GOTO, mklnode(ICON, label, 0, INT), 0, INT);
	sendp2(p);
}


/* branch to address constant or integer variable */
void
putbranch(struct bigblock *q)
{
	NODE *p;

	p = mkunode(GOTO, putex1(q), 0, INT);
	sendp2(p);
}

/*
 * put out label l: in text segment
 */
void
putlabel(int label)
{
	struct interpass *ip = ckalloc(sizeof(struct interpass));

	ip->type = IP_DEFLAB;
	ip->lineno = lineno;
	ip->ip_lbl = label;
	pass2_compile(ip);
}


/*
 * Called from inner routines.  Generates a NODE tree and writes it out.
 */
void
putexpr(bigptr q)
{
	NODE *p;
	p = putex1(q);
	sendp2(p);
}



void
putcmgo(bigptr x, int nlab, struct labelblock *labels[])
{
	bigptr y;
	int i;

	if (!ISINT(x->vtype)) {
		execerr("computed goto index must be integer", NULL);
		return;
	}

	y = fmktemp(x->vtype, NULL);
	putexpr(mkexpr(OPASSIGN, cpexpr(y), x));
#ifdef notyet /* target-specific computed goto */
	vaxgoto(y, nlab, labels);
#else
	/*
	 * Primitive implementation, should use table here.
	 */
	for(i = 0 ; i < nlab ; ++i)
		putif(mkexpr(OPNE, cpexpr(y), MKICON(i+1)), labels[i]->labelno);
	frexpr(y);
#endif
}

/*
 * Convert a f77 tree statement to something that looks like a
 * pcc expression tree.
 */
NODE *
putx(bigptr q)
{
	struct bigblock *x1;
	NODE *p = NULL; /* XXX */
	int opc;
	int type, k;

#ifdef PCC_DEBUG
	if (tflag) {
		printf("putx %p\n", q);
		fprint(q, 0);
	}
#endif

	switch(q->tag) {
	case TERROR:
		ckfree(q);
		break;

	case TCONST:
		switch(type = q->vtype) {
			case TYLOGICAL:
				type = tyint;
			case TYLONG:
			case TYSHORT:
				p = mklnode(ICON, q->b_const.fconst.ci,
				    0, types2[type]);
				ckfree(q);
				break;

			case TYADDR:
				p = mklnode(ICON, 0, 0, types2[type]);
				p->n_name = copys(memname(STGCONST,
				    (int)q->b_const.fconst.ci));
				ckfree(q);
				break;

			default:
				p = putx(putconst(q));
				break;
			}
		break;

	case TEXPR:
		switch(opc = q->b_expr.opcode) {
			case OPCALL:
			case OPCCALL:
				if( ISCOMPLEX(q->vtype) )
					p = putcxop(q);
				else {
					putcall(q);
					p = callval;
				}
				break;

			case OPMIN:
			case OPMAX:
				p = putmnmx(q);
				break;

			case OPASSIGN:
				if (ISCOMPLEX(q->b_expr.leftp->vtype) ||
				    ISCOMPLEX(q->b_expr.rightp->vtype)) {
					frexpr(putcxeq(q));
				} else if (ISCHAR(q))
					p = putcheq(q);
				else
					goto putopp;
				break;

			case OPEQ:
			case OPNE:
				if (ISCOMPLEX(q->b_expr.leftp->vtype) ||
				    ISCOMPLEX(q->b_expr.rightp->vtype) ) {
					p = putcxcmp(q);
					break;
				}
			case OPLT:
			case OPLE:
			case OPGT:
			case OPGE:
				if(ISCHAR(q->b_expr.leftp))
					p = putchcmp(q);
				else
					goto putopp;
				break;

			case OPPOWER:
				p = putpower(q);
				break;

			case OPSTAR:
				/*   m * (2**k) -> m<<k   */
				if (XINT(q->b_expr.leftp->vtype) &&
				    ISICON(q->b_expr.rightp) &&
				    ((k = flog2(q->b_expr.rightp->b_const.fconst.ci))>0) ) {
					q->b_expr.opcode = OPLSHIFT;
					frexpr(q->b_expr.rightp);
					q->b_expr.rightp = MKICON(k);
					goto putopp;
				}

			case OPMOD:
				goto putopp;
			case OPPLUS:
			case OPMINUS:
			case OPSLASH:
			case OPNEG:
				if( ISCOMPLEX(q->vtype) )
					p = putcxop(q);
				else	
					goto putopp;
				break;

			case OPCONV:
				if( ISCOMPLEX(q->vtype) )
					p = putcxop(q);
				else if (ISCOMPLEX(q->b_expr.leftp->vtype)) {
					p = putx(mkconv(q->vtype,
					    realpart(putcx1(q->b_expr.leftp))));
					ckfree(q);
				} else
					goto putopp;
				break;

			case OPAND:
				/* Create logical AND */
				x1 = fmktemp(TYLOGICAL, NULL);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(0)));
				k = newlabel();
				putif(q->b_expr.leftp, k);
				putif(q->b_expr.rightp, k);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(1)));
				putlabel(k);
				p = putx(x1);
				break;

			case OPNOT: /* Logical NOT */
				x1 = fmktemp(TYLOGICAL, NULL);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(1)));
				k = newlabel();
				putif(q->b_expr.leftp, k);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(0)));
				putlabel(k);
				p = putx(x1);
				break;

			case OPOR: /* Create logical OR */
				x1 = fmktemp(TYLOGICAL, NULL);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(1)));
				k = newlabel();
				putif(mkexpr(OPEQ, q->b_expr.leftp,
				    mklogcon(0)), k);
				putif(mkexpr(OPEQ, q->b_expr.rightp,
				    mklogcon(0)), k);
				putexpr(mkexpr(OPASSIGN, cpexpr(x1),
				    mklogcon(0)));
				putlabel(k);
				p = putx(x1);
				break;

			case OPCOMMA:
				for (x1 = q; x1->b_expr.opcode == OPCOMMA; 
				    x1 = x1->b_expr.leftp)
					putexpr(x1->b_expr.rightp);
				p = putx(x1);
				break;

			case OPEQV:
			case OPNEQV:
			case OPADDR:
			case OPBITOR:
			case OPBITAND:
			case OPBITXOR:
			case OPBITNOT:
			case OPLSHIFT:
			case OPRSHIFT:
		putopp:
				p = putop(q);
				break;

			default:
				fatal1("putx: invalid opcode %d", opc);
			}
		break;

	case TADDR:
		p = putaddr(q, YES);
		break;

	default:
		fatal1("putx: impossible tag %d", q->tag);
	}
	return p;
}

LOCAL NODE *
putop(bigptr q)
{
	NODE *p;
	int k;
	bigptr lp, tp;
	int pt, lt;

#ifdef PCC_DEBUG
	if (tflag) {
		printf("putop %p\n", q);
		fprint(q, 0);
	}
#endif
	switch(q->b_expr.opcode) { /* check for special cases and rewrite */
	case OPCONV:
		pt = q->vtype;
		lp = q->b_expr.leftp;
		lt = lp->vtype;
		while(q->tag==TEXPR && q->b_expr.opcode==OPCONV &&
		     ((ISREAL(pt)&&ISREAL(lt)) ||
			(XINT(pt)&&(ONEOF(lt,MSKINT|MSKADDR))) )) {
			if(lp->tag != TEXPR) {
				if(pt==TYINT && lt==TYLONG)
					break;
				if(lt==TYINT && pt==TYLONG)
					break;
			}
			ckfree(q);
			q = lp;
			pt = lt;
			lp = q->b_expr.leftp;
			lt = lp->vtype;
		}
		if(q->tag==TEXPR && q->b_expr.opcode==OPCONV)
			break;
		p = putx(q);
		return p;

	case OPADDR:
		lp = q->b_expr.leftp;
		if(lp->tag != TADDR) {
			tp = fmktemp(lp->vtype, lp->vleng);
			p = putx(mkexpr(OPASSIGN, cpexpr(tp), lp));
			sendp2(p);
			lp = tp;
		}
		p = putaddr(lp, NO);
		ckfree(q);
		return p;
	}

	if ((k = ops2[q->b_expr.opcode]) <= 0)
		fatal1("putop: invalid opcode %d (%d)", q->b_expr.opcode, k);
	p = putx(q->b_expr.leftp);
	if(q->b_expr.rightp)
		p = mkbinode(k, p, putx(q->b_expr.rightp), types2[q->vtype]);
	else
		p = mkunode(k, p, 0, types2[q->vtype]);

	if(q->vleng)
		frexpr(q->vleng);
	ckfree(q);
	return p;
}

/*
 * Put return values into correct register.
 */
void
putforce(int t, bigptr p)
{
	NODE *p1;

	p = mkconv(t, fixtype(p));
	p1 = putx(p);
	p1 = mkunode(FORCE, p1, 0, 
		(t==TYSHORT ? SHORT : (t==TYLONG ? LONG : LDOUBLE)));
	sendp2(p1);
}

LOCAL NODE *
putpower(bigptr p)
{
	NODE *p3;
	bigptr base;
	struct bigblock *t1, *t2;
	ftnint k = 0; /* XXX gcc */
	int type;

	if(!ISICON(p->b_expr.rightp) ||
	    (k = p->b_expr.rightp->b_const.fconst.ci)<2)
		fatal("putpower: bad call");
	base = p->b_expr.leftp;
	type = base->vtype;
	t1 = fmktemp(type, NULL);
	t2 = NULL;
	p3 = putassign(cpexpr(t1), cpexpr(base) );
	sendp2(p3);

	for( ; (k&1)==0 && k>2 ; k>>=1 ) {
		p3 = putassign(cpexpr(t1),
		    mkexpr(OPSTAR, cpexpr(t1), cpexpr(t1)));
		sendp2(p3);
	}

	if(k == 2)
		p3 = putx(mkexpr(OPSTAR, cpexpr(t1), cpexpr(t1)));
	else {
		t2 = fmktemp(type, NULL);
		p3 = putassign(cpexpr(t2), cpexpr(t1));
		sendp2(p3);
	
		for(k>>=1 ; k>1 ; k>>=1) {
			p3 = putassign(cpexpr(t1),
			    mkexpr(OPSTAR, cpexpr(t1), cpexpr(t1)));
			sendp2(p3);
			if(k & 1) {
				p3 = putassign(cpexpr(t2),
				    mkexpr(OPSTAR, cpexpr(t2), cpexpr(t1)));
				sendp2(p3);
			}
		}
		p3 = putx( mkexpr(OPSTAR, cpexpr(t2),
		mkexpr(OPSTAR, cpexpr(t1), cpexpr(t1)) ));
	}
	frexpr(t1);
	if(t2)
		frexpr(t2);
	frexpr(p);
	return p3;
}

LOCAL struct bigblock *
intdouble(struct bigblock *p)
{
	struct bigblock *t;

	t = fmktemp(TYDREAL, NULL);

	sendp2(putassign(cpexpr(t), p));
	return(t);
}

LOCAL struct bigblock *
putcxeq(struct bigblock *q)
{
	struct bigblock *lp, *rp;

	lp = putcx1(q->b_expr.leftp);
	rp = putcx1(q->b_expr.rightp);
	sendp2(putassign(realpart(lp), realpart(rp)));
	if( ISCOMPLEX(q->vtype) ) {
		sendp2(putassign(imagpart(lp), imagpart(rp)));
	}
	frexpr(rp);
	ckfree(q);
	return(lp);
}



LOCAL NODE *
putcxop(bigptr q)
{
	NODE *p;

	p = putaddr(putcx1(q), NO);
	return p;
}

LOCAL struct bigblock *
putcx1(bigptr qq)
{
	struct bigblock *q, *lp, *rp;
	register struct bigblock *resp;
	NODE *p;
	int opcode;
	int ltype, rtype;

	ltype = rtype = 0; /* XXX gcc */
	if(qq == NULL)
		return(NULL);

	switch(qq->tag) {
	case TCONST:
		if( ISCOMPLEX(qq->vtype) )
			qq = putconst(qq);
		return( qq );

	case TADDR:
		if( ! addressable(qq) ) {
			resp = fmktemp(tyint, NULL);
			p = putassign( cpexpr(resp), qq->b_addr.memoffset );
			sendp2(p);
			qq->b_addr.memoffset = resp;
		}
		return( qq );

	case TEXPR:
		if( ISCOMPLEX(qq->vtype) )
			break;
		resp = fmktemp(TYDREAL, NO);
		p = putassign( cpexpr(resp), qq);
		sendp2(p);
		return(resp);

	default:
		fatal1("putcx1: bad tag %d", qq->tag);
	}

	opcode = qq->b_expr.opcode;
	if(opcode==OPCALL || opcode==OPCCALL) {
		q = putcall(qq);
		sendp2(callval);
		return(q);
	} else if(opcode == OPASSIGN) {
		return( putcxeq(qq) );
	}

	resp = fmktemp(qq->vtype, NULL);
	if((lp = putcx1(qq->b_expr.leftp) ))
		ltype = lp->vtype;
	if((rp = putcx1(qq->b_expr.rightp) ))
		rtype = rp->vtype;

	switch(opcode) {
	case OPCOMMA:
		frexpr(resp);
		resp = rp;
		rp = NULL;
		break;

	case OPNEG:
		p = putassign(realpart(resp),
		    mkexpr(OPNEG, realpart(lp), NULL));
		sendp2(p);
		p = putassign(imagpart(resp),
		    mkexpr(OPNEG, imagpart(lp), NULL));
		sendp2(p);
		break;

	case OPPLUS:
	case OPMINUS:
		p = putassign( realpart(resp),
		    mkexpr(opcode, realpart(lp), realpart(rp) ));
		sendp2(p);
		if(rtype < TYCOMPLEX) {
			p = putassign(imagpart(resp), imagpart(lp) );
		} else if(ltype < TYCOMPLEX) {
			if(opcode == OPPLUS)
				p = putassign( imagpart(resp), imagpart(rp) );
			else
				p = putassign( imagpart(resp),
				    mkexpr(OPNEG, imagpart(rp), NULL) );
		} else
			p = putassign( imagpart(resp),
			    mkexpr(opcode, imagpart(lp), imagpart(rp) ));
		sendp2(p);
		break;

	case OPSTAR:
		if(ltype < TYCOMPLEX) {
			if( ISINT(ltype) )
				lp = intdouble(lp);
			p = putassign( realpart(resp),
			    mkexpr(OPSTAR, cpexpr(lp), realpart(rp) ));
			sendp2(p);
			p = putassign( imagpart(resp),
			    mkexpr(OPSTAR, cpexpr(lp), imagpart(rp) ));
		} else if(rtype < TYCOMPLEX) {
			if( ISINT(rtype) )
				rp = intdouble(rp);
			p = putassign( realpart(resp),
			    mkexpr(OPSTAR, cpexpr(rp), realpart(lp) ));
			sendp2(p);
			p = putassign( imagpart(resp),
			    mkexpr(OPSTAR, cpexpr(rp), imagpart(lp) ));
		} else {
			p = putassign( realpart(resp), mkexpr(OPMINUS,
				mkexpr(OPSTAR, realpart(lp), realpart(rp)),
				mkexpr(OPSTAR, imagpart(lp), imagpart(rp)) ));
			sendp2(p);
			p = putassign( imagpart(resp), mkexpr(OPPLUS,
				mkexpr(OPSTAR, realpart(lp), imagpart(rp)),
				mkexpr(OPSTAR, imagpart(lp), realpart(rp)) ));
		}
		sendp2(p);
		break;

	case OPSLASH:
		/* fixexpr has already replaced all divisions
		 * by a complex by a function call
		 */
		if( ISINT(rtype) )
			rp = intdouble(rp);
		p = putassign( realpart(resp),
		    mkexpr(OPSLASH, realpart(lp), cpexpr(rp)) );
		sendp2(p);
		p = putassign( imagpart(resp),
		    mkexpr(OPSLASH, imagpart(lp), cpexpr(rp)) );
		sendp2(p);
		break;

	case OPCONV:
		p = putassign( realpart(resp), realpart(lp) );
		if( ISCOMPLEX(lp->vtype) )
			q = imagpart(lp);
		else if(rp != NULL)
			q = realpart(rp);
		else
			q = mkrealcon(TYDREAL, 0.0);
		sendp2(p);
		p = putassign( imagpart(resp), q);
		sendp2(p);
		break;

	default:
		fatal1("putcx1 of invalid opcode %d", opcode);
	}

	frexpr(lp);
	frexpr(rp);
	ckfree(qq);
	return(resp);
}


LOCAL NODE *
putcxcmp(struct bigblock *p)
{
	NODE *p1;
	int opcode;
	struct bigblock *lp, *rp;
	struct bigblock *q;

	opcode = p->b_expr.opcode;
	lp = putcx1(p->b_expr.leftp);
	rp = putcx1(p->b_expr.rightp);

	q = mkexpr( opcode==OPEQ ? OPAND : OPOR ,
	    mkexpr(opcode, realpart(lp), realpart(rp)),
	    mkexpr(opcode, imagpart(lp), imagpart(rp)) );
	p1 = putx( fixexpr(q) );

	ckfree(lp);
	ckfree(rp);
	ckfree(p);
	return p1;
}

LOCAL struct bigblock *
putch1(bigptr p)
{
	struct bigblock *t;

	switch(p->tag) {
	case TCONST:
		return( putconst(p) );

	case TADDR:
		return(p);

	case TEXPR:
		switch(p->b_expr.opcode) {
			case OPCALL:
			case OPCCALL:
				t = putcall(p);
				sendp2(callval);
				break;

			case OPCONCAT:
				t = fmktemp(TYCHAR, cpexpr(p->vleng) );
				sendp2(putcat( cpexpr(t), p ));
				break;

			case OPCONV:
				if(!ISICON(p->vleng) ||
				    p->vleng->b_const.fconst.ci!=1
				   || ! XINT(p->b_expr.leftp->vtype) )
					fatal("putch1: bad character conversion");
				t = fmktemp(TYCHAR, MKICON(1) );
				sendp2(putassign( cpexpr(t), p));
				break;
			default:
				fatal1("putch1: invalid opcode %d", p->b_expr.opcode);
				t = NULL; /* XXX gcc */
			}
		return(t);

	default:
		fatal1("putch1: bad tag %d", p->tag);
	}
/* NOTREACHED */
return NULL; /* XXX gcc */
}




LOCAL NODE *
putchop(bigptr p)
{
	NODE *p1;

	p1 = putaddr( putch1(p) , NO );
	return p1;
}


/*
 * Assign a character to another.
 */
LOCAL NODE *
putcheq(struct bigblock *p)
{
	NODE *p1, *p2, *p3;

	if( p->b_expr.rightp->tag==TEXPR &&
	    p->b_expr.rightp->b_expr.opcode==OPCONCAT )
		p3 = putcat(p->b_expr.leftp, p->b_expr.rightp);
	else if( ISONE(p->b_expr.leftp->vleng) &&
	    ISONE(p->b_expr.rightp->vleng) ) {
		p1 = putaddr( putch1(p->b_expr.leftp) , YES );
		p2 = putaddr( putch1(p->b_expr.rightp) , YES );
		p3 = mkbinode(ASSIGN, p1, p2, CHAR);
	} else
		p3 = putx(call2(TYINT, "s_copy",
		    p->b_expr.leftp, p->b_expr.rightp));

	frexpr(p->vleng);
	ckfree(p);
	return p3;
}



/*
 * Compare character(s) code.
 */
LOCAL NODE *
putchcmp(struct bigblock *p)
{
	NODE *p1, *p2, *p3;

	if(ISONE(p->b_expr.leftp->vleng) && ISONE(p->b_expr.rightp->vleng) ) {
		p1 = putaddr( putch1(p->b_expr.leftp) , YES );
		p2 = putaddr( putch1(p->b_expr.rightp) , YES );
		p3 = mkbinode(ops2[p->b_expr.opcode], p1, p2, CHAR);
		ckfree(p);
	} else {
		p->b_expr.leftp = call2(TYINT,"s_cmp",
		    p->b_expr.leftp, p->b_expr.rightp);
		p->b_expr.rightp = MKICON(0);
		p3 = putop(p);
	}
	return p3;
}

LOCAL NODE *
putcat(bigptr lhs, bigptr rhs)
{
	NODE *p3;
	int n;
	struct bigblock *lp, *cp;

	n = ncat(rhs);
	lp = mktmpn(n, TYLENG, NULL);
	cp = mktmpn(n, TYADDR, NULL);

	n = 0;
	putct1(rhs, lp, cp, &n);

	p3 = putx( call4(TYSUBR, "s_cat", lhs, cp, lp, MKICON(n) ) );
	return p3;
}

LOCAL int
ncat(bigptr p)
{
	if(p->tag==TEXPR && p->b_expr.opcode==OPCONCAT)
		return( ncat(p->b_expr.leftp) + ncat(p->b_expr.rightp) );
	else
		return(1);
}

LOCAL void
putct1(bigptr q, bigptr lp, bigptr cp, int *ip)
{
	NODE *p;
	int i;
	struct bigblock *lp1, *cp1;

	if(q->tag==TEXPR && q->b_expr.opcode==OPCONCAT) {
		putct1(q->b_expr.leftp, lp, cp, ip);
		putct1(q->b_expr.rightp, lp, cp , ip);
		frexpr(q->vleng);
		ckfree(q);
	} else {
		i = (*ip)++;
		lp1 = cpexpr(lp);
		lp1->b_addr.memoffset =
		    mkexpr(OPPLUS, lp1->b_addr.memoffset, MKICON(i*FSZLENG));
		cp1 = cpexpr(cp);
		cp1->b_addr.memoffset =
		    mkexpr(OPPLUS, cp1->b_addr.memoffset, MKICON(i*FSZADDR));
		p = putassign( lp1, cpexpr(q->vleng) );
		sendp2(p);
		p = putassign( cp1, addrof(putch1(q)) );
		sendp2(p);
	}
}

/*
 * Create a tree that can later be converted to an OREG.
 */
static NODE *
oregtree(int off, int reg, int type)
{
	NODE *p, *q;

	p = mklnode(REG, 0, reg, INCREF(type));
	q = mklnode(ICON, off, 0, INT);
	return mkunode(UMUL, mkbinode(PLUS, p, q, INCREF(type)), 0, type);
}

static NODE *
putaddr(bigptr q, int indir)
{
	int type, type2, funct;
	NODE *p, *p1, *p2;
	ftnint offset;
	bigptr offp;

	p = p1 = p2 = NULL; /* XXX */

	type = q->vtype;
	type2 = types2[type];
	funct = (q->vclass==CLPROC ? FTN<<TSHIFT : 0);

	offp = (q->b_addr.memoffset ? cpexpr(q->b_addr.memoffset) : NULL);

	offset = simoffset(&offp);
	if(offp)
		offp = mkconv(TYINT, offp);

	switch(q->vstg) {
	case STGAUTO:
		if(indir && !offp) {
			p = oregtree(offset, AUTOREG, type2);
			break;
		}

		if(!indir && !offp && !offset) {
			p = mklnode(REG, 0, AUTOREG, INCREF(type2));
			break;
		}

		p = mklnode(REG, 0, AUTOREG, INCREF(type2));
		if(offp) {
			p1 = putx(offp);
			if(offset)
				p2 = mklnode(ICON, offset, 0, INT);
		} else
			p1 = mklnode(ICON, offset, 0, INT);
		if (offp && offset)
			p1 = mkbinode(PLUS, p1, p2, INCREF(type2));
		p = mkbinode(PLUS, p, p1, INCREF(type2));
		if (indir)
			p = mkunode(UMUL, p, 0, type2);
		break;

	case STGARG:
		p = oregtree(ARGOFFSET + (ftnint)(q->b_addr.memno),
		    ARGREG, INCREF(type2)|funct);

		if (offp)
			p1 = putx(offp);
		if (offset)
			p2 = mklnode(ICON, offset, 0, INT);
		if (offp && offset)
			p1 = mkbinode(PLUS, p1, p2, INCREF(type2));
		else if (offset)
			p1 = p2;
		if (offp || offset)
			p = mkbinode(PLUS, p, p1, INCREF(type2));
		if (indir)
			p = mkunode(UMUL, p, 0, type2);
		break;

	case STGLENG:
		if(indir) {
			p = oregtree(ARGOFFSET + (ftnint)(q->b_addr.memno),
			    ARGREG, INCREF(type2)|funct);
		} else	{
			fatal1("faddrnode: STGLENG: fixme!");
#if 0
			p2op(P2PLUS, types2[TYLENG] | P2PTR );
			p2reg(ARGREG, types2[TYLENG] | P2PTR );
			p2icon( ARGOFFSET +
				(ftnint) (FUDGEOFFSET*p->b_addr.memno), P2INT);
#endif
		}
		break;


	case STGBSS:
	case STGINIT:
	case STGEXT:
	case STGCOMMON:
	case STGEQUIV:
	case STGCONST:
		if(offp) {
			p1 = putx(offp);
			p2 = putmem(q, ICON, offset);
			p = mkbinode(PLUS, p1, p2, INCREF(type2));
			if(indir)
				p = mkunode(UMUL, p, 0, type2);
		} else
			p = putmem(q, (indir ? NAME : ICON), offset);
		break;

	case STGREG:
		if(indir)
			p = mklnode(REG, 0, q->b_addr.memno, type2);
		else
			fatal("attempt to take address of a register");
		break;

	default:
		fatal1("putaddr: invalid vstg %d", q->vstg);
	}
	frexpr(q);
	return p;
}

NODE *
putmem(bigptr q, int class, ftnint offset)
{
	NODE *p;
	int type2;

	type2 = types2[q->vtype];
	if(q->vclass == CLPROC)
		type2 |= (FTN<<TSHIFT);
	if (class == ICON)
		type2 |= PTR;
	p = mklnode(class, offset, 0, type2);
	p->n_name = copys(memname(q->vstg, q->b_addr.memno));
	return p;
}



LOCAL struct bigblock *
putcall(struct bigblock *qq)
{
	chainp arglist, charsp, cp;
	int n, first;
	struct bigblock *t;
	struct bigblock *q;
	struct bigblock *fval;
	int type, type2, ctype, indir;
	NODE *lp, *p1, *p2;

	lp = p2 = NULL; /* XXX */

	type2 = types2[type = qq->vtype];
	charsp = NULL;
	indir =  (qq->b_expr.opcode == OPCCALL);
	n = 0;
	first = YES;

	if(qq->b_expr.rightp) {
		arglist = qq->b_expr.rightp->b_list.listp;
		ckfree(qq->b_expr.rightp);
	} else
		arglist = NULL;

	for(cp = arglist ; cp ; cp = cp->chain.nextp)
		if(indir) {
			++n;
		} else {
			q = cp->chain.datap;
			if(q->tag == TCONST)
				cp->chain.datap = q = putconst(q);
			if( ISCHAR(q) ) {
				charsp = hookup(charsp,
				    mkchain(cpexpr(q->vleng), 0) );
				n += 2;
			} else if(q->vclass == CLPROC) {
				charsp = hookup(charsp,
				    mkchain( MKICON(0) , 0));
				n += 2;
			} else
				n += 1;
		}

	if(type == TYCHAR) {
		if( ISICON(qq->vleng) ) {
			fval = fmktemp(TYCHAR, qq->vleng);
			n += 2;
		} else {
			err("adjustable character function");
			return NULL;
		}
	} else if(ISCOMPLEX(type)) {
		fval = fmktemp(type, NULL);
		n += 1;
	} else
		fval = NULL;

	ctype = (fval ? P2INT : type2);
	p1 = putaddr(qq->b_expr.leftp, NO);

	if(fval) {
		first = NO;
		lp = putaddr( cpexpr(fval), NO);
		if(type==TYCHAR)
			lp = mkbinode(CM, lp, putx(cpexpr(qq->vleng)), INT);
	}

	for(cp = arglist ; cp ; cp = cp->chain.nextp) {
		q = cp->chain.datap;
		if(q->tag==TADDR && (indir || q->vstg!=STGREG) )
			p2 = putaddr(q, indir && q->vtype!=TYCHAR);
		else if( ISCOMPLEX(q->vtype) )
			p2 = putcxop(q);
		else if (ISCHAR(q) ) {
			p2 = putchop(q);
		} else if( ! ISERROR(q) ) {
			if(indir)
				p2 = putx(q);
			else	{
				t = fmktemp(q->vtype, q->vleng);
				p2 = putassign( cpexpr(t), q );
				sendp2(p2);
				p2 = putaddr(t, NO);
			}
		}
		if(first) {
			first = NO;
			lp = p2;
		} else
			lp = mkbinode(CM, lp, p2, INT);
	}

	if(arglist)
		frchain(&arglist);
	for(cp = charsp ; cp ; cp = cp->chain.nextp) {
		p2 = putx( mkconv(TYLENG,cp->chain.datap) );
		lp = mkbinode(CM, lp, p2, INT);
	}
	frchain(&charsp);
	if (n > 0)
		callval = mkbinode(CALL, p1, lp, ctype);
	else
		callval = mkunode(UCALL, p1, 0, ctype);
	ckfree(qq);
	return(fval);
}

/*
 * Write out code to do min/max calculations.
 * Note that these operators may have multiple arguments in fortran.
 */
LOCAL NODE *
putmnmx(struct bigblock *p)
{
	NODE *n1, *n2;
	int op, type, lab;
	chainp p0, p1;
	struct bigblock *tp;

	type = p->vtype;
	op = (p->b_expr.opcode==OPMIN ? LT : GT );
	p0 = p->b_expr.leftp->b_list.listp;
	ckfree(p->b_expr.leftp);
	ckfree(p);

	/*
	 * Store first value in a temporary, then compare it with 
	 * each following value and save that if needed.
	 */
	tp = fmktemp(type, NULL);
	sendp2(putassign(cpexpr(tp), p0->chain.datap));

	for(p1 = p0->chain.nextp ; p1 ; p1 = p1->chain.nextp) {
		n1 = putx(cpexpr(tp));
		n2 = putx(cpexpr(p1->chain.datap));
		lab = newlabel();
		sendp2(mkbinode(CBRANCH, mkbinode(op, n1, n2, INT), 
		    mklnode(ICON, lab, 0, INT), INT));
		sendp2(putassign(cpexpr(tp), p1->chain.datap));
		putlabel(lab);
	}
	return putx(tp);
}

ftnint
simoffset(bigptr *p0)
{
	ftnint offset, prod;
	bigptr p, lp, rp;

	offset = 0;
	p = *p0;
	if(p == NULL)
		return(0);

	if( ! ISINT(p->vtype) )
		return(0);

	if(p->tag==TEXPR && p->b_expr.opcode==OPSTAR) {
		lp = p->b_expr.leftp;
		rp = p->b_expr.rightp;
		if(ISICON(rp) && lp->tag==TEXPR &&
		    lp->b_expr.opcode==OPPLUS && ISICON(lp->b_expr.rightp)) {
			p->b_expr.opcode = OPPLUS;
			lp->b_expr.opcode = OPSTAR;
			prod = rp->b_const.fconst.ci *
			    lp->b_expr.rightp->b_const.fconst.ci;
			lp->b_expr.rightp->b_const.fconst.ci =
			    rp->b_const.fconst.ci;
			rp->b_const.fconst.ci = prod;
		}
	}

	if(p->tag==TEXPR && p->b_expr.opcode==OPPLUS &&
	    ISICON(p->b_expr.rightp)) {
		rp = p->b_expr.rightp;
		lp = p->b_expr.leftp;
		offset += rp->b_const.fconst.ci;
		frexpr(rp);
		ckfree(p);
		*p0 = lp;
	}

	if(p->tag == TCONST) {
		offset += p->b_const.fconst.ci;
		frexpr(p);
		*p0 = NULL;
	}

	return(offset);
}

/*
 * F77 uses ckalloc() (malloc) for NODEs.
 */
NODE *
talloc()
{
	NODE *p = ckalloc(sizeof(NODE));
	p->n_name = "";
	return p;
}

#ifdef PCC_DEBUG
static char *tagnam[] = {
 "NONE", "NAME", "CONST", "EXPR", "ADDR", "PRIM", "LIST", "IMPLDO", "ERROR",
};
static char *typnam[] = {
 "unknown", "addr", "short", "long", "real", "dreal", "complex", "dcomplex",
 "logical", "char", "subr", "error",
};
static char *classnam[] = {
 "unknown", "param", "var", "entry", "main", "block", "proc",
};
static char *stgnam[] = {
 "unknown", "arg", "auto", "bss", "init", "const", "intr", "stfunct",
 "common", "equiv", "reg", "leng",
};


/*
 * Print out a f77 tree, for diagnostic purposes.
 */
void
fprint(bigptr p, int indx)
{
	extern char *ops[];
	int x = indx;
	bigptr lp, rp;
	struct chain *bp;

	if (p == NULL)
		return;

	while (x >= 2) {
		putchar('\t');
		x -= 2;
	}
	if (x--)
		printf("    " );
	printf("%p) %s, ", p, tagnam[p->tag]);
	if (p->vtype)
		printf("type=%s, ", typnam[p->vtype]);
	if (p->vclass)
		printf("class=%s, ", classnam[p->vclass]);
	if (p->vstg)
		printf("stg=%s, ", stgnam[p->vstg]);

	lp = rp = NULL;
	switch (p->tag) {
	case TEXPR:
		printf("OP %s\n", ops[p->b_expr.opcode]);
		lp = p->b_expr.leftp;
		rp = p->b_expr.rightp;
		break;
	case TADDR:
		printf("memno=%d\n", p->b_addr.memno);
		lp = p->vleng;
		rp = p->b_addr.memoffset;
		break;
	case TCONST:
		switch (p->vtype) {
		case TYSHORT:
		case TYLONG:
		case TYLOGICAL:
		case TYADDR:
			printf("val=%ld\n", p->b_const.fconst.ci);
			break;
		case TYCHAR:
			lp = p->vleng;
			printf("\n");
			break;
		}
		break;
	case TPRIM:
		lp = p->b_prim.namep;
		rp = p->b_prim.argsp;
		printf("fcharp=%p, lcharp=%p\n", p->b_prim.fcharp, p->b_prim.lcharp);
		break;
	case TNAME:
		printf("name=%s\n", p->b_name.varname);
		break;
	case TLIST:
		printf("\n");
		for (bp = &p->b_list.listp->chain; bp; bp = &bp->nextp->chain)
			fprint(bp->datap, indx+1);
		break;
	default:
		printf("\n");
	}

	fprint(lp, indx+1);
	fprint(rp, indx+1);
}
#endif
