/*	$Id: code.c,v 1.56.2.1 2011/03/01 17:33:24 ragge Exp $	*/
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


# include "pass1.h"

int lastloc = -1;

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	extern char *nextsect;
	struct attr *ap;
	int weak = 0;
	char *name = NULL;
#if defined(ELFABI) || defined(PECOFFABI)
	static char *loctbl[] = { "text", "data", "section .rodata" };
#elif defined(MACHOABI)
	static char *loctbl[] = { "text", "data", "const_data" };
#endif
	TWORD t;
	int s, al;

	if (sp == NULL) {
		lastloc = -1;
		return;
	}
	t = sp->stype;
	s = ISFTN(t) ? PROG : ISCON(cqual(t, sp->squal)) ? RDATA : DATA;
	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);
#ifdef TLS
	if (sp->sflags & STLS) {
		if (s != DATA)
			cerror("non-data symbol in tls section");
		nextsect = ".tdata";
	}
#endif
#ifdef GCC_COMPAT
	{
		if ((ap = attr_find(sp->sap, GCC_ATYP_SECTION)) != NULL)
			nextsect = ap->sarg(0);
		if (attr_find(sp->sap, GCC_ATYP_WEAK) != NULL)
			weak = 1;
		if (attr_find(sp->sap, GCC_ATYP_DESTRUCTOR)) {
			printf("\t.section\t.dtors,\"aw\",@progbits\n");
			printf("\t.align 4\n\t.long\t%s\n", name);
			lastloc = -1;
		}
		if (attr_find(sp->sap, GCC_ATYP_CONSTRUCTOR)) {
			printf("\t.section\t.ctors,\"aw\",@progbits\n");
			printf("\t.align 4\n\t.long\t%s\n", name);
			lastloc = -1;
		}
		if ((ap = attr_find(sp->sap, GCC_ATYP_VISIBILITY)) &&
		    strcmp(ap->sarg(0), "default"))
			printf("\t.%s %s\n", ap->sarg(0), name);
	}
#endif
#ifdef ELFABI
	if (kflag && !ISFTN(t)) {
		/* Must place aggregates with pointers in relocatable memory */
		TWORD t2 = t;

		while (ISARY(t2))
			t2 = DECREF(t2);
		if (t2 > LDOUBLE) {
			/* put in reloc memory */
			printf("\t.section .data.rel.local,\"aw\",@progbits\n");
			s = lastloc = -1;
		}
	}
#endif
	if (nextsect) {
		printf("	.section %s,\"wa\",@progbits\n", nextsect);
		nextsect = NULL;
		s = -1;
	} else if (s != lastloc)
		printf("	.%s\n", loctbl[s]);
	lastloc = s;
	while (ISARY(t))
		t = DECREF(t);
	al = ISFTN(t) ? ALINT : talign(t, sp->sap);
	if (al > ALCHAR)
		printf("	.align %d\n", al/ALCHAR);
	if (weak)
		printf("	.weak %s\n", name);
	else if (sp->sclass == EXTDEF) {
		printf("	.globl %s\n", name);
#if defined(ELFABI)
		printf("\t.type %s,@%s\n", name,
		    ISFTN(t)? "function" : "object");
#endif
	}
#if defined(ELFABI)
	if (!ISFTN(t)) {
		if (sp->slevel == 0)
			printf("\t.size %s,%d\n", name,
			    (int)tsize(t, sp->sdf, sp->sap)/SZCHAR);
		else
			printf("\t.size " LABFMT ",%d\n", sp->soffset,
			    (int)tsize(t, sp->sdf, sp->sap)/SZCHAR);
	}
#endif
	if (sp->slevel == 0)
		printf("%s:\n", name);
	else
		printf(LABFMT ":\n", sp->soffset);
}

/*
 * code for the end of a function
 * deals with struct return here
 */
void
efcode()
{
	extern int gotnr;
	NODE *p, *q;

	gotnr = 0;	/* new number for next fun */
	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;
#if defined(os_openbsd)
	/* struct return for small structs */
	int sz = tsize(BTYPE(cftnsp->stype), cftnsp->sdf, cftnsp->sap);
	if (sz == SZCHAR || sz == SZSHORT || sz == SZINT || sz == SZLONGLONG) {
		/* Pointer to struct in eax */
		if (sz == SZLONGLONG) {
			q = block(OREG, NIL, NIL, INT, 0, MKAP(INT));
			q->n_lval = 4;
			p = block(REG, NIL, NIL, INT, 0, MKAP(INT));
			p->n_rval = EDX;
			ecomp(buildtree(ASSIGN, p, q));
		}
		if (sz < SZSHORT) sz = CHAR;
		else if (sz > SZSHORT) sz = INT;
		else sz = SHORT;
		q = block(OREG, NIL, NIL, sz, 0, MKAP(sz));
		p = block(REG, NIL, NIL, sz, 0, MKAP(sz));
		ecomp(buildtree(ASSIGN, p, q));
		return;
	}
#endif
	/* Create struct assignment */
	q = block(OREG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	q->n_rval = EBP;
	q->n_lval = 8; /* return buffer offset */
	q = buildtree(UMUL, q, NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* put hidden arg in eax on return */
	q = block(OREG, NIL, NIL, INT, 0, MKAP(INT));
	regno(q) = FPREG;
	q->n_lval = 8;
	p = block(REG, NIL, NIL, INT, 0, MKAP(INT));
	regno(p) = EAX;
	ecomp(buildtree(ASSIGN, p, q));
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **sp, int cnt)
{
	extern int argstacksize;
	struct symtab *sp2;
	extern int gotnr;
	NODE *n, *p;
	int i;

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		/* Function returns struct, adjust arg offset */
#if defined(os_openbsd)
		/* OpenBSD uses non-standard return for small structs */
		int sz = tsize(BTYPE(cftnsp->stype), cftnsp->sdf, cftnsp->sap);
		if (sz != SZCHAR && sz != SZSHORT &&
		    sz != SZINT && sz != SZLONGLONG)
#endif
			for (i = 0; i < cnt; i++) 
				sp[i]->soffset += SZPOINT(INT);
	}

#ifdef GCC_COMPAT
	if (attr_find(cftnsp->sap, GCC_ATYP_STDCALL) != NULL)
		cftnsp->sflags |= SSTDCALL;
#endif

	/*
	 * Count the arguments
	 */
	argstacksize = 0;
	if (cftnsp->sflags & SSTDCALL) {
#ifdef os_win32

		char buf[256];
		char *name;
#endif

		for (i = 0; i < cnt; i++) {
			TWORD t = sp[i]->stype;
			if (t == STRTY || t == UNIONTY)
				argstacksize +=
				    tsize(t, sp[i]->sdf, sp[i]->sap);
			else
				argstacksize += szty(t) * SZINT / SZCHAR;
		}
#ifdef os_win32
		/*
		 * mangle name in symbol table as a callee.
		 */
		if ((name = cftnsp->soname) == NULL)
			name = exname(cftnsp->sname);
		snprintf(buf, 256, "%s@%d", name, argstacksize);
		cftnsp->soname = addname(buf);
#endif
	}

	if (kflag) {
#define	STL	200
		char *str = inlalloc(STL);
#if !defined(MACHOABI)
		int l = getlab();
#else
		char *name;
#endif

		/* Generate extended assembler for PIC prolog */
		p = tempnode(0, INT, 0, MKAP(INT));
		gotnr = regno(p);
		p = block(XARG, p, NIL, INT, 0, MKAP(INT));
		p->n_name = "=g";
		p = block(XASM, p, bcon(0), INT, 0, MKAP(INT));

#if defined(MACHOABI)
		if ((name = cftnsp->soname) == NULL)
			name = cftnsp->sname;
		if (snprintf(str, STL, "call L%s$pb\nL%s$pb:\n\tpopl %%0\n",
		    name, name) >= STL)
			cerror("bfcode");
#else
		if (snprintf(str, STL,
		    "call " LABFMT "\n" LABFMT ":\n	popl %%0\n"
		    "	addl $_GLOBAL_OFFSET_TABLE_+[.-" LABFMT "], %%0\n",
		    l, l, l) >= STL)
			cerror("bfcode");
#endif
		p->n_name = str;
		p->n_right->n_type = STRTY;
		ecomp(p);
	}
	if (xtemps == 0)
		return;

	/* put arguments in temporaries */
	for (i = 0; i < cnt; i++) {
		if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY ||
		    cisreg(sp[i]->stype) == 0)
			continue;
		if (cqual(sp[i]->stype, sp[i]->squal) & VOL)
			continue;
		sp2 = sp[i];
		n = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
		n = buildtree(ASSIGN, n, nametree(sp2));
		sp[i]->soffset = regno(n->n_left);
		sp[i]->sflags |= STNODE;
		ecomp(n);
	}
}


/*
 * by now, the automatics and register variables are allocated
 */
void
bccode()
{
	SETOFF(autooff, SZINT);
}

#if defined(MACHOABI)
struct stub stublist;
struct stub nlplist;
#endif

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag )
{
#if defined(MACHOABI)
	/*
	 * iterate over the stublist and output the PIC stubs
`	 */
	if (kflag) {
		struct stub *p;

		DLIST_FOREACH(p, &stublist, link) {
			printf("\t.section __IMPORT,__jump_table,symbol_stubs,self_modifying_code+pure_instructions,5\n");
			printf("L%s$stub:\n", p->name);
			printf("\t.indirect_symbol %s\n", p->name);
			printf("\thlt ; hlt ; hlt ; hlt ; hlt\n");
			printf("\t.subsections_via_symbols\n");
		}

		printf("\t.section __IMPORT,__pointers,non_lazy_symbol_pointers\n");
		DLIST_FOREACH(p, &nlplist, link) {
			printf("L%s$non_lazy_ptr:\n", p->name);
			printf("\t.indirect_symbol %s\n", p->name);
			printf("\t.long 0\n");
	        }

	}
#endif

#define _MKSTR(x) #x
#define MKSTR(x) _MKSTR(x)
#define OS MKSTR(TARGOS)
        printf("\t.ident \"PCC: %s (%s)\"\n", PACKAGE_STRING, OS);
}

void
bjobcode()
{
#if defined(MACHOABI)
	DLIST_INIT(&stublist, link);
	DLIST_INIT(&nlplist, link);
#endif
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	extern int gotnr;
	NODE *r, *l;

	/* Fix function call arguments. On x86, just add funarg */
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG)
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = l;
		r->n_type = l->n_type;
	}
	if (kflag == 0)
		return p;
#if defined(ELFABI)
	/* Create an ASSIGN node for ebx */
	l = block(REG, NIL, NIL, INT, 0, MKAP(INT));
	l->n_rval = EBX;
	l = buildtree(ASSIGN, l, tempnode(gotnr, INT, 0, MKAP(INT)));
	if (p->n_right->n_op != CM) {
		p->n_right = block(CM, l, p->n_right, INT, 0, MKAP(INT));
	} else {
		for (r = p->n_right; r->n_left->n_op == CM; r = r->n_left)
			;
		r->n_left = block(CM, l, r->n_left, INT, 0, MKAP(INT));
	}
#endif
	return p;
}

/*
 * return the alignment of field of type t
 */
int
fldal(unsigned int t)
{
	uerror("illegal field type");
	return(ALINT);
}

/* fix up type of field p */
void
fldty(struct symtab *p)
{
}

/*
 * XXX - fix genswitch.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}
