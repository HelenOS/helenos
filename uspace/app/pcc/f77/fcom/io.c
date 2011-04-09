/*	$Id: io.c,v 1.15 2008/12/19 08:08:48 ragge Exp $	*/
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
/* TEMPORARY */
#define TYIOINT TYLONG
#define FSZIOINT FSZLONG

#include <string.h>

#include "defines.h"
#include "defs.h"

LOCAL void doiolist(chainp);
LOCAL void dofopen(void);
LOCAL void dofclose(void);
LOCAL void dofinquire(void);
LOCAL void dofmove(char *);
LOCAL void ioset(int, int, bigptr);
LOCAL void iosetc(int, bigptr);
LOCAL void iosetip(int, int);
LOCAL void iosetlc(int, int, int);
LOCAL void putiocall(struct bigblock *q);
LOCAL void putio(bigptr, bigptr);
LOCAL void startrw(void);


LOCAL char ioroutine[XL+1];

LOCAL int ioendlab;
LOCAL int ioerrlab;
LOCAL int endbit;
LOCAL int jumplab;
LOCAL int skiplab;
LOCAL int ioformatted;

#define UNFORMATTED 0
#define FORMATTED 1
#define LISTDIRECTED 2

#define V(z)	ioc[z].iocval

#define IOALL 07777

LOCAL struct ioclist
	{
	char *iocname;
	int iotype;
	bigptr iocval;
	} ioc[ ] =
	{
		{ "", 0 },
		{ "unit", IOALL },
		{ "fmt", M(IOREAD) | M(IOWRITE) },
		{ "err", IOALL },
		{ "end", M(IOREAD) },
		{ "iostat", IOALL },
		{ "rec", M(IOREAD) | M(IOWRITE) },
		{ "recl", M(IOOPEN) | M(IOINQUIRE) },
		{ "file", M(IOOPEN) | M(IOINQUIRE) },
		{ "status", M(IOOPEN) | M(IOCLOSE) },
		{ "access", M(IOOPEN) | M(IOINQUIRE) },
		{ "form", M(IOOPEN) | M(IOINQUIRE) },
		{ "blank", M(IOOPEN) | M(IOINQUIRE) },
		{ "exist", M(IOINQUIRE) },
		{ "opened", M(IOINQUIRE) },
		{ "number", M(IOINQUIRE) },
		{ "named", M(IOINQUIRE) },
		{ "name", M(IOINQUIRE) },
		{ "sequential", M(IOINQUIRE) },
		{ "direct", M(IOINQUIRE) },
		{ "formatted", M(IOINQUIRE) },
		{ "unformatted", M(IOINQUIRE) },
		{ "nextrec", M(IOINQUIRE) }
	} ;

#define NIOS (sizeof(ioc)/sizeof(struct ioclist) - 1)
#define MAXIO	FSZFLAG + 10*FSZIOINT + 15*FSZADDR

#define IOSUNIT 1
#define IOSFMT 2
#define IOSERR 3
#define IOSEND 4
#define IOSIOSTAT 5
#define IOSREC 6
#define IOSRECL 7
#define IOSFILE 8
#define IOSSTATUS 9
#define IOSACCESS 10
#define IOSFORM 11
#define IOSBLANK 12
#define IOSEXISTS 13
#define IOSOPENED 14
#define IOSNUMBER 15
#define IOSNAMED 16
#define IOSNAME 17
#define IOSSEQUENTIAL 18
#define IOSDIRECT 19
#define IOSFORMATTED 20
#define IOSUNFORMATTED 21
#define IOSNEXTREC 22

#define IOSTP V(IOSIOSTAT)


/* offsets in generated structures */

#define FSZFLAG FSZIOINT

#define XERR 0
#define XUNIT	FSZFLAG
#define XEND	FSZFLAG + FSZIOINT
#define XFMT	2*FSZFLAG + FSZIOINT
#define XREC	2*FSZFLAG + FSZIOINT + FSZADDR
#define XRLEN	2*FSZFLAG + 2*FSZADDR
#define XRNUM	2*FSZFLAG + 2*FSZADDR + FSZIOINT

#define XIFMT	2*FSZFLAG + FSZADDR
#define XIEND	FSZFLAG + FSZADDR
#define XIUNIT	FSZFLAG

#define XFNAME	FSZFLAG + FSZIOINT
#define XFNAMELEN	FSZFLAG + FSZIOINT + FSZADDR
#define XSTATUS	FSZFLAG + 2*FSZIOINT + FSZADDR
#define XACCESS	FSZFLAG + 2*FSZIOINT + 2*FSZADDR
#define XFORMATTED	FSZFLAG + 2*FSZIOINT + 3*FSZADDR
#define XRECLEN	FSZFLAG + 2*FSZIOINT + 4*FSZADDR
#define XBLANK	FSZFLAG + 3*FSZIOINT + 4*FSZADDR

#define XCLSTATUS	FSZFLAG + FSZIOINT

#define XFILE	FSZFLAG + FSZIOINT
#define XFILELEN	FSZFLAG + FSZIOINT + FSZADDR
#define XEXISTS	FSZFLAG + 2*FSZIOINT + FSZADDR
#define XOPEN	FSZFLAG + 2*FSZIOINT + 2*FSZADDR
#define XNUMBER	FSZFLAG + 2*FSZIOINT + 3*FSZADDR
#define XNAMED	FSZFLAG + 2*FSZIOINT + 4*FSZADDR
#define XNAME	FSZFLAG + 2*FSZIOINT + 5*FSZADDR
#define XNAMELEN	FSZFLAG + 2*FSZIOINT + 6*FSZADDR
#define XQACCESS	FSZFLAG + 3*FSZIOINT + 6*FSZADDR
#define XQACCLEN	FSZFLAG + 3*FSZIOINT + 7*FSZADDR
#define XSEQ	FSZFLAG + 4*FSZIOINT + 7*FSZADDR
#define XSEQLEN	FSZFLAG + 4*FSZIOINT + 8*FSZADDR
#define XDIRECT	FSZFLAG + 5*FSZIOINT + 8*FSZADDR
#define XDIRLEN	FSZFLAG + 5*FSZIOINT + 9*FSZADDR
#define XFORM	FSZFLAG + 6*FSZIOINT + 9*FSZADDR
#define XFORMLEN	FSZFLAG + 6*FSZIOINT + 10*FSZADDR
#define XFMTED	FSZFLAG + 7*FSZIOINT + 10*FSZADDR
#define XFMTEDLEN	FSZFLAG + 7*FSZIOINT + 11*FSZADDR
#define XUNFMT	FSZFLAG + 8*FSZIOINT + 11*FSZADDR
#define XUNFMTLEN	FSZFLAG + 8*FSZIOINT + 12*FSZADDR
#define XQRECL	FSZFLAG + 9*FSZIOINT + 12*FSZADDR
#define XNEXTREC	FSZFLAG + 9*FSZIOINT + 13*FSZADDR
#define XQBLANK	FSZFLAG + 9*FSZIOINT + 14*FSZADDR
#define XQBLANKLEN	FSZFLAG + 9*FSZIOINT + 15*FSZADDR

int
fmtstmt(lp)
register struct labelblock *lp;
{
if(lp == NULL)
	{
	execerr("unlabeled format statement" , 0);
	return(-1);
	}
if(lp->labtype == LABUNKNOWN)
	{
	lp->labtype = LABFORMAT;
	lp->labelno = newlabel();
	}
else if(lp->labtype != LABFORMAT)
	{
	execerr("bad format number", 0);
	return(-1);
	}
return(lp->labelno);
}


void
setfmt(struct labelblock *lp)
{
	ftnint n;
	char *s;

	s = lexline(&n);
	preven(ALILONG);
	prlabel(lp->labelno);
	putstr(s, n);
	flline();
}


void
startioctl()
{
unsigned int i;

inioctl = YES;
nioctl = 0;
ioerrlab = 0;
ioformatted = UNFORMATTED;
for(i = 1 ; i<=NIOS ; ++i)
	V(i) = NULL;
}


void
endioctl()
{
unsigned int i;
bigptr p;

inioctl = NO;
if(ioblkp == NULL)
	ioblkp = autovar( (MAXIO+FSZIOINT-1)/FSZIOINT , TYIOINT, NULL);

/* set up for error recovery */

ioerrlab = ioendlab = skiplab = jumplab = 0;

if((p = V(IOSEND))) {
	if(ISICON(p))
		ioendlab = mklabel(p->b_const.fconst.ci)->labelno;
	else
		err("bad end= clause");
}

if((p = V(IOSERR))) {
	if(ISICON(p))
		ioerrlab = mklabel(p->b_const.fconst.ci)->labelno;
	else
		err("bad err= clause");
}

if(IOSTP==NULL && ioerrlab!=0 && ioendlab!=0 && ioerrlab!=ioendlab)
	IOSTP = fmktemp(TYINT, NULL);

if(IOSTP != NULL) {
	if(IOSTP->tag!=TADDR || ! ISINT(IOSTP->vtype) )
		{
		err("iostat must be an integer variable");
		frexpr(IOSTP);
		IOSTP = NULL;
		}
}

if(IOSTP)
	{
	if( (iostmt==IOREAD || iostmt==IOWRITE) &&
	    (ioerrlab!=ioendlab || ioerrlab==0) )
		jumplab = skiplab = newlabel();
	else
		jumplab = ioerrlab;
	}
else
	{
	jumplab = ioerrlab;
	if(ioendlab)
		jumplab = ioendlab;
	}

ioset(TYIOINT, XERR, MKICON(IOSTP!=NULL || ioerrlab!=0) );
endbit = IOSTP!=NULL || ioendlab!=0;	/* for use in startrw() */

switch(iostmt)
	{
	case IOOPEN:
		dofopen();  break;

	case IOCLOSE:
		dofclose();  break;

	case IOINQUIRE:
		dofinquire();  break;

	case IOBACKSPACE:
		dofmove("f_back"); break;

	case IOREWIND:
		dofmove("f_rew");  break;

	case IOENDFILE:
		dofmove("f_end");  break;

	case IOREAD:
	case IOWRITE:
		startrw();  break;

	default:
		fatal1("impossible iostmt %d", iostmt);
	}
for(i = 1 ; i<=NIOS ; ++i)
	if(i!=IOSIOSTAT || (iostmt!=IOREAD && iostmt!=IOWRITE) )
		frexpr(V(i));
}


int
iocname()
{
unsigned int i;
int found, mask;

found = 0;
mask = M(iostmt);
for(i = 1 ; i <= NIOS ; ++i) {
	if(toklen==(int)strlen(ioc[i].iocname) && eqn(toklen, token, ioc[i].iocname)) {
		if(ioc[i].iotype & mask)
			return(i);
		else	found = i;
	}
}

if(found)
	err1("invalid control %s for statement", ioc[found].iocname);
else
	err1("unknown iocontrol %s", varstr(toklen, token) );
return(IOSBAD);
}

void
ioclause(n, p)
register int n;
register bigptr p;
{
struct ioclist *iocp;

++nioctl;
if(n == IOSBAD)
	return;
if(n == IOSPOSITIONAL)
	{
	if(nioctl > IOSFMT)
		{
		err("illegal positional iocontrol");
		return;
		}
	n = nioctl;
	}

if(p == NULL)
	{
	if(n == IOSUNIT)
		p = (iostmt==IOREAD ? IOSTDIN : IOSTDOUT);
	else if(n != IOSFMT)
		{
		err("illegal * iocontrol");
		return;
		}
	}
if(n == IOSFMT)
	ioformatted = (p==NULL ? LISTDIRECTED : FORMATTED);

iocp = & ioc[n];
if(iocp->iocval == NULL)
	{
	p = cpexpr(p);
	if(n!=IOSFMT && ( n!=IOSUNIT || (p!=NULL && p->vtype!=TYCHAR) ) )
		p = fixtype(p);
	iocp->iocval = p;
}
else
	err1("iocontrol %s repeated", iocp->iocname);
}

/* io list item */
void
doio(list)
chainp list;
{
doiolist(list);
ioroutine[0] = 'e';
putiocall( call0(TYINT, ioroutine) );
frexpr(IOSTP);
}





LOCAL void doiolist(p0)
chainp p0;
{
chainp p;
register bigptr q;
register bigptr qe;
register struct bigblock *qn;
struct bigblock *tp;
int range;

for (p = p0 ; p ; p = p->chain.nextp)
	{
	q = p->chain.datap;
	if(q->tag == TIMPLDO)
		{
		exdo(range=newlabel(), (chainp)q->b_impldo.varnp);
		doiolist(q->b_impldo.datalist);
		enddo(range);
		ckfree(q);
		}
	else	{
		if(q->tag==TPRIM && q->b_prim.argsp==NULL && q->b_prim.namep->b_name.vdim!=NULL)
			{
			vardcl(qn = q->b_prim.namep);
			if(qn->b_name.vdim->nelt)
				putio( fixtype(cpexpr(qn->b_name.vdim->nelt)),
					mkscalar(qn) );
			else
				err("attempt to i/o array of unknown size");
			}
		else if(q->tag==TPRIM && q->b_prim.argsp==NULL && (qe = memversion(q->b_prim.namep)) )
			putio(MKICON(1),qe);
		else if( (qe = fixtype(cpexpr(q)))->tag==TADDR)
			putio(MKICON(1), qe);
		else if(qe->vtype != TYERROR)
			{
			if(iostmt == IOWRITE)
				{
				tp = fmktemp(qe->vtype, qe->vleng);
				puteq( cpexpr(tp), qe);
				putio(MKICON(1), tp);
				}
			else
				err("non-left side in READ list");
			}
		frexpr(q);
		}
	}
frchain( &p0 );
}





LOCAL void
putio(nelt, addr)
bigptr nelt;
register bigptr addr;
{
int type;
register struct bigblock *q;

type = addr->vtype;
if(ioformatted!=LISTDIRECTED && ISCOMPLEX(type) )
	{
	nelt = mkexpr(OPSTAR, MKICON(2), nelt);
	type -= (TYCOMPLEX-TYREAL);
	}

/* pass a length with every item.  for noncharacter data, fake one */
if(type != TYCHAR)
	{
	if( ISCONST(addr) )
		addr = putconst(addr);
	addr->vtype = TYCHAR;
	addr->vleng = MKICON( typesize[type] );
	}

nelt = fixtype( mkconv(TYLENG,nelt) );
if(ioformatted == LISTDIRECTED)
	q = call3(TYINT, "do_lio", mkconv(TYLONG, MKICON(type)), nelt, addr);
else
	q = call2(TYINT, (ioformatted==FORMATTED ? "do_fio" : "do_uio"),
		nelt, addr);
putiocall(q);
}



void
endio()
{
if(skiplab)
	{
	putlabel(skiplab);
	if(ioendlab)
		putif( mkexpr(OPGE, cpexpr(IOSTP), MKICON(0)), ioendlab);
	if(ioerrlab)
		putif( mkexpr( ( (iostmt==IOREAD||iostmt==IOWRITE) ? OPLE : OPEQ),
			cpexpr(IOSTP), MKICON(0)) , ioerrlab);
	}
if(IOSTP)
	frexpr(IOSTP);
}



LOCAL void
putiocall(q)
register struct bigblock *q;
{
if(IOSTP)
	{
	q->vtype = TYINT;
	q = fixexpr( mkexpr(OPASSIGN, cpexpr(IOSTP), q));
	}

if(jumplab)
	putif( mkexpr(OPEQ, q, MKICON(0) ), jumplab);
else
	putexpr(q);
}


void
startrw()
{
register bigptr p;
register struct bigblock *np;
register struct bigblock *unitp, *nump;
int k, fmtoff;
int intfile, sequential;


sequential = YES;
if((p = V(IOSREC))) {
	if( ISINT(p->vtype) )
		{
		ioset(TYIOINT, XREC, cpexpr(p) );
		sequential = NO;
		}
	else
		err("bad REC= clause");
}

intfile = NO;
if((p = V(IOSUNIT)))
	{
	if( ISINT(p->vtype) )
		ioset(TYIOINT, XUNIT, cpexpr(p) );
	else if(p->vtype == TYCHAR)
		{
		intfile = YES;
		if(p->tag==TPRIM && p->b_prim.argsp==NULL && (np = p->b_prim.namep)->b_name.vdim!=NULL)
			{
			vardcl(np);
			if(np->b_name.vdim->nelt)
				nump = cpexpr(np->b_name.vdim->nelt);
			else
				{
				err("attempt to use internal unit array of unknown size");
				nump = MKICON(1);
				}
			unitp = mkscalar(np);
			}
		else	{
			nump = MKICON(1);
			unitp = fixtype(cpexpr(p));
			}
		ioset(TYIOINT, XRNUM, nump);
		ioset(TYIOINT, XRLEN, cpexpr(unitp->vleng) );
		ioset(TYADDR, XUNIT, addrof(unitp) );
		}
	}
else
	err("bad unit specifier");

if(iostmt == IOREAD)
	ioset(TYIOINT, (intfile ? XIEND : XEND), MKICON(endbit) );

fmtoff = (intfile ? XIFMT : XFMT);

if((p = V(IOSFMT)))
	{
	if(p->tag==TPRIM && p->b_prim.argsp==NULL)
		{
		vardcl(np = p->b_prim.namep);
		if(np->b_name.vdim)
			{
			ioset(TYADDR, fmtoff, addrof(mkscalar(np)) );
			goto endfmt;
			}
		if( ISINT(np->vtype) )
			{
			ioset(TYADDR, fmtoff, cpexpr(p));
			goto endfmt;
			}
		}
	p = V(IOSFMT) = fixtype(p);
	if(p->vtype == TYCHAR)
		ioset(TYADDR, fmtoff, addrof(cpexpr(p)) );
	else if( ISICON(p) )
		{
		if( (k = fmtstmt( mklabel(p->b_const.fconst.ci) )) > 0 )
			ioset(TYADDR, fmtoff, mkaddcon(k) );
		else
			ioformatted = UNFORMATTED;
		}
	else	{
		err("bad format descriptor");
		ioformatted = UNFORMATTED;
		}
	}
else
	ioset(TYADDR, fmtoff, MKICON(0) );

endfmt:


ioroutine[0] = 's';
ioroutine[1] = '_';
ioroutine[2] = (iostmt==IOREAD ? 'r' : 'w');
ioroutine[3] = (sequential ? 's' : 'd');
ioroutine[4] = "ufl" [ioformatted];
ioroutine[5] = (intfile ? 'i' : 'e');
ioroutine[6] = '\0';
putiocall( call1(TYINT, ioroutine, cpexpr(ioblkp) ));
}



LOCAL void dofopen()
{
register bigptr p;

if( (p = V(IOSUNIT)) && ISINT(p->vtype) )
	ioset(TYIOINT, XUNIT, cpexpr(p) );
else
	err("bad unit in open");
if( (p = V(IOSFILE)) && p->vtype==TYCHAR)
	{
	ioset(TYIOINT, XFNAMELEN, cpexpr(p->vleng) );
	iosetc(XFNAME, p);
	}
else
	err("bad file in open");

if((p = V(IOSRECL)))
	if( ISINT(p->vtype) )
		ioset(TYIOINT, XRECLEN, cpexpr(p) );
	else
		err("bad recl");
else
	ioset(TYIOINT, XRECLEN, MKICON(0) );

iosetc(XSTATUS, V(IOSSTATUS));
iosetc(XACCESS, V(IOSACCESS));
iosetc(XFORMATTED, V(IOSFORM));
iosetc(XBLANK, V(IOSBLANK));

putiocall( call1(TYINT, "f_open", cpexpr(ioblkp) ));
}


LOCAL void
dofclose()
{
register bigptr p;

if( (p = V(IOSUNIT)) && ISINT(p->vtype) )
	{
	ioset(TYIOINT, XUNIT, cpexpr(p) );
	iosetc(XCLSTATUS, V(IOSSTATUS));
	putiocall( call1(TYINT, "f_clos", cpexpr(ioblkp)) );
	}
else
	err("bad unit in close statement");
}


LOCAL void dofinquire()
{
register bigptr p;
if((p = V(IOSUNIT)))
	{
	if( V(IOSFILE) )
		err("inquire by unit or by file, not both");
	ioset(TYIOINT, XUNIT, cpexpr(p) );
	}
else if( ! V(IOSFILE) )
	err("must inquire by unit or by file");
iosetlc(IOSFILE, XFILE, XFILELEN);
iosetip(IOSEXISTS, XEXISTS);
iosetip(IOSOPENED, XOPEN);
iosetip(IOSNUMBER, XNUMBER);
iosetip(IOSNAMED, XNAMED);
iosetlc(IOSNAME, XNAME, XNAMELEN);
iosetlc(IOSACCESS, XQACCESS, XQACCLEN);
iosetlc(IOSSEQUENTIAL, XSEQ, XSEQLEN);
iosetlc(IOSDIRECT, XDIRECT, XDIRLEN);
iosetlc(IOSFORM, XFORM, XFORMLEN);
iosetlc(IOSFORMATTED, XFMTED, XFMTEDLEN);
iosetlc(IOSUNFORMATTED, XUNFMT, XUNFMTLEN);
iosetip(IOSRECL, XQRECL);
iosetip(IOSNEXTREC, XNEXTREC);

putiocall( call1(TYINT,  "f_inqu", cpexpr(ioblkp) ));
}



LOCAL void
dofmove(subname)
char *subname;
{
register bigptr p;

if( (p = V(IOSUNIT)) && ISINT(p->vtype) )
	{
	ioset(TYIOINT, XUNIT, cpexpr(p) );
	putiocall( call1(TYINT, subname, cpexpr(ioblkp) ));
	}
else
	err("bad unit in move statement");
}



LOCAL void
ioset(type, offset, p)
int type, offset;
bigptr p;
{
register struct bigblock *q;

q = cpexpr(ioblkp);
q->vtype = type;
q->b_addr.memoffset = fixtype( mkexpr(OPPLUS, q->b_addr.memoffset, MKICON(offset)) );
puteq(q, p);
}




LOCAL void
iosetc(offset, p)
int offset;
register bigptr p;
{
if(p == NULL)
	ioset(TYADDR, offset, MKICON(0) );
else if(p->vtype == TYCHAR)
	ioset(TYADDR, offset, addrof(cpexpr(p) ));
else
	err("non-character control clause");
}



LOCAL void
iosetip(i, offset)
int i, offset;
{
register bigptr p;

if((p = V(i))) {
	if(p->tag==TADDR && ONEOF(p->vtype, M(TYLONG)|M(TYLOGICAL)) )
		ioset(TYADDR, offset, addrof(cpexpr(p)) );
	else
		err1("impossible inquire parameter %s", ioc[i].iocname);
} else
	ioset(TYADDR, offset, MKICON(0) );
}



LOCAL void
iosetlc(i, offp, offl)
int i, offp, offl;
{
register bigptr p;
if( (p = V(i)) && p->vtype==TYCHAR)
	ioset(TYIOINT, offl, cpexpr(p->vleng) );
iosetc(offp, p);
}
