/*	$Id: common.c,v 1.93 2011/01/22 22:08:23 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pass2.h"

# ifndef EXIT
# define EXIT exit
# endif

int nerrors = 0;  /* number of errors */
extern char *ftitle;
int lineno;

int warniserr = 0;

#ifndef WHERE
#define	WHERE(ch) fprintf(stderr, "%s, line %d: ", ftitle, lineno);
#endif

static void
incerr(void)
{
	if (++nerrors > 30)
		cerror("too many errors");
}

/*
 * nonfatal error message
 * the routine where is different for pass 1 and pass 2;
 * it tells where the error took place
 */
void
uerror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('u');
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	incerr();
}

/*
 * compiler error: die
 */
void
cerror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('c');

	/* give the compiler the benefit of the doubt */
	if (nerrors && nerrors <= 30) {
		fprintf(stderr,
		    "cannot recover from earlier errors: goodbye!\n");
	} else {
		fprintf(stderr, "compiler error: ");
		vfprintf(stderr, s, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
	EXIT(1);
}

/*
 * warning
 */
void
werror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('w');
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	if (warniserr)
		incerr();
}

#ifndef MKEXT

bittype warnary[(NUMW/NUMBITS)+1], werrary[(NUMW/NUMBITS)+1];

static char *warntxt[] = {
	"conversion to '%s' from '%s' may alter its value",
	"function declaration isn't a prototype", /* Wstrict_prototypes */
	"no previous prototype for `%s'", /* Wmissing_prototypes */
	"return type defaults to `int'", /* Wimplicit_int */
		 /* Wimplicit_function_declaration */
	"implicit declaration of function '%s'",
	"declaration of '%s' shadows a %s declaration", /* Wshadow */
	"illegal pointer combination", /* Wpointer_sign */
	"comparison between signed and unsigned", /* Wsign_compare */
	"ignoring #pragma %s %s", /* Wunknown_pragmas */
	"statement not reached", /* Wunreachable_code */
};

char *flagstr[] = {
	"truncate", "strict-prototypes", "missing-prototypes", 
	"implicit-int", "implicit-function-declaration", "shadow", 
	"pointer-sign", "sign-compare", "unknown-pragmas", 
	"unreachable-code", 
};

/*
 * "emulate" the gcc warning flags.
 */
void
Wflags(char *str)
{
	int i, flagval;

	if (strncmp("no-", str, 3) == 0) {
		str += 3;
		flagval = 0;
	} else
		flagval = 1;
	if (strcmp(str, "error") == 0) {
		/* special */
		for (i = 0; i < NUMW; i++)
			BITSET(werrary, i);
		return;
	}
	for (i = 0; i < NUMW; i++) {
		if (strcmp(flagstr[i], str) != 0)
			continue;
		if (flagval)
			BITSET(warnary, i);
		else
			BITCLEAR(warnary, i);
		return;
	}
	fprintf(stderr, "unrecognised warning option '%s'\n", str);
}

/*
 * Deal with gcc warnings.
 */
void
warner(int type, ...)
{
	va_list ap;
	char *w;

	if (TESTBIT(warnary, type) == 0)
		return; /* no warning */
	if (TESTBIT(werrary, type)) {
		w = "error";
		incerr();
	} else
		w = "warning";

	va_start(ap, type);
	fprintf(stderr, "%s:%d: %s: ", ftitle, lineno, w);
	vfprintf(stderr, warntxt[type], ap);
	fprintf(stderr, "\n");
	va_end(ap);
}
#endif /* MKEXT */

#ifndef MKEXT
static NODE *freelink;
static int usednodes;

#ifndef LANG_F77
NODE *
talloc()
{
	register NODE *p;

	usednodes++;

	if (freelink != NULL) {
		p = freelink;
		freelink = p->next;
		if (p->n_op != FREE)
			cerror("node not FREE: %p", p);
		if (nflag)
			printf("alloc node %p from freelist\n", p);
		return p;
	}

	p = permalloc(sizeof(NODE));
	p->n_op = FREE;
	if (nflag)
		printf("alloc node %p from memory\n", p);
	return p;
}
#endif

/*
 * make a fresh copy of p
 */
NODE *
tcopy(NODE *p)
{
	NODE *q;

	q = talloc();
	*q = *p;

	switch (optype(q->n_op)) {
	case BITYPE:
		q->n_right = tcopy(p->n_right);
	case UTYPE:
		q->n_left = tcopy(p->n_left);
	}

	return(q);
}

#ifndef LANG_F77
/*
 * ensure that all nodes have been freed
 */
void
tcheck()
{
	extern int inlnodecnt;

	if (nerrors)
		return;

	if ((usednodes - inlnodecnt) != 0)
		cerror("usednodes == %d, inlnodecnt %d", usednodes, inlnodecnt);
}
#endif

/*
 * free the tree p
 */
void
tfree(NODE *p)
{
	if (p->n_op != FREE)
		walkf(p, (void (*)(NODE *, void *))nfree, 0);
}

/*
 * Free a node, and return its left descendant.
 * It is up to the caller to know whether the return value is usable.
 */
NODE *
nfree(NODE *p)
{
	NODE *l;
#ifdef PCC_DEBUG_NODES
	NODE *q;
#endif

	if (p == NULL)
		cerror("freeing blank node!");
		
	l = p->n_left;
	if (p->n_op == FREE)
		cerror("freeing FREE node", p);
#ifdef PCC_DEBUG_NODES
	q = freelink;
	while (q != NULL) {
		if (q == p)
			cerror("freeing free node %p", p);
		q = q->next;
	}
#endif

	if (nflag)
		printf("freeing node %p\n", p);
	p->n_op = FREE;
	p->next = freelink;
	freelink = p;
	usednodes--;
	return l;
}
#endif

#ifdef LANG_F77
#define OPTYPE(x) optype(x)
#else
#define OPTYPE(x) coptype(x)
#endif

#ifdef MKEXT
#define coptype(o)	(dope[o]&TYFLG)
#else
int cdope(int);
#define coptype(o)	(cdope(o)&TYFLG)
#endif

void
fwalk(NODE *t, void (*f)(NODE *, int, int *, int *), int down)
{

	int down1, down2;

	more:
	down1 = down2 = 0;

	(*f)(t, down, &down1, &down2);

	switch (OPTYPE( t->n_op )) {

	case BITYPE:
		fwalk( t->n_left, f, down1 );
		t = t->n_right;
		down = down2;
		goto more;

	case UTYPE:
		t = t->n_left;
		down = down1;
		goto more;

	}
}

void
walkf(NODE *t, void (*f)(NODE *, void *), void *arg)
{
	int opty;


	opty = OPTYPE(t->n_op);

	if (opty != LTYPE)
		walkf( t->n_left, f, arg );
	if (opty == BITYPE)
		walkf( t->n_right, f, arg );
	(*f)(t, arg);
}

int dope[DSIZE];
char *opst[DSIZE];

struct dopest {
	int dopeop;
	char opst[8];
	int dopeval;
} indope[] = {
	{ NAME, "NAME", LTYPE, },
	{ REG, "REG", LTYPE, },
	{ OREG, "OREG", LTYPE, },
	{ TEMP, "TEMP", LTYPE, },
	{ ICON, "ICON", LTYPE, },
	{ FCON, "FCON", LTYPE, },
	{ CCODES, "CCODES", LTYPE, },
	{ UMINUS, "U-", UTYPE, },
	{ UMUL, "U*", UTYPE, },
	{ FUNARG, "FUNARG", UTYPE, },
	{ UCALL, "UCALL", UTYPE|CALLFLG, },
	{ UFORTCALL, "UFCALL", UTYPE|CALLFLG, },
	{ COMPL, "~", UTYPE, },
	{ FORCE, "FORCE", UTYPE, },
	{ XARG, "XARG", UTYPE, },
	{ XASM, "XASM", BITYPE, },
	{ SCONV, "SCONV", UTYPE, },
	{ PCONV, "PCONV", UTYPE, },
	{ PLUS, "+", BITYPE|FLOFLG|SIMPFLG|COMMFLG, },
	{ MINUS, "-", BITYPE|FLOFLG|SIMPFLG, },
	{ MUL, "*", BITYPE|FLOFLG|MULFLG, },
	{ AND, "&", BITYPE|SIMPFLG|COMMFLG, },
	{ CM, ",", BITYPE, },
	{ ASSIGN, "=", BITYPE|ASGFLG, },
	{ DIV, "/", BITYPE|FLOFLG|MULFLG|DIVFLG, },
	{ MOD, "%", BITYPE|DIVFLG, },
	{ LS, "<<", BITYPE|SHFFLG, },
	{ RS, ">>", BITYPE|SHFFLG, },
	{ OR, "|", BITYPE|COMMFLG|SIMPFLG, },
	{ ER, "^", BITYPE|COMMFLG|SIMPFLG, },
	{ STREF, "->", BITYPE, },
	{ CALL, "CALL", BITYPE|CALLFLG, },
	{ FORTCALL, "FCALL", BITYPE|CALLFLG, },
	{ EQ, "==", BITYPE|LOGFLG, },
	{ NE, "!=", BITYPE|LOGFLG, },
	{ LE, "<=", BITYPE|LOGFLG, },
	{ LT, "<", BITYPE|LOGFLG, },
	{ GE, ">=", BITYPE|LOGFLG, },
	{ GT, ">", BITYPE|LOGFLG, },
	{ UGT, "UGT", BITYPE|LOGFLG, },
	{ UGE, "UGE", BITYPE|LOGFLG, },
	{ ULT, "ULT", BITYPE|LOGFLG, },
	{ ULE, "ULE", BITYPE|LOGFLG, },
	{ CBRANCH, "CBRANCH", BITYPE, },
	{ FLD, "FLD", UTYPE, },
	{ PMCONV, "PMCONV", BITYPE, },
	{ PVCONV, "PVCONV", BITYPE, },
	{ RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG, },
	{ GOTO, "GOTO", UTYPE, },
	{ STASG, "STASG", BITYPE|ASGFLG, },
	{ STARG, "STARG", UTYPE, },
	{ STCALL, "STCALL", BITYPE|CALLFLG, },
	{ USTCALL, "USTCALL", UTYPE|CALLFLG, },
	{ ADDROF, "U&", UTYPE, },

	{ -1,	"",	0 },
};

void
mkdope()
{
	struct dopest *q;

	for( q = indope; q->dopeop >= 0; ++q ){
		dope[q->dopeop] = q->dopeval;
		opst[q->dopeop] = q->opst;
	}
}

/*
 * output a nice description of the type of t
 */
void
tprint(FILE *fp, TWORD t, TWORD q)
{
	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"uchar",
		"short",
		"ushort",
		"int",
		"unsigned",
		"long",
		"ulong",
		"longlong",
		"ulonglong",
		"float",
		"double",
		"ldouble",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"void",
		"signed", /* pass1 */
		"bool", /* pass1 */
		"fimag", /* pass1 */
		"dimag", /* pass1 */
		"limag", /* pass1 */
		"fcomplex", /* pass1 */
		"dcomplex", /* pass1 */
		"lcomplex", /* pass1 */
		"enumty", /* pass1 */
		"?", "?"
		};

	for(;; t = DECREF(t), q = DECREF(q)) {
		if (ISCON(q))
			fputc('C', fp);
		if (ISVOL(q))
			fputc('V', fp);

		if (ISPTR(t))
			fprintf(fp, "PTR ");
		else if (ISFTN(t))
			fprintf(fp, "FTN ");
		else if (ISARY(t))
			fprintf(fp, "ARY ");
		else {
			fprintf(fp, "%s%s%s", ISCON(q << TSHIFT) ? "const " : "",
			    ISVOL(q << TSHIFT) ? "volatile " : "", tnames[t]);
			return;
		}
	}
}

/*
 * Memory allocation routines.
 * Memory are allocated from the system in MEMCHUNKSZ blocks.
 * permalloc() returns a bunch of memory that is never freed.
 * Memory allocated through tmpalloc() will be released the
 * next time a function is ended (via tmpfree()).
 */

#define	MEMCHUNKSZ 8192	/* 8k per allocation */
struct balloc {
	char a1;
	union {
		long long l;
		long double d;
	} a2;
};

#define ALIGNMENT ((long)&((struct balloc *)0)->a2)
#define	ROUNDUP(x) (((x) + ((ALIGNMENT)-1)) & ~((ALIGNMENT)-1))

static char *allocpole;
static int allocleft;
int permallocsize, tmpallocsize, lostmem;

void *
permalloc(int size)
{
	void *rv;

	if (size > MEMCHUNKSZ) {
		if ((rv = malloc(size)) == NULL)
			cerror("permalloc: missing %d bytes", size);
		return rv;
	}
	if (size <= 0)
		cerror("permalloc2");
	if (allocleft < size) {
		/* looses unused bytes */
		lostmem += allocleft;
		if ((allocpole = malloc(MEMCHUNKSZ)) == NULL)
			cerror("permalloc: out of memory");
		allocleft = MEMCHUNKSZ;
	}
	size = ROUNDUP(size);
	rv = &allocpole[MEMCHUNKSZ-allocleft];
	allocleft -= size;
	permallocsize += size;
	return rv;
}

void *
tmpcalloc(int size)
{
	void *rv;

	rv = tmpalloc(size);
	memset(rv, 0, size);
	return rv;
}

/*
 * Duplicate a string onto the temporary heap.
 */
char *
tmpstrdup(char *str)
{
	int len;

	len = strlen(str) + 1;
	return memcpy(tmpalloc(len), str, len);
}

/*
 * Allocation routines for temporary memory.
 */
#if 0
#define	ALLDEBUG(x)	printf x
#else
#define	ALLDEBUG(x)
#endif

#define	NELEM	((MEMCHUNKSZ-ROUNDUP(sizeof(struct xalloc *)))/ALIGNMENT)
#define	ELEMSZ	(ALIGNMENT)
#define	MAXSZ	(NELEM*ELEMSZ)
struct xalloc {
	struct xalloc *next;
	union {
		struct balloc b; /* for initial alignment */
		char elm[MAXSZ];
	} u;
} *tapole, *tmpole;
int uselem = NELEM; /* next unused element */

void *
tmpalloc(int size)
{
	struct xalloc *xp;
	void *rv;
	size_t nelem;

	nelem = ROUNDUP(size)/ELEMSZ;
	ALLDEBUG(("tmpalloc(%ld,%ld) %d (%zd) ", ELEMSZ, NELEM, size, nelem));
	if (nelem > NELEM/2) {
		xp = malloc(size + ROUNDUP(sizeof(struct xalloc *)));
		if (xp == NULL)
			cerror("out of memory");
		ALLDEBUG(("XMEM! (%ld,%p) ",
		    size + ROUNDUP(sizeof(struct xalloc *)), xp));
		xp->next = tmpole;
		tmpole = xp;
		ALLDEBUG(("rv %p\n", &xp->u.elm[0]));
		return &xp->u.elm[0];
	}
	if (nelem + uselem >= NELEM) {
		ALLDEBUG(("MOREMEM! "));
		/* alloc more */
		if ((xp = malloc(sizeof(struct xalloc))) == NULL)
			cerror("out of memory");
		xp->next = tapole;
		tapole = xp;
		uselem = 0;
	} else
		xp = tapole;
	rv = &xp->u.elm[uselem * ELEMSZ];
	ALLDEBUG(("elemno %d ", uselem));
	uselem += nelem;
	ALLDEBUG(("new %d rv %p\n", uselem, rv));
	return rv;
}

void
tmpfree()
{
	struct xalloc *x1;

	while (tmpole) {
		x1 = tmpole;
		tmpole = tmpole->next;
		ALLDEBUG(("XMEM! free %p\n", x1));
		free(x1);
	}
	while (tapole && tapole->next) {
		x1 = tapole;
		tapole = tapole->next;
		ALLDEBUG(("MOREMEM! free %p\n", x1));
		free(x1);
	}
	if (tapole)
		uselem = 0;
}

/*
 * Set a mark for later removal from the temp heap.
 */
void
markset(struct mark *m)
{
	m->tmsav = tmpole;
	m->tasav = tapole;
	m->elem = uselem;
}

/*
 * Remove everything on tmp heap from a mark.
 */
void
markfree(struct mark *m)
{
	struct xalloc *x1;

	while (tmpole != m->tmsav) {
		x1 = tmpole;
		tmpole = tmpole->next;
		free(x1);
	}
	while (tapole != m->tasav) {
		x1 = tapole;
		tapole = tapole->next;
		free(x1);
	}
	uselem = m->elem;
}

/*
 * Allocate space on the permanent stack for a string of length len+1
 * and copy it there.
 * Return the new address.
 */
char *
newstring(char *s, int len)
{
	char *u, *c;

	len++;
	if (allocleft < len) {
		u = c = permalloc(len);
	} else {
		u = c = &allocpole[MEMCHUNKSZ-allocleft];
		allocleft -= ROUNDUP(len+1);
	}
	while (len--)
		*c++ = *s++;
	return u;
}

/*
 * Do a preorder walk of the CM list p and apply function f on each element.
 */
void
flist(NODE *p, void (*f)(NODE *, void *), void *arg)
{
	if (p->n_op == CM) {
		(*f)(p->n_right, arg);
		flist(p->n_left, f, arg);
	} else
		(*f)(p, arg);
}

/*
 * The same as flist but postorder.
 */
void
listf(NODE *p, void (*f)(NODE *))
{
	if (p->n_op == CM) {
		listf(p->n_left, f);
		(*f)(p->n_right);
	} else
		(*f)(p);
}

/*
 * Get list argument number n from list, or NIL if out of list.
 */
NODE *
listarg(NODE *p, int n, int *cnt)
{
	NODE *r;

	if (p->n_op == CM) {
		r = listarg(p->n_left, n, cnt);
		if (n == ++(*cnt))
			r = p->n_right;
	} else {
		*cnt = 0;
		r = n == 0 ? p : NIL;
	}
	return r;
}
