/*	$Id: data.c,v 1.15 2008/05/11 15:28:03 ragge Exp $	*/
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

#include "defines.h"
#include "defs.h"

#if 1 /* RAGGE */
extern FILE *initfile;
#endif

/* ROUTINES CALLED DURING DATA STATEMENT PROCESSING */
LOCAL void setdata(struct bigblock *, struct bigblock *, ftnint, ftnint);

static char datafmt[] = "%s\t%05ld\t%05ld\t%d" ;

/* another initializer, called from parser */
void
dataval(repp, valp)
register struct bigblock *repp, *valp;
{
int i, nrep;
ftnint elen, vlen;
register struct bigblock *p;

if(repp == NULL)
	nrep = 1;
else if (ISICON(repp) && repp->b_const.fconst.ci >= 0)
	nrep = repp->b_const.fconst.ci;
else
	{
	err("invalid repetition count in DATA statement");
	frexpr(repp);
	goto ret;
	}
frexpr(repp);

if( ! ISCONST(valp) )
	{
	err("non-constant initializer");
	goto ret;
	}

if(toomanyinit) goto ret;
for(i = 0 ; i < nrep ; ++i)
	{
	p = nextdata(&elen, &vlen);
	if(p == NULL)
		{
		err("too many initializers");
		toomanyinit = YES;
		goto ret;
		}
	setdata(p, valp, elen, vlen);
	frexpr(p);
	}

ret:
	frexpr(valp);
}


struct bigblock *nextdata(elenp, vlenp)
ftnint *elenp, *vlenp;
{
register struct bigblock *ip;
struct bigblock *pp;
register struct bigblock *np;
register chainp rp;
bigptr p;
bigptr neltp;
register bigptr q;
int skip;
ftnint off;

while(curdtp)
	{
	p = curdtp->chain.datap;
	if(p->tag == TIMPLDO)
		{
		ip = p;
		if(ip->b_impldo.implb==NULL || ip->b_impldo.impub==NULL || ip->b_impldo.varnp==NULL)
			fatal1("bad impldoblock 0%o", ip);
		if(ip->isactive)
			ip->b_impldo.varvp->b_const.fconst.ci += ip->b_impldo.impdiff;
		else
			{
			q = fixtype(cpexpr(ip->b_impldo.implb));
			if( ! ISICON(q) )
				goto doerr;
			ip->b_impldo.varvp = q;

			if(ip->b_impldo.impstep)
				{
				q = fixtype(cpexpr(ip->b_impldo.impstep));
				if( ! ISICON(q) )
					goto doerr;
				ip->b_impldo.impdiff = q->b_const.fconst.ci;
				frexpr(q);
				}
			else
				ip->b_impldo.impdiff = 1;

			q = fixtype(cpexpr(ip->b_impldo.impub));
			if(! ISICON(q))
				goto doerr;
			ip->b_impldo.implim = q->b_const.fconst.ci;
			frexpr(q);

			ip->isactive = YES;
			rp = ALLOC(rplblock);
			rp->rplblock.nextp = rpllist;
			rpllist = rp;
			rp->rplblock.rplnp = ip->b_impldo.varnp;
			rp->rplblock.rplvp = ip->b_impldo.varvp;
			rp->rplblock.rpltag = TCONST;
			}

		if( (ip->b_impldo.impdiff>0 &&
		 (ip->b_impldo.varvp->b_const.fconst.ci <= ip->b_impldo.implim))
		 || (ip->b_impldo.impdiff<0 &&
		(ip->b_impldo.varvp->b_const.fconst.ci >= ip->b_impldo.implim)))
			{ /* start new loop */
			curdtp = ip->b_impldo.datalist;
			goto next;
			}

		/* clean up loop */

		popstack(&rpllist);

		frexpr(ip->b_impldo.varvp);
		ip->isactive = NO;
		curdtp = curdtp->chain.nextp;
		goto next;
		}

	pp = p;
	np = pp->b_prim.namep;
	skip = YES;

	if(p->b_prim.argsp==NULL && np->b_name.vdim!=NULL)
		{   /* array initialization */
		q = mkaddr(np);
		off = typesize[np->vtype] * curdtelt;
		if(np->vtype == TYCHAR)
			off *= np->vleng->b_const.fconst.ci;
		q->b_addr.memoffset = mkexpr(OPPLUS, q->b_addr.memoffset, mkintcon(off) );
		if( (neltp = np->b_name.vdim->nelt) && ISCONST(neltp))
			{
			if(++curdtelt < neltp->b_const.fconst.ci)
				skip = NO;
			}
		else
			err("attempt to initialize adjustable array");
		}
	else
		q = mklhs( cpexpr(pp) );
	if(skip)
		{
		curdtp = curdtp->chain.nextp;
		curdtelt = 0;
		}
	if(q->vtype == TYCHAR)
		if(ISICON(q->vleng))
			*elenp = q->vleng->b_const.fconst.ci;
		else	{
			err("initialization of string of nonconstant length");
			continue;
			}
	else	*elenp = typesize[q->vtype];

	if(np->vstg == STGCOMMON)
		*vlenp = extsymtab[np->b_name.vardesc.varno].maxleng;
	else if(np->vstg == STGEQUIV)
		*vlenp = eqvclass[np->b_name.vardesc.varno].eqvleng;
	else	{
		*vlenp =  (np->vtype==TYCHAR ?
				np->vleng->b_const.fconst.ci : typesize[np->vtype]);
		if(np->b_name.vdim)
			*vlenp *= np->b_name.vdim->nelt->b_const.fconst.ci;
		}
	return(q);

doerr:
		err("nonconstant implied DO parameter");
		frexpr(q);
		curdtp = curdtp->chain.nextp;

next:	curdtelt = 0;
	}

return(NULL);
}






LOCAL void setdata(varp, valp, elen, vlen)
struct bigblock *varp;
ftnint elen, vlen;
struct bigblock *valp;
{
union constant con;
int i, k;
int stg, type, valtype;
ftnint offset;
register char *s, *t;
static char varname[XL+2];

/* output form of name is padded with blanks and preceded
   with a storage class digit
*/

stg = varp->vstg;
varname[0] = (stg==STGCOMMON ? '2' : (stg==STGEQUIV ? '1' : '0') );
s = memname(stg, varp->b_addr.memno);
for(t = varname+1 ; *s ; )
	*t++ = *s++;
while(t < varname+XL+1)
	*t++ = ' ';
varname[XL+1] = '\0';

offset = varp->b_addr.memoffset->b_const.fconst.ci;
type = varp->vtype;
valtype = valp->vtype;
if(type!=TYCHAR && valtype==TYCHAR)
	{
	if(! ftn66flag)
		warn("non-character datum initialized with character string");
	varp->vleng = MKICON(typesize[type]);
	varp->vtype = type = TYCHAR;
	}
else if( (type==TYCHAR && valtype!=TYCHAR) ||
	 (cktype(OPASSIGN,type,valtype) == TYERROR) )
	{
	err("incompatible types in initialization");
	return;
	}
if(type != TYCHAR) {
	if(valtype == TYUNKNOWN)
		con.ci = valp->b_const.fconst.ci;
	else	consconv(type, &con, valtype, &valp->b_const.fconst);
}

k = 1;
switch(type)
	{
	case TYLOGICAL:
		type = tylogical;
	case TYSHORT:
	case TYLONG:
		fprintf(initfile, datafmt, varname, offset, vlen, type);
		prconi(initfile, type, con.ci);
		break;

	case TYCOMPLEX:
		k = 2;
		type = TYREAL;
	case TYREAL:
		goto flpt;

	case TYDCOMPLEX:
		k = 2;
		type = TYDREAL;
	case TYDREAL:
	flpt:

		for(i = 0 ; i < k ; ++i)
			{
			fprintf(initfile, datafmt, varname, offset, vlen, type);
			prconr(initfile, type, con.cd[i]);
			offset += typesize[type];
			}
		break;

	case TYCHAR:
		k = valp->vleng->b_const.fconst.ci;
		if(elen < k)
			k = elen;

		for(i = 0 ; i < k ; ++i)
			{
			fprintf(initfile, datafmt, varname, offset++, vlen, TYCHAR);
			fprintf(initfile, "\t%d\n", valp->b_const.fconst.ccp[i]);
			}
		k = elen - valp->vleng->b_const.fconst.ci;
		while( k-- > 0)
			{
			fprintf(initfile, datafmt, varname, offset++, vlen, TYCHAR);
			fprintf(initfile, "\t%d\n", ' ');
			}
		break;

	default:
		fatal1("setdata: impossible type %d", type);
	}

}


void
frdata(p0)
chainp p0;
{
register chainp p;
register bigptr q;

for(p = p0 ; p ; p = p->chain.nextp)
	{
	q = p->chain.datap;
	if(q->tag == TIMPLDO)
		{
		if(q->isbusy)
			return;	/* circular chain completed */
		q->isbusy = YES;
		frdata(q->b_impldo.datalist);
		ckfree(q);
		}
	else
		frexpr(q);
	}

frchain( &p0);
}
