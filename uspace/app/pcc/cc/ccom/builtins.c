/*	$Id: builtins.c,v 1.18.2.2 2011/03/29 15:56:24 ragge Exp $	*/
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

# include "pass1.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef NO_C_BUILTINS
/*
 * replace an alloca function with direct allocation on stack.
 * return a destination temp node.
 */
static NODE *
builtin_alloca(NODE *f, NODE *a, TWORD rt)
{
	struct symtab *sp;
	NODE *t, *u;

#ifdef notyet
	if (xnobuiltins)
		return NULL;
#endif
	sp = f->n_sp;

	t = tempnode(0, VOID|PTR, 0, MKAP(INT) /* XXX */);
	u = tempnode(regno(t), VOID|PTR, 0, MKAP(INT) /* XXX */);
	spalloc(t, a, SZCHAR);
	tfree(f);
	return u;
}

/*
 * See if there is a goto in the tree.
 * XXX this function is a hack for a flaw in handling of 
 * compound expressions and inline functions and should not be 
 * needed.
 */
static int
hasgoto(NODE *p)
{
	int o = coptype(p->n_op);

	if (o == LTYPE)
		return 0;
	if (p->n_op == GOTO)
		return 1;
	if (o == UTYPE)
		return hasgoto(p->n_left);
	if (hasgoto(p->n_left))
		return 1;
	return hasgoto(p->n_right);
}

/*
 * Determine if a value is known to be constant at compile-time and
 * hence that PCC can perform constant-folding on expressions involving
 * that value.
 */
static NODE *
builtin_constant_p(NODE *f, NODE *a, TWORD rt)
{
	void putjops(NODE *p, void *arg);
	int isconst;

	tfree(f);
	walkf(a, putjops, 0);
	for (f = a; f->n_op == COMOP; f = f->n_right)
		;
	isconst = nncon(f);
	tfree(a);
	return bcon(isconst);
}

/*
 * Hint to the compiler whether this expression will evaluate true or false.
 * Just ignored for now.
 */
static NODE *
builtin_expect(NODE *f, NODE *a, TWORD rt)
{

	tfree(f);
	if (a && a->n_op == CM) {
		tfree(a->n_right);
		f = a->n_left;
		nfree(a);
		a = f;
	}

	return a;
}

/*
 * Take integer absolute value.
 * Simply does: ((((x)>>(8*sizeof(x)-1))^(x))-((x)>>(8*sizeof(x)-1)))
 */
static NODE *
builtin_abs(NODE *f, NODE *a, TWORD rt)
{
	NODE *p, *q, *r, *t, *t2, *t3;
	int tmp1, tmp2, shift;

	if (a->n_type != INT)
		a = cast(a, INT, 0);

	tfree(f);

	if (a->n_op == ICON) {
		if (a->n_lval < 0)
			a->n_lval = -a->n_lval;
		p = a;
	} else {
		t = tempnode(0, a->n_type, a->n_df, a->n_ap);
		tmp1 = regno(t);
		p = buildtree(ASSIGN, t, a);

		t = tempnode(tmp1, a->n_type, a->n_df, a->n_ap);
		shift = (int)tsize(a->n_type, a->n_df, a->n_ap) - 1;
		q = buildtree(RS, t, bcon(shift));

		t2 = tempnode(0, a->n_type, a->n_df, a->n_ap);
		tmp2 = regno(t2);
		q = buildtree(ASSIGN, t2, q);

		t = tempnode(tmp1, a->n_type, a->n_df, a->n_ap);
		t2 = tempnode(tmp2, a->n_type, a->n_df, a->n_ap);
		t3 = tempnode(tmp2, a->n_type, a->n_df, a->n_ap);
		r = buildtree(MINUS, buildtree(ER, t, t2), t3);

		p = buildtree(COMOP, p, buildtree(COMOP, q, r));
	}

	return p;
}

/*
 * Get size of object, if possible.
 * Currently does nothing,
 */
static NODE *
builtin_object_size(NODE *f, NODE *a, TWORD rt)
{
	int v = icons(a->n_right);
	if (v < 0 || v > 3)
		uerror("arg2 must be between 0 and 3");

	tfree(f);
	f = buildtree(COMOP, a->n_left, xbcon(v < 2 ? -1 : 0, NULL, rt));
	nfree(a);
	return f;
}

#ifndef TARGET_STDARGS
static NODE *
builtin_stdarg_start(NODE *f, NODE *a, TWORD rt)
{
	NODE *p, *q;
	int sz;

	/* must first deal with argument size; use int size */
	p = a->n_right;
	if (p->n_type < INT) {
		sz = (int)(SZINT/tsize(p->n_type, p->n_df, p->n_ap));
	} else
		sz = 1;

	/* do the real job */
	p = buildtree(ADDROF, p, NIL); /* address of last arg */
#ifdef BACKAUTO
	p = optim(buildtree(PLUS, p, bcon(sz))); /* add one to it (next arg) */
#else
	p = optim(buildtree(MINUS, p, bcon(sz))); /* add one to it (next arg) */
#endif
	q = block(NAME, NIL, NIL, PTR+VOID, 0, 0); /* create cast node */
	q = buildtree(CAST, q, p); /* cast to void * (for assignment) */
	p = q->n_right;
	nfree(q->n_left);
	nfree(q);
	p = buildtree(ASSIGN, a->n_left, p); /* assign to ap */
	tfree(f);
	nfree(a);
	return p;
}

static NODE *
builtin_va_arg(NODE *f, NODE *a, TWORD rt)
{
	NODE *p, *q, *r, *rv;
	int sz, nodnum;

	/* create a copy to a temp node of current ap */
	p = ccopy(a->n_left);
	q = tempnode(0, p->n_type, p->n_df, p->n_ap);
	nodnum = regno(q);
	rv = buildtree(ASSIGN, q, p);

	r = a->n_right;
	sz = (int)tsize(r->n_type, r->n_df, r->n_ap)/SZCHAR;
	/* add one to ap */
#ifdef BACKAUTO
	rv = buildtree(COMOP, rv , buildtree(PLUSEQ, a->n_left, bcon(sz)));
#else
#error fix wrong eval order in builtin_va_arg
	ecomp(buildtree(MINUSEQ, a->n_left, bcon(sz)));
#endif

	nfree(a->n_right);
	nfree(a);
	nfree(f);
	r = tempnode(nodnum, INCREF(r->n_type), r->n_df, r->n_ap);
	return buildtree(COMOP, rv, buildtree(UMUL, r, NIL));

}

static NODE *
builtin_va_end(NODE *f, NODE *a, TWORD rt)
{
	tfree(f);
	tfree(a);
	return bcon(0); /* nothing */
}

static NODE *
builtin_va_copy(NODE *f, NODE *a, TWORD rt)
{
	tfree(f);
	f = buildtree(ASSIGN, a->n_left, a->n_right);
	nfree(a);
	return f;
}
#endif /* TARGET_STDARGS */

/*
 * For unimplemented "builtin" functions, try to invoke the
 * non-builtin name
 */
static NODE *
binhelp(NODE *f, NODE *a, TWORD rt, char *n)
{
	f->n_sp = lookup(addname(n), SNORMAL);
	if (f->n_sp->sclass == SNULL) {
		f->n_sp->sclass = EXTERN;
		f->n_sp->stype = INCREF(rt)+(FTN-PTR);
	}
	f->n_type = f->n_sp->stype;
	f = clocal(f);
	return buildtree(CALL, f, a);
}

static NODE *
builtin_unimp(NODE *f, NODE *a, TWORD rt)
{
	char *n = f->n_sp->sname;

	if (strncmp("__builtin_", n, 10) == 0)
		n += 10;
	return binhelp(f, a, rt, n);
}

static NODE *
builtin_unimp_f(NODE *f, NODE *a, TWORD rt)
{
	return binhelp(f, a, rt, f->n_sp->sname);
}

#ifndef TARGET_ISMATH
/*
 * Handle the builtin macros for the math functions is*
 * To get something that is be somewhat generic assume that 
 * isnan() is a real function and that cast of a NaN type 
 * to double will still be a NaN.
 */
static NODE *
mtisnan(NODE *p)
{
	NODE *q = block(NAME, NIL, NIL, INT, 0, MKAP(INT));

	return binhelp(q, cast(ccopy(p), DOUBLE, 0), INT, "isnan");
}

static TWORD
mtcheck(NODE *p)
{
	TWORD t1 = p->n_left->n_type, t2 = p->n_right->n_type;

	if ((t1 >= FLOAT && t1 <= LDOUBLE) ||
	    (t2 >= FLOAT && t2 <= LDOUBLE))
		return MAX(t1, t2);
	return 0;
}

static NODE *
builtin_isunordered(NODE *f, NODE *a, TWORD rt)
{
	NODE *p;

	if (mtcheck(a) == 0)
		return bcon(0);

	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	tfree(f);
	tfree(a);
	return p;
}
static NODE *
builtin_isany(NODE *f, NODE *a, TWORD rt, int cmpt)
{
	NODE *p, *q;
	TWORD t;

	if ((t = mtcheck(a)) == 0)
		return bcon(0);
	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	p = buildtree(NOT, p, NIL);
	q = buildtree(cmpt, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	p = buildtree(ANDAND, p, q);
	tfree(f);
	tfree(a);
	return p;
}
static NODE *
builtin_isgreater(NODE *f, NODE *a, TWORD rt)
{
	return builtin_isany(f, a, rt, GT);
}
static NODE *
builtin_isgreaterequal(NODE *f, NODE *a, TWORD rt)
{
	return builtin_isany(f, a, rt, GE);
}
static NODE *
builtin_isless(NODE *f, NODE *a, TWORD rt)
{
	return builtin_isany(f, a, rt, LT);
}
static NODE *
builtin_islessequal(NODE *f, NODE *a, TWORD rt)
{
	return builtin_isany(f, a, rt, LE);
}
static NODE *
builtin_islessgreater(NODE *f, NODE *a, TWORD rt)
{
	NODE *p, *q, *r;
	TWORD t;

	if ((t = mtcheck(a)) == 0)
		return bcon(0);
	p = buildtree(OROR, mtisnan(a->n_left), mtisnan(a->n_right));
	p = buildtree(NOT, p, NIL);
	q = buildtree(GT, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	r = buildtree(LT, cast(ccopy(a->n_left), t, 0),
	    cast(ccopy(a->n_right), t, 0));
	q = buildtree(OROR, q, r);
	p = buildtree(ANDAND, p, q);
	tfree(f);
	tfree(a);
	return p;
}
#endif

/*
 * Math-specific builtins that expands to constants.
 * Versins here is for IEEE FP, vax needs its own versions.
 */
#ifdef RTOLBYTES
static char vFLOAT[] = { 0, 0, 0x80, 0x7f };
static char vDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
#ifdef LDBL_128
static char vLDOUBLE[] = { 0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f };
#else /* LDBL_80 */
static char vLDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f };
#endif
static char nFLOAT[] = { 0, 0, 0xc0, 0x7f };
static char nDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f };
#ifdef LDBL_128
static char nLDOUBLE[] = { 0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0x7f };
#else /* LDBL_80 */
static char nLDOUBLE[] = { 0, 0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0x7f, 0, 0 };
#endif
#else
static char vFLOAT[] = { 0x7f, 0x80, 0, 0 };
static char vDOUBLE[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
#ifdef LDBL_128
static char vLDOUBLE[] = { 0x7f, 0xff, 0x80, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#else /* LDBL_80 */
static char vLDOUBLE[] = { 0x7f, 0xff, 0x80, 0, 0, 0, 0, 0, 0, 0 };
#endif
static char nFLOAT[] = { 0x7f, 0xc0, 0, 0 };
static char nDOUBLE[] = { 0x7f, 0xf8, 0, 0, 0, 0, 0, 0 };
#ifdef LDBL_128
static char nLDOUBLE[] = { 0x7f, 0xff, 0xc0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0 };
#else /* LDBL_80 */
static char nLDOUBLE[] = { 0x7f, 0xff, 0xc0, 0, 0, 0, 0, 0, 0, 0 };
#endif
#endif

#define VALX(typ,TYP) {						\
	typ d;							\
	int x;							\
	x = MIN(sizeof(n ## TYP), sizeof(d));			\
	memcpy(&d, v ## TYP, x);				\
	nfree(f);						\
	f = block(FCON, NIL, NIL, TYP, NULL, MKAP(TYP));	\
	f->n_dcon = d;						\
	return f;						\
}

static NODE *
builtin_huge_valf(NODE *f, NODE *a, TWORD rt) VALX(float,FLOAT)
static NODE *
builtin_huge_val(NODE *f, NODE *a, TWORD rt) VALX(double,DOUBLE)
static NODE *
builtin_huge_vall(NODE *f, NODE *a, TWORD rt) VALX(long double,LDOUBLE)

#define	builtin_inff	builtin_huge_valf
#define	builtin_inf	builtin_huge_val
#define	builtin_infl	builtin_huge_vall

#define	NANX(typ,TYP) {							\
	typ d;								\
	int x;								\
	if ((a->n_op == ICON && a->n_sp && a->n_sp->sname[0] == '\0') ||\
	    (a->n_op == ADDROF && a->n_left->n_op == NAME && 		\
	     a->n_left->n_sp && a->n_left->n_sp->sname[0] == '\0')) {	\
		x = MIN(sizeof(n ## TYP), sizeof(d));			\
		memcpy(&d, n ## TYP, x);				\
		tfree(a); tfree(f);					\
		f = block(FCON, NIL, NIL, TYP, NULL, MKAP(TYP));	\
		f->n_dcon = d;						\
		return f;						\
	}								\
	return buildtree(CALL, f, a);					\
}

/*
 * Return NANs, if reasonable.
 */
static NODE *
builtin_nanf(NODE *f, NODE *a, TWORD rt) NANX(float,FLOAT)
static NODE *
builtin_nan(NODE *f, NODE *a, TWORD rt) NANX(double,DOUBLE)
static NODE *
builtin_nanl(NODE *f, NODE *a, TWORD rt) NANX(long double,LDOUBLE)

/*
 * Target defines, to implement target versions of the generic builtins
 */
#ifndef TARGET_MEMCMP
#define	builtin_memcmp builtin_unimp
#endif
#ifndef TARGET_MEMCPY
#define	builtin_memcpy builtin_unimp
#endif
#ifndef TARGET_MEMSET
#define	builtin_memset builtin_unimp
#endif

/* Reasonable type of size_t */
#ifndef SIZET
#if SZINT == SZSHORT
#define	SIZET UNSIGNED
#elif SZLONG > SZINT
#define SIZET ULONG
#else
#define SIZET UNSIGNED
#endif
#endif

static TWORD memcpyt[] = { VOID|PTR, VOID|PTR, SIZET, INT };
static TWORD memsett[] = { VOID|PTR, INT, SIZET, INT };
static TWORD allocat[] = { SIZET };
static TWORD expectt[] = { LONG, LONG };
static TWORD strcmpt[] = { CHAR|PTR, CHAR|PTR };
static TWORD strcpyt[] = { CHAR|PTR, CHAR|PTR, INT };
static TWORD strncpyt[] = { CHAR|PTR, CHAR|PTR, SIZET, INT };
static TWORD strchrt[] = { CHAR|PTR, INT };
static TWORD strcspnt[] = { CHAR|PTR, CHAR|PTR };
static TWORD nant[] = { CHAR|PTR };
static TWORD bitt[] = { UNSIGNED };
static TWORD bitlt[] = { ULONG };
static TWORD ffst[] = { INT };

static const struct bitable {
	char *name;
	NODE *(*fun)(NODE *f, NODE *a, TWORD);
	int narg;
	TWORD *tp;
	TWORD rt;
} bitable[] = {
	{ "__builtin___memcpy_chk", builtin_unimp, 4, memcpyt, VOID|PTR },
	{ "__builtin___memmove_chk", builtin_unimp, 4, memcpyt, VOID|PTR },
	{ "__builtin___memset_chk", builtin_unimp, 4, memsett, VOID|PTR },

	{ "__builtin___strcat_chk", builtin_unimp, 3, strcpyt, CHAR|PTR },
	{ "__builtin___strcpy_chk", builtin_unimp, 3, strcpyt, CHAR|PTR },
	{ "__builtin___strncat_chk", builtin_unimp, 4, strncpyt,CHAR|PTR },
	{ "__builtin___strncpy_chk", builtin_unimp, 4, strncpyt,CHAR|PTR },

	{ "__builtin___printf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___fprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___sprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___snprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___vprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___vfprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___vsprintf_chk", builtin_unimp, -1, 0, INT },
	{ "__builtin___vsnprintf_chk", builtin_unimp, -1, 0, INT },

	{ "__builtin_alloca", builtin_alloca, 1, allocat },
	{ "__builtin_abs", builtin_abs, 1 },
	{ "__builtin_clz", builtin_unimp_f, 1, bitt, INT },
	{ "__builtin_ctz", builtin_unimp_f, 1, bitt, INT },
	{ "__builtin_clzl", builtin_unimp_f, 1, bitlt, INT },
	{ "__builtin_ctzl", builtin_unimp_f, 1, bitlt, INT },
	{ "__builtin_ffs", builtin_unimp, 1, ffst, INT },

	{ "__builtin_constant_p", builtin_constant_p, 1 },
	{ "__builtin_expect", builtin_expect, 2, expectt },
	{ "__builtin_memcmp", builtin_memcmp, 3, memcpyt, INT },
	{ "__builtin_memcpy", builtin_memcpy, 3, memcpyt, VOID|PTR },
	{ "__builtin_memset", builtin_memset, 3, memsett, VOID|PTR },
	{ "__builtin_huge_valf", builtin_huge_valf, 0 },
	{ "__builtin_huge_val", builtin_huge_val, 0 },
	{ "__builtin_huge_vall", builtin_huge_vall, 0 },
	{ "__builtin_inff", builtin_inff, 0 },
	{ "__builtin_inf", builtin_inf, 0 },
	{ "__builtin_infl", builtin_infl, 0 },
	{ "__builtin_isgreater", builtin_isgreater, 2, NULL, INT },
	{ "__builtin_isgreaterequal", builtin_isgreaterequal, 2, NULL, INT },
	{ "__builtin_isless", builtin_isless, 2, NULL, INT },
	{ "__builtin_islessequal", builtin_islessequal, 2, NULL, INT },
	{ "__builtin_islessgreater", builtin_islessgreater, 2, NULL, INT },
	{ "__builtin_isunordered", builtin_isunordered, 2, NULL, INT },
	{ "__builtin_nanf", builtin_nanf, 1, nant, FLOAT },
	{ "__builtin_nan", builtin_nan, 1, nant, DOUBLE },
	{ "__builtin_nanl", builtin_nanl, 1, nant, LDOUBLE },
	{ "__builtin_object_size", builtin_object_size, 2, memsett, SIZET },
	{ "__builtin_strcmp", builtin_unimp, 2, strcmpt, INT },
	{ "__builtin_strcpy", builtin_unimp, 2, strcmpt, CHAR|PTR },
	{ "__builtin_strchr", builtin_unimp, 2, strchrt, CHAR|PTR },
	{ "__builtin_strlen", builtin_unimp, 1, strcmpt, SIZET },
	{ "__builtin_strrchr", builtin_unimp, 2, strchrt, CHAR|PTR },
	{ "__builtin_strncpy", builtin_unimp, 3, strncpyt, CHAR|PTR },
	{ "__builtin_strncat", builtin_unimp, 3, strncpyt, CHAR|PTR },
	{ "__builtin_strcspn", builtin_unimp, 2, strcspnt, SIZET },
#ifndef TARGET_STDARGS
	{ "__builtin_stdarg_start", builtin_stdarg_start, 2 },
	{ "__builtin_va_start", builtin_stdarg_start, 2 },
	{ "__builtin_va_arg", builtin_va_arg, 2 },
	{ "__builtin_va_end", builtin_va_end, 1 },
	{ "__builtin_va_copy", builtin_va_copy, 2 },
#endif
#ifdef TARGET_BUILTINS
	TARGET_BUILTINS
#endif
};

/*
 * Check and cast arguments for builtins.
 */
static int
acnt(NODE *a, int narg, TWORD *tp)
{
	NODE *q;
	TWORD t;

	if (a == NIL)
		return narg;
	for (; a->n_op == CM; a = a->n_left, narg--) {
		if (tp == NULL)
			continue;
		q = a->n_right;
		t = tp[narg-1];
		if (q->n_type == t)
			continue;
		a->n_right = ccast(q, t, 0, NULL, MKAP(BTYPE(t)));
	}

	/* Last arg is ugly to deal with */
	if (narg == 1 && tp != NULL) {
		q = talloc();
		*q = *a;
		q = ccast(q, tp[0], 0, NULL, MKAP(BTYPE(tp[0])));
		*a = *q;
		nfree(q);
	}
	return narg != 1;
}

NODE *
builtin_check(NODE *f, NODE *a)
{
	const struct bitable *bt;
	int i;

	for (i = 0; i < (int)(sizeof(bitable)/sizeof(bitable[0])); i++) {
		bt = &bitable[i];
		if (strcmp(bt->name, f->n_sp->sname))
			continue;
		if (bt->narg >= 0 && acnt(a, bt->narg, bt->tp)) {
			uerror("wrong argument count to %s", bt->name);
			return bcon(0);
		}
		return (*bt->fun)(f, a, bt->rt);
	}
	return NIL;
}
#endif
