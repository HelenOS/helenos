/*      $Id: gcc_compat.c,v 1.77 2011/02/01 14:20:02 ragge Exp $     */
/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Routines to support some of the gcc extensions to C.
 */
#ifdef GCC_COMPAT

#include "pass1.h"
#include "cgram.h"

#include <string.h>

static struct kw {
	char *name, *ptr;
	int rv;
} kw[] = {
/*
 * Do NOT change the order of these entries unless you know 
 * what you're doing!
 */
/* 0 */	{ "__asm", NULL, C_ASM },
/* 1 */	{ "__signed", NULL, 0 },
/* 2 */	{ "__inline", NULL, C_FUNSPEC },
/* 3 */	{ "__const", NULL, 0 },
/* 4 */	{ "__asm__", NULL, C_ASM },
/* 5 */	{ "__inline__", NULL, C_FUNSPEC },
/* 6 */	{ "__thread", NULL, 0 },
/* 7 */	{ "__FUNCTION__", NULL, 0 },
/* 8 */	{ "__volatile", NULL, 0 },
/* 9 */	{ "__volatile__", NULL, 0 },
/* 10 */{ "__restrict", NULL, -1 },
/* 11 */{ "__typeof__", NULL, C_TYPEOF },
/* 12 */{ "typeof", NULL, C_TYPEOF },
/* 13 */{ "__extension__", NULL, -1 },
/* 14 */{ "__signed__", NULL, 0 },
/* 15 */{ "__attribute__", NULL, 0 },
/* 16 */{ "__attribute", NULL, 0 },
/* 17 */{ "__real__", NULL, 0 },
/* 18 */{ "__imag__", NULL, 0 },
/* 19 */{ "__builtin_offsetof", NULL, PCC_OFFSETOF },
/* 20 */{ "__PRETTY_FUNCTION__", NULL, 0 },
/* 21 */{ "__alignof__", NULL, C_ALIGNOF },
/* 22 */{ "__typeof", NULL, C_TYPEOF },
/* 23 */{ "__alignof", NULL, C_ALIGNOF },
/* 24 */{ "__restrict__", NULL, -1 },
	{ NULL, NULL, 0 },
};

/* g77 stuff */
#if SZFLOAT == SZLONG
#define G77_INTEGER LONG
#define G77_UINTEGER ULONG
#elif SZFLOAT == SZINT
#define G77_INTEGER INT
#define G77_UINTEGER UNSIGNED
#else
#error fix g77 stuff
#endif
#if SZFLOAT*2 == SZLONG
#define G77_LONGINT LONG
#define G77_ULONGINT ULONG
#elif SZFLOAT*2 == SZLONGLONG
#define G77_LONGINT LONGLONG
#define G77_ULONGINT ULONGLONG
#else
#error fix g77 long stuff
#endif

static TWORD g77t[] = { G77_INTEGER, G77_UINTEGER, G77_LONGINT, G77_ULONGINT };
static char *g77n[] = { "__g77_integer", "__g77_uinteger",
	"__g77_longint", "__g77_ulongint" };

void
gcc_init()
{
	struct kw *kwp;
	NODE *p;
	TWORD t;
	int i;

	for (kwp = kw; kwp->name; kwp++)
		kwp->ptr = addname(kwp->name);

	for (i = 0; i < 4; i++) {
		struct symtab *sp;
		t = ctype(g77t[i]);
		p = block(NAME, NIL, NIL, t, NULL, MKAP(t));
		sp = lookup(addname(g77n[i]), 0);
		p->n_sp = sp;
		defid(p, TYPEDEF);
		nfree(p);
	}

}

#define	TS	"\n#pragma tls\n# %d\n"
#define	TLLEN	sizeof(TS)+10
/*
 * See if a string matches a gcc keyword.
 */
int
gcc_keyword(char *str, NODE **n)
{
	extern int inattr, parlvl, parbal;
	YYSTYPE *yyl = (YYSTYPE *)n; /* XXX should pass yylval */
	char tlbuf[TLLEN], *tw;
	struct kw *kwp;
	int i;

	/* XXX hack, should pass everything in expressions */
	if (str == kw[21].ptr)
		return kw[21].rv;

	if (inattr)
		return 0;

	for (i = 0, kwp = kw; kwp->name; kwp++, i++)
		if (str == kwp->ptr)
			break;
	if (kwp->name == NULL)
		return 0;
	if (kwp->rv)
		return kwp->rv;
	switch (i) {
	case 1:  /* __signed */
	case 14: /* __signed__ */
		*n = mkty((TWORD)SIGNED, 0, MKAP(SIGNED));
		return C_TYPE;
	case 3: /* __const */
		*n = block(QUALIFIER, NIL, NIL, CON, 0, 0);
		(*n)->n_qual = CON;
		return C_QUALIFIER;
	case 6: /* __thread */
		snprintf(tlbuf, TLLEN, TS, lineno);
		tw = &tlbuf[strlen(tlbuf)];
		while (tw > tlbuf)
			cunput(*--tw);
		return -1;
	case 7: /* __FUNCTION__ */
	case 20: /* __PRETTY_FUNCTION__ */
		if (cftnsp == NULL) {
			uerror("%s outside function", kwp->name);
			yylval.strp = "";
		} else
			yylval.strp = cftnsp->sname; /* XXX - not C99 */
		return C_STRING;
	case 8: /* __volatile */
	case 9: /* __volatile__ */
		*n = block(QUALIFIER, NIL, NIL, VOL, 0, 0);
		(*n)->n_qual = VOL;
		return C_QUALIFIER;
	case 15: /* __attribute__ */
	case 16: /* __attribute */
		inattr = 1;
		parlvl = parbal;
		return C_ATTRIBUTE;
	case 17: /* __real__ */
		yyl->intval = XREAL;
		return C_UNOP;
	case 18: /* __imag__ */
		yyl->intval = XIMAG;
		return C_UNOP;
	}
	cerror("gcc_keyword");
	return 0;
}

#ifndef TARGET_ATTR
#define	TARGET_ATTR(p, sue)		0
#endif
#ifndef	ALMAX
#define	ALMAX (ALLDOUBLE > ALLONGLONG ? ALLDOUBLE : ALLONGLONG)
#endif

/* allowed number of args */
#define	A_0ARG	0x01
#define	A_1ARG	0x02
#define	A_2ARG	0x04
#define	A_3ARG	0x08
/* arg # is a name */
#define	A1_NAME	0x10
#define	A2_NAME	0x20
#define	A3_NAME	0x40
#define	A_MANY	0x80
/* arg # is "string" */
#define	A1_STR	0x100
#define	A2_STR	0x200
#define	A3_STR	0x400

#ifdef __MSC__
#define	CS(x)
#else
#define CS(x) [x] =
#endif

struct atax {
	int typ;
	char *name;
} atax[GCC_ATYP_MAX] = {
	CS(ATTR_NONE)		{ 0, NULL },
	CS(ATTR_COMPLEX)	{ 0, NULL },
	CS(ATTR_BASETYP)	{ 0, NULL },
	CS(ATTR_QUALTYP)	{ 0, NULL },
	CS(ATTR_STRUCT)		{ 0, NULL },
	CS(GCC_ATYP_ALIGNED)	{ A_0ARG|A_1ARG, "aligned" },
	CS(GCC_ATYP_PACKED)	{ A_0ARG|A_1ARG, "packed" },
	CS(GCC_ATYP_SECTION)	{ A_1ARG|A1_STR, "section" },
	CS(GCC_ATYP_TRANSP_UNION) { A_0ARG, "transparent_union" },
	CS(GCC_ATYP_UNUSED)	{ A_0ARG, "unused" },
	CS(GCC_ATYP_DEPRECATED)	{ A_0ARG, "deprecated" },
	CS(GCC_ATYP_MAYALIAS)	{ A_0ARG, "may_alias" },
	CS(GCC_ATYP_MODE)	{ A_1ARG|A1_NAME, "mode" },
	CS(GCC_ATYP_NORETURN)	{ A_0ARG, "noreturn" },
	CS(GCC_ATYP_FORMAT)	{ A_3ARG|A1_NAME, "format" },
	CS(GCC_ATYP_NONNULL)	{ A_MANY, "nonnull" },
	CS(GCC_ATYP_SENTINEL)	{ A_0ARG|A_1ARG, "sentinel" },
	CS(GCC_ATYP_WEAK)	{ A_0ARG, "weak" },
	CS(GCC_ATYP_FORMATARG)	{ A_1ARG, "format_arg" },
	CS(GCC_ATYP_GNU_INLINE)	{ A_0ARG, "gnu_inline" },
	CS(GCC_ATYP_MALLOC)	{ A_0ARG, "malloc" },
	CS(GCC_ATYP_NOTHROW)	{ A_0ARG, "nothrow" },
	CS(GCC_ATYP_CONST)	{ A_0ARG, "const" },
	CS(GCC_ATYP_PURE)	{ A_0ARG, "pure" },
	CS(GCC_ATYP_CONSTRUCTOR) { A_0ARG, "constructor" },
	CS(GCC_ATYP_DESTRUCTOR)	{ A_0ARG, "destructor" },
	CS(GCC_ATYP_VISIBILITY)	{ A_1ARG|A1_STR, "visibility" },
	CS(GCC_ATYP_STDCALL)	{ A_0ARG, "stdcall" },
	CS(GCC_ATYP_CDECL)	{ A_0ARG, "cdecl" },
	CS(GCC_ATYP_WARN_UNUSED_RESULT) { A_0ARG, "warn_unused_result" },
	CS(GCC_ATYP_USED)	{ A_0ARG, "used" },
	CS(GCC_ATYP_NO_INSTR_FUN) { A_0ARG, "no_instrument_function" },
	CS(GCC_ATYP_NOINLINE)	{ A_0ARG, "noinline" },
	CS(GCC_ATYP_ALIAS)	{ A_1ARG|A1_STR, "alias" },
	CS(GCC_ATYP_WEAKREF)	{ A_0ARG|A_1ARG|A1_STR, "weakref" },
	CS(GCC_ATYP_ALLOCSZ)	{ A_1ARG|A_2ARG, "alloc_size" },
	CS(GCC_ATYP_ALW_INL)	{ A_0ARG, "always_inline" },
	CS(GCC_ATYP_TLSMODEL)	{ A_1ARG|A1_STR, "tls_model" },
	CS(GCC_ATYP_ALIASWEAK)	{ A_1ARG|A1_STR, "aliasweak" },

	CS(GCC_ATYP_BOUNDED)	{ A_3ARG|A_MANY|A1_NAME, "bounded" },
};

#if SZPOINT(CHAR) == SZLONGLONG
#define	GPT	LONGLONG
#else
#define	GPT	INT
#endif

struct atax mods[] = {
	{ 0, NULL },
	{ INT, "SI" },
	{ INT, "word" },
	{ GPT, "pointer" },
	{ CHAR, "byte" },
	{ CHAR, "QI" },
	{ SHORT, "HI" },
	{ LONGLONG, "DI" },
	{ FLOAT, "SF" },
	{ DOUBLE, "DF" },
	{ LDOUBLE, "XF" },
	{ FCOMPLEX, "SC" },
	{ COMPLEX, "DC" },
	{ LCOMPLEX, "XC" },
#ifdef TARGET_MODS
	TARGET_MODS
#endif
};
#define	ATSZ	(sizeof(mods)/sizeof(mods[0]))

static int
amatch(char *s, struct atax *at, int mx)
{
	int i, len;

	if (s[0] == '_' && s[1] == '_')
		s += 2;
	len = strlen(s);
	if (len > 2 && s[len-1] == '_' && s[len-2] == '_')
		len -= 2;
	for (i = 0; i < mx; i++) {
		char *t = at[i].name;
		if (t != NULL && strncmp(s, t, len) == 0 && t[len] == 0)
			return i;
	}
	return 0;
}

static void
setaarg(int str, union aarg *aa, NODE *p)
{
	if (str) {
		if (((str & (A1_STR|A2_STR|A3_STR)) && p->n_op != STRING) ||
		    ((str & (A1_NAME|A2_NAME|A3_NAME)) && p->n_op != NAME))
			uerror("bad arg to attribute");
		if (p->n_op == STRING) {
			aa->sarg = newstring(p->n_name, strlen(p->n_name)+1);
		} else
			aa->sarg = (char *)p->n_sp;
		nfree(p);
	} else
		aa->iarg = (int)icons(eve(p));
}

/*
 * Parse attributes from an argument list.
 */
static struct attr *
gcc_attribs(NODE *p)
{
	NODE *q, *r;
	struct attr *ap;
	char *name = NULL, *c;
	int cw, attr, narg, i;

	if (p->n_op == NAME) {
		name = (char *)p->n_sp;
	} else if (p->n_op == CALL || p->n_op == UCALL) {
		name = (char *)p->n_left->n_sp;
	} else if (p->n_op == ICON && p->n_type == STRTY) {
		return NULL;
	} else
		cerror("bad variable attribute");

	if ((attr = amatch(name, atax, GCC_ATYP_MAX)) == 0) {
		werror("unsupported attribute '%s'", name);
		ap = NULL;
		goto out;
	}
	narg = 0;
	if (p->n_op == CALL)
		for (narg = 1, q = p->n_right; q->n_op == CM; q = q->n_left)
			narg++;

	cw = atax[attr].typ;
	if (!(cw & A_MANY) && ((narg > 3) || ((cw & (1 << narg)) == 0))) {
		uerror("wrong attribute arg count");
		return NULL;
	}
	ap = attr_new(attr, 3); /* XXX should be narg */
	q = p->n_right;

	switch (narg) {
	default:
		/* XXX */
		while (narg-- > 3) {
			r = q;
			q = q->n_left;
			tfree(r->n_right);
			nfree(r);
		}
		/* FALLTHROUGH */
	case 3:
		setaarg(cw & (A3_NAME|A3_STR), &ap->aa[2], q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 2:
		setaarg(cw & (A2_NAME|A2_STR), &ap->aa[1], q->n_right);
		r = q;
		q = q->n_left;
		nfree(r);
		/* FALLTHROUGH */
	case 1:
		setaarg(cw & (A1_NAME|A1_STR), &ap->aa[0], q);
		p->n_op = UCALL;
		/* FALLTHROUGH */
	case 0:
		break;
	}

	/* some attributes must be massaged special */
	switch (attr) {
	case GCC_ATYP_ALIGNED:
		if (narg == 0)
			ap->aa[0].iarg = ALMAX;
		else
			ap->aa[0].iarg *= SZCHAR;
		break;
	case GCC_ATYP_PACKED:
		if (narg == 0)
			ap->aa[0].iarg = 1; /* bitwise align */
		else
			ap->aa[0].iarg *= SZCHAR;
		break;

	case GCC_ATYP_MODE:
		if ((i = amatch(ap->aa[0].sarg, mods, ATSZ)) == 0)
			werror("unknown mode arg %s", ap->aa[0].sarg);
		ap->aa[0].iarg = ctype(mods[i].typ);
		break;

	case GCC_ATYP_VISIBILITY:
		c = ap->aa[0].sarg;
		if (strcmp(c, "default") && strcmp(c, "hidden") &&
		    strcmp(c, "internal") && strcmp(c, "protected"))
			werror("unknown visibility %s", c);
		break;

	case GCC_ATYP_TLSMODEL:
		c = ap->aa[0].sarg;
		if (strcmp(c, "global-dynamic") && strcmp(c, "local-dynamic") &&
		    strcmp(c, "initial-exec") && strcmp(c, "local-exec"))
			werror("unknown tls model %s", c);
		break;

	default:
		break;
	}
out:
	return ap;
}

/*
 * Extract attributes from a node tree and return attribute entries 
 * based on its contents.
 */
struct attr *
gcc_attr_parse(NODE *p)
{
	struct attr *b, *c;

	if (p == NIL)
		return NULL;

	if (p->n_op != CM) {
		b = gcc_attribs(p);
		tfree(p);
	} else {
		b = gcc_attr_parse(p->n_left);
		c = gcc_attr_parse(p->n_right);
		nfree(p);
		b = b ? attr_add(b, c) : c;
	}
	return b;
}

/*
 * Fixup struct/unions depending on attributes.
 */
void
gcc_tcattrfix(NODE *p)
{
	struct symtab *sp;
	struct attr *ap;
	int sz, coff, csz, al;

	if ((ap = attr_find(p->n_ap, GCC_ATYP_PACKED)) == NULL)
		return; /* nothing to fix */

	al = ap->iarg(0);

	/* Must repack struct */
	coff = csz = 0;
	for (sp = strmemb(ap); sp; sp = sp->snext) {
		if (sp->sclass & FIELD)
			sz = sp->sclass&FLDSIZ;
		else
			sz = (int)tsize(sp->stype, sp->sdf, sp->sap);
		SETOFF(sz, al);
		sp->soffset = coff;
		coff += sz;
		if (coff > csz)
			csz = coff;
		if (p->n_type == UNIONTY)
			coff = 0;
	}
	SETOFF(csz, al); /* Roundup to whatever */

	ap = attr_find(p->n_ap, ATTR_BASETYP);
	ap->atypsz = csz;
	ap->aalign = al;
}

/*
 * gcc-specific pragmas.
 */
int
pragmas_gcc(char *t)
{
	int ign, warn, err, i, u;
	extern bittype warnary[], werrary[];
	extern char *flagstr[], *pragstore;

	if (strcmp((t = pragtok(NULL)), "diagnostic") == 0) {
		ign = warn = err = 0;
		if (strcmp((t = pragtok(NULL)), "ignored") == 0)
			ign = 1;
		else if (strcmp(t, "warning") == 0)
			warn = 1;
		else if (strcmp(t, "error") == 0)
			err = 1;
		else
			return 1;
		if (eat('\"') || eat('-'))
			return 1;
		for (t = pragstore; *t && *t != '\"'; t++)
			;
		u = *t;
		*t = 0;
		for (i = 0; i < NUMW; i++) {
			if (strcmp(flagstr[i], pragstore+1) != 0)
				continue;
			if (err) {
				BITSET(warnary, i);
				BITSET(werrary, i);
			} else if (warn) {
				BITSET(warnary, i);
				BITCLEAR(werrary, i);
			} else {
				BITCLEAR(warnary, i);
				BITCLEAR(werrary, i);
			}
			return 0;
		}
		*t = u;
	} else if (strcmp(t, "poison") == 0) {
		/* currently ignore */;
	} else if (strcmp(t, "visibility") == 0) {
		/* currently ignore */;
	} else
		werror("gcc pragma unsupported");
	return 0;
}

#ifdef PCC_DEBUG
void
dump_attr(struct attr *ap)
{
	printf("attributes; ");
	for (; ap; ap = ap->next) {
		if (ap->atype >= GCC_ATYP_MAX) {
			printf("bad type %d, ", ap->atype);
		} else if (atax[ap->atype].name == 0) {
			char *c = ap->atype == ATTR_COMPLEX ? "complex" :
			    ap->atype == ATTR_BASETYP ? "basetyp" :
			    ap->atype == ATTR_STRUCT ? "struct" : "badtype";
			printf("%s, ", c);
		} else {
			printf("%s: ", atax[ap->atype].name);
			printf("%d %d %d, ", ap->iarg(0),
			    ap->iarg(1), ap->iarg(2));
		}
	}
	printf("\n");
}
#endif
#endif
