/*	$Id: init.c,v 1.16 2008/12/24 17:40:41 sgk Exp $	*/
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


FILEP infile;
FILEP diagfile;

long int headoffset;

char token[100];
int toklen;
int lineno;
char *infname;
int needkwd;
struct labelblock *thislabel	= NULL;
flag nowarnflag	= NO;
flag ftn66flag	= NO;
flag profileflag	= NO;
flag optimflag	= NO;
flag quietflag	= NO;
flag shiftcase	= YES;
flag undeftype	= NO;
flag shortsubs	= YES;
flag onetripflag	= NO;
flag checksubs	= NO;
flag debugflag	= NO;
int nerr;
int nwarn;
int ndata;

flag saveall;
flag substars;
int parstate	= OUTSIDE;
flag headerdone	= NO;
int blklevel;
int impltype[26];
int implleng[26];
int implstg[26];

int tyint	= TYLONG ;
int tylogical	= TYLONG;
ftnint typesize[NTYPES]
	= { 1, FSZADDR, FSZSHORT, FSZLONG, FSZLONG, 2*FSZLONG,
	    2*FSZLONG, 4*FSZLONG, FSZLONG, 1, 1, 1};
int typealign[NTYPES]
	= { 1, ALIADDR, ALISHORT, ALILONG, ALILONG, ALIDOUBLE,
	    ALILONG, ALIDOUBLE, ALILONG, 1, 1, 1};
int procno;
int proctype	= TYUNKNOWN;
char *procname;
int rtvlabel[NTYPES];
int fudgelabel;
struct bigblock *typeaddr;
struct bigblock *retslot;
int cxslot	= -1;
int chslot	= -1;
int chlgslot	= -1;
int procclass	= CLUNKNOWN;
int nentry;
flag multitype;
ftnint procleng;
int lastlabno	= 10;
int lastvarno;
int lastargslot;
int argloc;
ftnint autoleng;
ftnint bssleng	= 0;
int retlabel;
int ret0label;
struct ctlframe ctls[MAXCTL];
struct ctlframe *ctlstack	= ctls-1;
struct ctlframe *lastctl	= ctls+MAXCTL ;

bigptr regnamep[10]; /* XXX MAXREGVAR */
int highregvar;

struct extsym extsymtab[MAXEXT];
struct extsym *nextext	= extsymtab;
struct extsym *lastext	= extsymtab+MAXEXT;

struct equivblock eqvclass[MAXEQUIV];
struct hashentry hashtab[MAXHASH];
struct hashentry *lasthash	= hashtab+MAXHASH;

struct labelblock labeltab[MAXSTNO];
struct labelblock *labtabend	= labeltab+MAXSTNO;
struct labelblock *highlabtab =	labeltab;
chainp rpllist	= NULL;
chainp curdtp	= NULL;
flag toomanyinit;
ftnint curdtelt;
chainp templist	= NULL;
chainp holdtemps	= NULL;
int dorange	= 0;
chainp entries	= NULL;
chainp chains	= NULL;

flag inioctl;
struct bigblock *ioblkp;
int iostmt;
int nioctl;
int nequiv	= 0;
int nintnames	= 0;
int nextnames	= 0;

struct literal litpool[MAXLITERALS];
int nliterals;

/*
 * Return a number for internal labels.
 */
int getlab(void);

int crslab = 10;
int
getlab(void)
{
	return crslab++;
}


void
fileinit()
{
procno = 0;
lastlabno = 10;
lastvarno = 0;
nextext = extsymtab;
nliterals = 0;
nerr = 0;
ndata = 0;
}




void
procinit()
{
register struct bigblock *p;
register struct dimblock *q;
register struct hashentry *hp;
register struct labelblock *lp;
chainp cp;
int i;

	setloc(RDATA);
parstate = OUTSIDE;
headerdone = NO;
blklevel = 1;
saveall = NO;
substars = NO;
nwarn = 0;
thislabel = NULL;
needkwd = 0;

++procno;
proctype = TYUNKNOWN;
procname = "MAIN_    ";
procclass = CLUNKNOWN;
nentry = 0;
multitype = NO;
typeaddr = NULL;
retslot = NULL;
cxslot = -1;
chslot = -1;
chlgslot = -1;
procleng = 0;
blklevel = 1;
lastargslot = 0;
	autoleng = AUTOINIT;

for(lp = labeltab ; lp < labtabend ; ++lp)
	lp->stateno = 0;

for(hp = hashtab ; hp < lasthash ; ++hp)
	if((p = hp->varp))
		{
		frexpr(p->vleng);
		if((q = p->b_name.vdim))
			{
			for(i = 0 ; i < q->ndim ; ++i)
				{
				frexpr(q->dims[i].dimsize);
				frexpr(q->dims[i].dimexpr);
				}
			frexpr(q->nelt);
			frexpr(q->baseoffset);
			frexpr(q->basexpr);
			ckfree(q);
			}
		ckfree(p);
		hp->varp = NULL;
		}
nintnames = 0;
highlabtab = labeltab;

ctlstack = ctls - 1;
for(cp = templist ; cp ; cp = cp->chain.nextp)
	ckfree(cp->chain.datap);
frchain(&templist);
holdtemps = NULL;
dorange = 0;
highregvar = 0;
entries = NULL;
rpllist = NULL;
inioctl = NO;
ioblkp = NULL;
nequiv = 0;

for(i = 0 ; i<NTYPES ; ++i)
	rtvlabel[i] = 0;
fudgelabel = 0;

if(undeftype)
	setimpl(TYUNKNOWN, (ftnint) 0, 'a', 'z');
else
	{
	setimpl(TYREAL, (ftnint) 0, 'a', 'z');
	setimpl(tyint,  (ftnint) 0, 'i', 'n');
	}
setimpl(-STGBSS, (ftnint) 0, 'a', 'z');	/* set class */
setlog();
}



void
setimpl(type, length, c1, c2)
int type;
ftnint length;
int c1, c2;
{
int i;
char buff[100];

if(c1==0 || c2==0)
	return;

if(c1 > c2) {
	sprintf(buff, "characters out of order in implicit:%c-%c", c1, c2);
	err(buff);
} else
	if(type < 0)
		for(i = c1 ; i<=c2 ; ++i)
			implstg[i-'a'] = - type;
	else
		{
		type = lengtype(type, (int) length);
		if(type != TYCHAR)
			length = 0;
		for(i = c1 ; i<=c2 ; ++i)
			{
			impltype[i-'a'] = type;
			implleng[i-'a'] = length;
			}
		}
}
