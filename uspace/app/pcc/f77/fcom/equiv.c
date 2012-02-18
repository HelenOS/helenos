/*	$Id: equiv.c,v 1.11 2008/05/11 15:28:03 ragge Exp $	*/
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


/* ROUTINES RELATED TO EQUIVALENCE CLASS PROCESSING */
LOCAL void eqvcommon(struct equivblock *, int, ftnint);
LOCAL void eqveqv(int, int, ftnint);
LOCAL void freqchain(struct equivblock *p);
LOCAL int nsubs(struct bigblock *p);

/* called at end of declarations section to process chains
   created by EQUIVALENCE statements
 */
void
doequiv()
{
register int i;
int inequiv, comno, ovarno;
ftnint comoffset, offset, leng;
register struct equivblock *p;
register chainp q;
struct bigblock *itemp;
register struct bigblock *np;
bigptr offp;
int ns;
chainp cp;

ovarno = comoffset = offset = 0; /* XXX gcc */
for(i = 0 ; i < nequiv ; ++i)
	{
	p = &eqvclass[i];
	p->eqvbottom = p->eqvtop = 0;
	comno = -1;

	for(q = p->equivs ; q ; q = q->eqvchain.nextp)
		{
		itemp = q->eqvchain.eqvitem;
		vardcl(np = itemp->b_prim.namep);
		if(itemp->b_prim.argsp || itemp->b_prim.fcharp)
			{
			if(np->b_name.vdim!=NULL && np->b_name.vdim->ndim>1 &&
			   nsubs(itemp->b_prim.argsp)==1 )
				{
				if(! ftn66flag)
					warn("1-dim subscript in EQUIVALENCE");
				cp = NULL;
				ns = np->b_name.vdim->ndim;
				while(--ns > 0)
					cp = mkchain( MKICON(1), cp);
				itemp->b_prim.argsp->b_list.listp->chain.nextp = cp;
				}
			offp = suboffset(itemp);
			}
		else	offp = MKICON(0);
		if(ISICON(offp))
			offset = q->eqvchain.eqvoffset = offp->b_const.fconst.ci;
		else	{
			dclerr("nonconstant subscript in equivalence ", np);
			np = NULL;
			goto endit;
			}
		if( (leng = iarrlen(np)) < 0)
			{
			dclerr("adjustable in equivalence", np);
			np = NULL;
			goto endit;
			}
		p->eqvbottom = lmin(p->eqvbottom, -offset);
		p->eqvtop = lmax(p->eqvtop, leng-offset);

		switch(np->vstg)
			{
			case STGUNKNOWN:
			case STGBSS:
			case STGEQUIV:
				break;

			case STGCOMMON:
				comno = np->b_name.vardesc.varno;
				comoffset = np->b_name.voffset + offset;
				break;

			default:
				dclerr("bad storage class in equivalence", np);
				np = NULL;
				goto endit;
			}
	endit:
		frexpr(offp);
		q->eqvchain.eqvitem = np;
		}

	if(comno >= 0)
		eqvcommon(p, comno, comoffset);
	else  for(q = p->equivs ; q ; q = q->eqvchain.nextp)
		{
		if((np = q->eqvchain.eqvitem))
			{
			inequiv = NO;
			if(np->vstg==STGEQUIV) {
				if( (ovarno = np->b_name.vardesc.varno) == i)
					{
					if(np->b_name.voffset + q->eqvchain.eqvoffset != 0)
						dclerr("inconsistent equivalence", np);
					}
				else	{
					offset = np->b_name.voffset;
					inequiv = YES;
					}
			}
			np->vstg = STGEQUIV;
			np->b_name.vardesc.varno = i;
			np->b_name.voffset = - q->eqvchain.eqvoffset;

			if(inequiv)
				eqveqv(i, ovarno, q->eqvchain.eqvoffset + offset);
			}
		}
	}

for(i = 0 ; i < nequiv ; ++i)
	{
	p = & eqvclass[i];
	if(p->eqvbottom!=0 || p->eqvtop!=0)
		{
		for(q = p->equivs ; q; q = q->eqvchain.nextp)
			{
			np = q->eqvchain.eqvitem;
			np->b_name.voffset -= p->eqvbottom;
			if(np->b_name.voffset % typealign[np->vtype] != 0)
				dclerr("bad alignment forced by equivalence", np);
			}
		p->eqvtop -= p->eqvbottom;
		p->eqvbottom = 0;
		}
	freqchain(p);
	}
}





/* put equivalence chain p at common block comno + comoffset */

LOCAL void eqvcommon(p, comno, comoffset)
struct equivblock *p;
int comno;
ftnint comoffset;
{
int ovarno;
ftnint k, offq;
register struct bigblock *np;
register chainp q;

if(comoffset + p->eqvbottom < 0)
	{
	err1("attempt to extend common %s backward",
		nounder(XL, extsymtab[comno].extname) );
	freqchain(p);
	return;
	}

if( (k = comoffset + p->eqvtop) > extsymtab[comno].extleng)
	extsymtab[comno].extleng = k;

for(q = p->equivs ; q ; q = q->eqvchain.nextp)
	if((np = q->eqvchain.eqvitem))
		{
		switch(np->vstg)
			{
			case STGUNKNOWN:
			case STGBSS:
				np->vstg = STGCOMMON;
				np->b_name.vardesc.varno = comno;
				np->b_name.voffset = comoffset - q->eqvchain.eqvoffset;
				break;

			case STGEQUIV:
				ovarno = np->b_name.vardesc.varno;
				offq = comoffset - q->eqvchain.eqvoffset - np->b_name.voffset;
				np->vstg = STGCOMMON;
				np->b_name.vardesc.varno = comno;
				np->b_name.voffset = comoffset - q->eqvchain.eqvoffset;
				if(ovarno != (p - eqvclass))
					eqvcommon(&eqvclass[ovarno], comno, offq);
				break;

			case STGCOMMON:
				if(comno != np->b_name.vardesc.varno ||
				   comoffset != np->b_name.voffset+q->eqvchain.eqvoffset)
					dclerr("inconsistent common usage", np);
				break;


			default:
				fatal1("eqvcommon: impossible vstg %d", np->vstg);
			}
		}

freqchain(p);
p->eqvbottom = p->eqvtop = 0;
}


/* put all items on ovarno chain on front of nvarno chain
 * adjust offsets of ovarno elements and top and bottom of nvarno chain
 */

LOCAL void eqveqv(nvarno, ovarno, delta)
int ovarno, nvarno;
ftnint delta;
{
register struct equivblock *p0, *p;
register struct nameblock *np;
chainp q, q1;

p0 = eqvclass + nvarno;
p = eqvclass + ovarno;
p0->eqvbottom = lmin(p0->eqvbottom, p->eqvbottom - delta);
p0->eqvtop = lmax(p0->eqvtop, p->eqvtop - delta);
p->eqvbottom = p->eqvtop = 0;

for(q = p->equivs ; q ; q = q1)
	{
	q1 = q->eqvchain.nextp;
	if( (np = q->eqvchain.eqvitem) && np->vardesc.varno==ovarno)
		{
		q->eqvchain.nextp = p0->equivs;
		p0->equivs = q;
		q->eqvchain.eqvoffset -= delta;
		np->vardesc.varno = nvarno;
		np->voffset -= delta;
		}
	else	ckfree(q);
	}
p->equivs = NULL;
}




LOCAL void
freqchain(p)
register struct equivblock *p;
{
register chainp q, oq;

for(q = p->equivs ; q ; q = oq)
	{
	oq = q->eqvchain.nextp;
	ckfree(q);
	}
p->equivs = NULL;
}





LOCAL int
nsubs(p)
register struct bigblock *p;
{
register int n;
register chainp q;

n = 0;
if(p)
	for(q = p->b_list.listp ; q ; q = q->chain.nextp)
		++n;

return(n);
}
