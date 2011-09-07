/*	$Id: misc.c,v 1.17 2009/02/11 15:58:55 ragge Exp $	*/
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

int max(int, int);

void
cpn(n, a, b)
register int n;
register char *a, *b;
{
while(--n >= 0)
	*b++ = *a++;
}


int
eqn(n, a, b)
register int n;
register char *a, *b;
{
while(--n >= 0)
	if(*a++ != *b++)
		return(NO);
return(YES);
}






int
cmpstr(a, b, la, lb)	/* compare two strings */
register char *a, *b;
ftnint la, lb;
{
register char *aend, *bend;
aend = a + la;
bend = b + lb;


if(la <= lb)
	{
	while(a < aend)
		if(*a != *b)
			return( *a - *b );
		else
			{ ++a; ++b; }

	while(b < bend)
		if(*b != ' ')
			return(' ' - *b);
		else
			++b;
	}

else
	{
	while(b < bend)
		if(*a != *b)
			return( *a - *b );
		else
			{ ++a; ++b; }
	while(a < aend)
		if(*a != ' ')
			return(*a - ' ');
		else
			++a;
	}
return(0);
}





chainp hookup(x,y)
register chainp x, y;
{
register chainp p;

if(x == NULL)
	return(y);

for(p = x ; p->chain.nextp ; p = p->chain.nextp)
	;
p->chain.nextp = y;
return(x);
}



struct bigblock *mklist(p)
chainp p;
{
register struct bigblock *q;

q = BALLO();
q->tag = TLIST;
q->b_list.listp = p;
return(q);
}


chainp
mkchain(bigptr p, chainp q)
{
	chainp r;

	if(chains) {
		r = chains;
		chains = chains->chain.nextp;
	} else
		r = ALLOC(chain);

	r->chain.datap = p;
	r->chain.nextp = q;
	return(r);
}



char * varstr(n, s)
register int n;
register char *s;
{
register int i;
static char name[XL+1];

for(i=0;  i<n && *s!=' ' && *s!='\0' ; ++i)
	name[i] = *s++;

name[i] = '\0';

return( name );
}




char * varunder(n, s)
register int n;
register char *s;
{
register int i;
static char name[XL+1];

for(i=0;  i<n && *s!=' ' && *s!='\0' ; ++i)
	name[i] = *s++;

name[i] = '\0';

return( name );
}





char * nounder(n, s)
register int n;
register char *s;
{
register int i;
static char name[XL+1];

for(i=0;  i<n && *s!=' ' && *s!='\0' ; ++s)
	if(*s != '_')
		name[i++] = *s;

name[i] = '\0';

return( name );
}

/*
 * Save a block on heap.
 */
char *
copyn(int n, char *s)
{
	char *p, *q;

	p = q = ckalloc(n);
	while(--n >= 0)
		*q++ = *s++;
	return(p);
}

/*
 * Save a string on heap.
 */
char *
copys(char *s)
{
	return(copyn(strlen(s)+1 , s));
}

/*
 * convert a string to an int.
 */
ftnint
convci(int n, char *s)
{
	ftnint sum;
	sum = 0;
	while(n-- > 0)
		sum = 10*sum + (*s++ - '0');
	return(sum);
}

char *convic(n)
ftnint n;
{
static char s[20];
register char *t;

s[19] = '\0';
t = s+19;

do	{
	*--t = '0' + n%10;
	n /= 10;
	} while(n > 0);

return(t);
}



double convcd(n, s)
int n;
register char *s;
{
char v[100];
register char *t;
if(n > 90)
	{
	err("too many digits in floating constant");
	n = 90;
	}
for(t = v ; n-- > 0 ; s++)
	*t++ = (*s=='d' ? 'e' : *s);
*t = '\0';
return( atof(v) );
}



struct bigblock *mkname(l, s)
int l;
register char *s;
{
struct hashentry *hp;
int hash;
register struct bigblock *q;
register int i;
char n[VL];

hash = 0;
for(i = 0 ; i<l && *s!='\0' ; ++i)
	{
	hash += *s;
	n[i] = *s++;
	}
hash %= MAXHASH;
while( i < VL )
	n[i++] = ' ';

hp = hashtab + hash;
while((q = hp->varp))
	if( hash==hp->hashval && eqn(VL,n,q->b_name.varname) )
		return(q);
	else if(++hp >= lasthash)
		hp = hashtab;

if(++nintnames >= MAXHASH-1)
	fatal("hash table full");
hp->varp = q = BALLO();
hp->hashval = hash;
q->tag = TNAME;
cpn(VL, n, q->b_name.varname);
return(q);
}



struct labelblock *mklabel(l)
ftnint l;
{
register struct labelblock *lp;

if(l == 0)
	return(0);

for(lp = labeltab ; lp < highlabtab ; ++lp)
	if(lp->stateno == l)
		return(lp);

if(++highlabtab >= labtabend)
	fatal("too many statement numbers");

lp->stateno = l;
lp->labelno = newlabel();
lp->blklevel = 0;
lp->labused = NO;
lp->labdefined = NO;
lp->labinacc = NO;
lp->labtype = LABUNKNOWN;
return(lp);
}

int
newlabel()
{
return( lastlabno++ );
}


/* find or put a name in the external symbol table */

struct extsym *mkext(s)
char *s;
{
int i;
register char *t;
char n[XL];
struct extsym *p;

i = 0;
t = n;
while(i<XL && *s)
	*t++ = *s++;
while(t < n+XL)
	*t++ = ' ';

for(p = extsymtab ; p<nextext ; ++p)
	if(eqn(XL, n, p->extname))
		return( p );

if(nextext >= lastext)
	fatal("too many external symbols");

cpn(XL, n, nextext->extname);
nextext->extstg = STGUNKNOWN;
nextext->extsave = NO;
nextext->extp = 0;
nextext->extleng = 0;
nextext->maxleng = 0;
nextext->extinit = NO;
return( nextext++ );
}








struct bigblock *builtin(t, s)
int t;
char *s;
{
register struct extsym *p;
register struct bigblock *q;

p = mkext(s);
if(p->extstg == STGUNKNOWN)
	p->extstg = STGEXT;
else if(p->extstg != STGEXT)
	{
	err1("improper use of builtin %s", s);
	return(0);
	}

q = BALLO();
q->tag = TADDR;
q->vtype = t;
q->vclass = CLPROC;
q->vstg = STGEXT;
q->b_addr.memno = p - extsymtab;
return(q);
}


void
frchain(p)
register chainp *p;
{
register chainp q;

if(p==0 || *p==0)
	return;

for(q = *p; q->chain.nextp ; q = q->chain.nextp)
	;
q->chain.nextp = chains;
chains = *p;
*p = 0;
}


ptr cpblock(n,p)
register int n;
register void * p;
{
register char *q, *r = p;
ptr q0;

q = q0 = ckalloc(n);
while(n-- > 0)
	*q++ = *r++;
return(q0);
}


int
max(a,b)
int a,b;
{
return( a>b ? a : b);
}


ftnint lmax(a, b)
ftnint a, b;
{
return( a>b ? a : b);
}

ftnint lmin(a, b)
ftnint a, b;
{
return(a < b ? a : b);
}



int
maxtype(t1, t2)
int t1, t2;
{
int t;

t = max(t1, t2);
if(t==TYCOMPLEX && (t1==TYDREAL || t2==TYDREAL) )
	t = TYDCOMPLEX;
return(t);
}



/* return log base 2 of n if n a power of 2; otherwise -1 */
int
flog2(n)
ftnint n;
{
int k;

/* trick based on binary representation */

if(n<=0 || (n & (n-1))!=0)
	return(-1);

for(k = 0 ;  n >>= 1  ; ++k)
	;
return(k);
}


void
frrpl()
{
chainp rp;

while(rpllist)
	{
	rp = rpllist->rplblock.nextp;
	ckfree(rpllist);
	rpllist = rp;
	}
}

void
popstack(p)
register chainp *p;
{
register chainp q;

if(p==NULL || *p==NULL)
	fatal("popstack: stack empty");
q = (*p)->chain.nextp;
ckfree(*p);
*p = q;
}



struct bigblock *
callk(type, name, args)
int type;
char *name;
bigptr args;
{
register struct bigblock *p;

p = mkexpr(OPCALL, builtin(type,name), args);
p->vtype = type;
return(p);
}



struct bigblock *
call4(type, name, arg1, arg2, arg3, arg4)
int type;
char *name;
bigptr arg1, arg2, arg3, arg4;
{
struct bigblock *args;
args = mklist( mkchain(arg1, mkchain(arg2, mkchain(arg3, mkchain(arg4, NULL)) ) ) );
return( callk(type, name, args) );
}




struct bigblock *call3(type, name, arg1, arg2, arg3)
int type;
char *name;
bigptr arg1, arg2, arg3;
{
struct bigblock *args;
args = mklist( mkchain(arg1, mkchain(arg2, mkchain(arg3, NULL) ) ) );
return( callk(type, name, args) );
}





struct bigblock *
call2(type, name, arg1, arg2)
int type;
char *name;
bigptr arg1, arg2;
{
bigptr args;

args = mklist( mkchain(arg1, mkchain(arg2, NULL) ) );
return( callk(type,name, args) );
}




struct bigblock *call1(type, name, arg)
int type;
char *name;
bigptr arg;
{
return( callk(type,name, mklist(mkchain(arg,0)) ));
}


struct bigblock *call0(type, name)
int type;
char *name;
{
return( callk(type, name, NULL) );
}



struct bigblock *
mkiodo(dospec, list)
chainp dospec, list;
{
register struct bigblock *q;

q = BALLO();
q->tag = TIMPLDO;
q->b_impldo.varnp = (struct bigblock *)dospec;
q->b_impldo.datalist = list;
return(q);
}




ptr 
ckalloc(int n)
{
	ptr p;

	if ((p = calloc(1, (unsigned) n)) == NULL)
		fatal("out of memory");
#ifdef PCC_DEBUG
	if (mflag)
		printf("ckalloc: sz %d ptr %p\n", n, p);
#endif
	return(p);
}

void
ckfree(void *p)
{
#ifdef PCC_DEBUG
	if (mflag)
		printf("ckfree: ptr %p\n", p);
#endif
	free(p);
}

#if 0
int
isaddr(p)
register bigptr p;
{
if(p->tag == TADDR)
	return(YES);
if(p->tag == TEXPR)
	switch(p->b_expr.opcode)
		{
		case OPCOMMA:
			return( isaddr(p->b_expr.rightp) );

		case OPASSIGN:
		case OPPLUSEQ:
			return( isaddr(p->b_expr.leftp) );
		}
return(NO);
}
#endif

/*
 * Return YES if not an expression.
 */
int
addressable(bigptr p)
{
	switch(p->tag) {
	case TCONST:
		return(YES);

	case TADDR:
		return( addressable(p->b_addr.memoffset) );

	default:
		return(NO);
	}
}


int
hextoi(c)
register int c;
{
register char *p;
static char p0[17] = "0123456789abcdef";

for(p = p0 ; *p ; ++p)
	if(*p == c)
		return( p-p0 );
return(16);
}
