/*	$Id: code.c,v 1.49 2011/01/29 14:55:33 ragge Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
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

static int nsse, ngpr, nrsp, rsaoff;
static int thissse, thisgpr, thisrsp;
enum { INTEGER = 1, INTMEM, SSE, SSEMEM, X87, STRREG, STRMEM, STRCPX };
static const int argregsi[] = { RDI, RSI, RDX, RCX, R08, R09 };
/*
 * The Register Save Area looks something like this.
 * It is put first on stack with fixed offsets.
 * struct {
 *	long regs[6];
 *	double xmm[8][2]; // 16 byte in width
 * };
 */
#define	RSASZ		(6*SZLONG+8*2*SZDOUBLE)
#define	RSALONGOFF(x)	(RSASZ-(x)*SZLONG)
#define	RSADBLOFF(x)	((8*2*SZDOUBLE)-(x)*SZDOUBLE*2)
/* va_list */
#define	VAARGSZ		(SZINT*2+SZPOINT(CHAR)*2)
#define	VAGPOFF(x)	(x)
#define	VAFPOFF(x)	(x-SZINT)
#define	VAOFA(x)	(x-SZINT-SZINT)
#define	VARSA(x)	(x-SZINT-SZINT-SZPOINT(0))

int lastloc = -1;
static int stroffset;

static int varneeds;
#define	NEED_GPNEXT	001
#define	NEED_FPNEXT	002
#define	NEED_1REGREF	004
#define	NEED_2REGREF	010
#define	NEED_MEMREF	020

static int argtyp(TWORD t, union dimfun *df, struct attr *ap);
static NODE *movtomem(NODE *p, int off, int reg);
static NODE *movtoreg(NODE *p, int rno);
void varattrib(char *name, struct attr *sap);

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	extern char *nextsect;
	static char *loctbl[] = { "text", "data", "section .rodata" };
	extern int tbss;
	char *name;
	TWORD t;
	int s;

	if (sp == NULL) {
		lastloc = -1;
		return;
	}
	if (kflag) {
#ifdef MACHOABI
		loctbl[DATA] = "section .data.rel.rw,\"aw\"";
		loctbl[RDATA] = "section .data.rel.ro,\"aw\"";
#else
		loctbl[DATA] = "section .data.rel.rw,\"aw\",@progbits";
		loctbl[RDATA] = "section .data.rel.ro,\"aw\",@progbits";
#endif
	}
	t = sp->stype;
	s = ISFTN(t) ? PROG : ISCON(cqual(t, sp->squal)) ? RDATA : DATA;
	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);

	if (sp->sflags & STLS) {
		if (s != DATA)
			cerror("non-data symbol in tls section");
		if (tbss)
			nextsect = ".tbss,\"awT\",@nobits";
		else
			nextsect = ".tdata,\"awT\",@progbits";
		tbss = 0;
		lastloc = -1;
	}

	varattrib(name, sp->sap);

	if (nextsect) {
		printf("	.section %s\n", nextsect);
		nextsect = NULL;
		s = -1;
	} else if (s != lastloc)
		printf("	.%s\n", loctbl[s]);
	lastloc = s;
	while (ISARY(t))
		t = DECREF(t);
	s = ISFTN(t) ? ALINT : talign(t, sp->sap);
	if (s > ALCHAR)
		printf("	.align %d\n", s/ALCHAR);
	if (sp->sclass == EXTDEF) {
		printf("\t.globl %s\n", name);
#ifndef MACHOABI
		printf("\t.type %s,@%s\n", name,
		    ISFTN(t)? "function" : "object");
#endif
	}
	if (sp->slevel == 0)
		printf("%s:\n", name);
	else
		printf(LABFMT ":\n", sp->soffset);
}

/*
 * Print out variable attributes.
 */
void
varattrib(char *name, struct attr *sap)
{
	extern char *nextsect;
	struct attr *ga;

	if ((ga = attr_find(sap, GCC_ATYP_SECTION)) != NULL)
		nextsect = ga->sarg(0);
	if ((ga = attr_find(sap, GCC_ATYP_WEAK)) != NULL)
		printf("	.weak %s\n", name);
	if (attr_find(sap, GCC_ATYP_DESTRUCTOR)) {
		printf("\t.section\t.dtors,\"aw\",@progbits\n");
		printf("\t.align 8\n\t.quad\t%s\n", name);
		lastloc = -1;
	}
	if (attr_find(sap, GCC_ATYP_CONSTRUCTOR)) {
		printf("\t.section\t.ctors,\"aw\",@progbits\n");
		printf("\t.align 8\n\t.quad\t%s\n", name);
		lastloc = -1;
	}
	if ((ga = attr_find(sap, GCC_ATYP_VISIBILITY)) &&
	    strcmp(ga->sarg(0), "default"))
		printf("\t.%s %s\n", ga->sarg(0), name);
	if ((ga = attr_find(sap, GCC_ATYP_ALIASWEAK))) {
		printf("	.weak %s\n", ga->sarg(0));
		printf("	.set %s,%s\n", ga->sarg(0), name);
	}
}

/*
 * code for the end of a function
 * deals with struct return here
 * The return value is in (or pointed to by) RETREG.
 */
void
efcode()
{
	struct symtab *sp;
	extern int gotnr;
	TWORD t;
	NODE *p, *r, *l;
	int typ, ssz, rno;

	gotnr = 0;	/* new number for next fun */
	sp = cftnsp;
	t = DECREF(sp->stype);
	if (t != STRTY && t != UNIONTY)
		return;

	/* XXX should have one routine for this */
	if ((typ = argtyp(t, sp->sdf, sp->sap)) == STRREG || typ == STRCPX) {
		/* Cast to long pointer and move to the registers */
		/* XXX can overrun struct size */
		/* XXX check carefully for SSE members */

		if ((ssz = tsize(t, sp->sdf, sp->sap)) > SZLONG*2)
			cerror("efcode1");

		if (typ == STRCPX) {
			t = DOUBLE;
			rno = XMM0;
		} else {
			t = LONG;
			rno = RAX;
		}
		if (ssz > SZLONG) {
			p = block(REG, NIL, NIL, INCREF(t), 0, MKAP(t));
			regno(p) = RAX;
			p = buildtree(UMUL, buildtree(PLUS, p, bcon(1)), NIL);
			ecomp(movtoreg(p, rno+1));
		}
		p = block(REG, NIL, NIL, INCREF(t), 0, MKAP(t));
		regno(p) = RAX;
		p = buildtree(UMUL, p, NIL);
		ecomp(movtoreg(p, rno));
	} else if (typ == STRMEM) {
		r = block(REG, NIL, NIL, INCREF(t), sp->sdf, sp->sap);
		regno(r) = RAX;
		r = buildtree(UMUL, r, NIL);
		l = tempnode(stroffset, INCREF(t), sp->sdf, sp->sap);
		l = buildtree(UMUL, l, NIL);
		ecomp(buildtree(ASSIGN, l, r));
		l = block(REG, NIL, NIL, LONG, 0, MKAP(LONG));
		regno(l) = RAX;
		r = tempnode(stroffset, LONG, 0, MKAP(LONG));
		ecomp(buildtree(ASSIGN, l, r));
	} else
		cerror("efcode");
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **s, int cnt)
{
	union arglist *al;
	struct symtab *sp;
	NODE *p, *r;
	TWORD t;
	int i, rno, typ;

	/* recalculate the arg offset and create TEMP moves */
	/* Always do this for reg, even if not optimizing, to free arg regs */
	nsse = ngpr = 0;
	nrsp = ARGINIT;
	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		sp = cftnsp;
		if (argtyp(DECREF(sp->stype), sp->sdf, sp->sap) == STRMEM) {
			r = block(REG, NIL, NIL, LONG, 0, MKAP(LONG));
			regno(r) = argregsi[ngpr++];
			p = tempnode(0, r->n_type, r->n_df, r->n_ap);
			stroffset = regno(p);
			ecomp(buildtree(ASSIGN, p, r));
		}
	}

	for (i = 0; i < cnt; i++) {
		sp = s[i];

		if (sp == NULL)
			continue; /* XXX when happens this? */

		switch (typ = argtyp(sp->stype, sp->sdf, sp->sap)) {
		case INTEGER:
		case SSE:
			if (typ == SSE)
				rno = XMM0 + nsse++;
			else
				rno = argregsi[ngpr++];
			r = block(REG, NIL, NIL, sp->stype, sp->sdf, sp->sap);
			regno(r) = rno;
			p = tempnode(0, sp->stype, sp->sdf, sp->sap);
			sp->soffset = regno(p);
			sp->sflags |= STNODE;
			ecomp(buildtree(ASSIGN, p, r));
			break;

		case SSEMEM:
			sp->soffset = nrsp;
			nrsp += SZDOUBLE;
			if (xtemps) {
				p = tempnode(0, sp->stype, sp->sdf, sp->sap);
				p = buildtree(ASSIGN, p, nametree(sp));
				sp->soffset = regno(p->n_left);
				sp->sflags |= STNODE;
				ecomp(p);
			}
			break;

		case INTMEM:
			sp->soffset = nrsp;
			nrsp += SZLONG;
			if (xtemps) {
				p = tempnode(0, sp->stype, sp->sdf, sp->sap);
				p = buildtree(ASSIGN, p, nametree(sp));
				sp->soffset = regno(p->n_left);
				sp->sflags |= STNODE;
				ecomp(p);
			}
			break;

		case STRMEM: /* Struct in memory */
			sp->soffset = nrsp;
			nrsp += tsize(sp->stype, sp->sdf, sp->sap);
			break;

		case X87: /* long double args */
			sp->soffset = nrsp;
			nrsp += SZLDOUBLE;
			break;

		case STRCPX:
		case STRREG: /* Struct in register */
			/* Allocate space on stack for the struct */
			/* For simplicity always fetch two longwords */
			autooff += (2*SZLONG);

			if (typ == STRCPX) {
				t = DOUBLE;
				rno = XMM0 + nsse++;
			} else {
				t = LONG;
				rno = argregsi[ngpr++];
			}
			r = block(REG, NIL, NIL, t, 0, MKAP(t));
			regno(r) = rno;
			ecomp(movtomem(r, -autooff, FPREG));

			if (tsize(sp->stype, sp->sdf, sp->sap) > SZLONG) {
				r = block(REG, NIL, NIL, t, 0, MKAP(t));
				regno(r) = (typ == STRCPX ?
				    XMM0 + nsse++ : argregsi[ngpr++]);
				ecomp(movtomem(r, -autooff+SZLONG, FPREG));
			}

			sp->soffset = -autooff;
			break;

		default:
			cerror("bfcode: %d", typ);
		}
	}

	/* Check if there are varargs */
	if (cftnsp->sdf == NULL || cftnsp->sdf->dfun == NULL)
		return; /* no prototype */
	al = cftnsp->sdf->dfun;

	for (; al->type != TELLIPSIS; al++) {
		t = al->type;
		if (t == TNULL)
			return;
		if (BTYPE(t) == STRTY || BTYPE(t) == UNIONTY)
			al++;
		for (; t > BTMASK; t = DECREF(t))
			if (ISARY(t) || ISFTN(t))
				al++;
	}

	/* fix stack offset */
	SETOFF(autooff, ALMAX);

	/* Save reg arguments in the reg save area */
	p = NIL;
	for (i = ngpr; i < 6; i++) {
		r = block(REG, NIL, NIL, LONG, 0, MKAP(LONG));
		regno(r) = argregsi[i];
		r = movtomem(r, -RSALONGOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, MKAP(INT)));
	}
	for (i = nsse; i < 8; i++) {
		r = block(REG, NIL, NIL, DOUBLE, 0, MKAP(DOUBLE));
		regno(r) = i + XMM0;
		r = movtomem(r, -RSADBLOFF(i)-autooff, FPREG);
		p = (p == NIL ? r : block(COMOP, p, r, INT, 0, MKAP(INT)));
	}
	autooff += RSASZ;
	rsaoff = autooff;
	thissse = nsse;
	thisgpr = ngpr;
	thisrsp = nrsp;

	ecomp(p);
}


/*
 * by now, the automatics and register variables are allocated
 */
void
bccode()
{
	SETOFF(autooff, SZINT);
}

/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag )
{
	if (flag)
		return;

#ifdef MACHOAPI
#define PT(x)
#else
#define	PT(x) printf(".type __pcc_" x ",@function\n")
#endif

	/* printout varargs routines if used */
	if (varneeds & NEED_GPNEXT) {
		printf(".text\n.align 4\n");
		PT("gpnext");
		printf("__pcc_gpnext:\n");
		printf("cmpl $48,(%%rdi)\njae 1f\n");
		printf("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("movq (%%rax),%%rax\naddl $8,(%%rdi)\nret\n");
		printf("1:movq 8(%%rdi),%%rax\nmovq (%%rax),%%rax\n");
		printf("addq $8,8(%%rdi)\nret\n");
	}
	if (varneeds & NEED_FPNEXT) {
		printf(".text\n.align 4\n");
		PT("fpnext");
		printf("__pcc_fpnext:\n");
		printf("cmpl $176,4(%%rdi)\njae 1f\n");
		printf("movl 4(%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("movsd (%%rax),%%xmm0\naddl $16,4(%%rdi)\nret\n");
		printf("1:movq 8(%%rdi),%%rax\nmovsd (%%rax),%%xmm0\n");
		printf("addq $8,8(%%rdi)\nret\n");
	}
	if (varneeds & NEED_1REGREF) {
		printf(".text\n.align 4\n");
		PT("1regref");
		printf("__pcc_1regref:\n");
		printf("cmpl $48,(%%rdi)\njae 1f\n");
		printf("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("addl $8,(%%rdi)\nret\n");
		printf("1:movq 8(%%rdi),%%rax\n");
		printf("addq $8,8(%%rdi)\nret\n");
	}
	if (varneeds & NEED_2REGREF) {
		printf(".text\n.align 4\n");
		PT("2regref");
		printf("__pcc_2regref:\n");
		printf("cmpl $40,(%%rdi)\njae 1f\n");
		printf("movl (%%rdi),%%eax\naddq 16(%%rdi),%%rax\n");
		printf("addl $16,(%%rdi)\nret\n");
		printf("1:movq 8(%%rdi),%%rax\n");
		printf("addq $16,8(%%rdi)\nret\n");
	}
	if (varneeds & NEED_MEMREF) {
		printf(".text\n.align 4\n");
		PT("memref");
		printf("__pcc_memref:\n");
		printf("movq 8(%%rdi),%%rax\n");
		printf("addq %%rsi,8(%%rdi)\nret\n");
	}
		

#define _MKSTR(x) #x
#define MKSTR(x) _MKSTR(x)
#define OS MKSTR(TARGOS)
#ifdef MACHOABI
	printf("\t.ident \"PCC: %s (%s)\"\n", PACKAGE_STRING, OS);
#else
        printf("\t.ident \"PCC: %s (%s)\"\n\t.end\n", PACKAGE_STRING, OS);
#endif
}

/*
 * Varargs stuff:
 * The ABI says that va_list should be declared as this typedef.
 * We handcraft it here and then just reference it.
 *
 * typedef struct {
 *	unsigned int gp_offset;
 *	unsigned int fp_offset;
 *	void *overflow_arg_area;
 *	void *reg_save_area;
 * } __builtin_va_list[1];
 *
 * There are a number of asm routines printed out if varargs are used:
 *	long __pcc_gpnext(va)	- get a gpreg value
 *	long __pcc_fpnext(va)	- get a fpreg value
 *	void *__pcc_1regref(va)	- get reference to a onereg struct 
 *	void *__pcc_2regref(va)	- get reference to a tworeg struct 
 *	void *__pcc_memref(va,sz)	- get reference to a large struct 
 */

static char *gp_offset, *fp_offset, *overflow_arg_area, *reg_save_area;
static char *gpnext, *fpnext, *_1regref, *_2regref, *memref;

void
bjobcode()
{
	struct symtab *sp;
	struct rstack *rp;
	NODE *p, *q;
	char *c;

	gp_offset = addname("gp_offset");
	fp_offset = addname("fp_offset");
	overflow_arg_area = addname("overflow_arg_area");
	reg_save_area = addname("reg_save_area");

	rp = bstruct(NULL, STNAME, NULL);
	p = block(NAME, NIL, NIL, UNSIGNED, 0, MKAP(UNSIGNED));
	soumemb(p, gp_offset, 0);
	soumemb(p, fp_offset, 0);
	p->n_type = VOID+PTR;
	p->n_ap = MKAP(VOID);
	soumemb(p, overflow_arg_area, 0);
	soumemb(p, reg_save_area, 0);
	nfree(p);
	q = dclstruct(rp);
	c = addname("__builtin_va_list");
	p = block(LB, bdty(NAME, c), bcon(1), INT, 0, MKAP(INT));
	p = tymerge(q, p);
	p->n_sp = lookup(c, 0);
	defid(p, TYPEDEF);
	nfree(q);
	nfree(p);

	/* for the static varargs functions */
#define	MKN(vn, rn, tp) \
	{ vn = addname(rn); sp = lookup(vn, SNORMAL); \
	  sp->sclass = USTATIC; sp->stype = tp; }

	MKN(gpnext, "__pcc_gpnext", FTN|LONG);
	MKN(fpnext, "__pcc_fpnext", FTN|DOUBLE);
	MKN(_1regref, "__pcc_1regref", FTN|VOID|(PTR<<TSHIFT));
	MKN(_2regref, "__pcc_2regref", FTN|VOID|(PTR<<TSHIFT));
	MKN(memref, "__pcc_memref", FTN|VOID|(PTR<<TSHIFT));
}

static NODE *
mkstkref(int off, TWORD typ)
{
	NODE *p;

	p = block(REG, NIL, NIL, PTR|typ, 0, MKAP(LONG));
	regno(p) = FPREG;
	return buildtree(PLUS, p, bcon(off/SZCHAR));
}

NODE *
amd64_builtin_stdarg_start(NODE *f, NODE *a, TWORD t)
{
	NODE *p, *r;

	/* use the values from the function header */
	p = a->n_left;
	r = buildtree(ASSIGN, structref(ccopy(p), STREF, reg_save_area),
	    mkstkref(-rsaoff, VOID));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, overflow_arg_area),
	    mkstkref(thisrsp, VOID)));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, gp_offset),
	    bcon(thisgpr*(SZLONG/SZCHAR))));
	r = buildtree(COMOP, r,
	    buildtree(ASSIGN, structref(ccopy(p), STREF, fp_offset),
	    bcon(thissse*(SZDOUBLE*2/SZCHAR)+48)));

	tfree(f);
	tfree(a);
	return r;
}

NODE *
amd64_builtin_va_arg(NODE *f, NODE *a, TWORD t)
{
	NODE *ap, *r, *dp;

	ap = a->n_left;
	dp = a->n_right;
	if (dp->n_type <= ULONGLONG || ISPTR(dp->n_type) ||
	    dp->n_type == FLOAT || dp->n_type == DOUBLE) {
		/* type might be in general register */
		if (dp->n_type == FLOAT || dp->n_type == DOUBLE) {
			f->n_sp = lookup(fpnext, SNORMAL);
			varneeds |= NEED_FPNEXT;
		} else {
			f->n_sp = lookup(gpnext, SNORMAL);
			varneeds |= NEED_GPNEXT;
		}
		f->n_type = f->n_sp->stype = INCREF(dp->n_type) + (FTN-PTR);
		f->n_ap = dp->n_ap;
		f->n_df = /* dp->n_df */ NULL;
		f = clocal(f);
		r = buildtree(CALL, f, ccopy(ap));
	} else if (ISSOU(dp->n_type) || dp->n_type == LDOUBLE) {
		/* put a reference directly to the stack */
		int sz = tsize(dp->n_type, dp->n_df, dp->n_ap);
		int al = talign(dp->n_type, dp->n_ap);
		if (al < ALLONG)
			al = ALLONG;
		if (sz <= SZLONG*2 && al == ALLONG) {
			if (sz <= SZLONG) {
				f->n_sp = lookup(_1regref, SNORMAL);
				varneeds |= NEED_1REGREF;
			} else {
				f->n_sp = lookup(_2regref, SNORMAL);
				varneeds |= NEED_2REGREF;
			}
			f->n_type = f->n_sp->stype;
			f = clocal(f);
			r = buildtree(CALL, f, ccopy(ap));
			r = ccast(r, INCREF(dp->n_type), 0, dp->n_df, dp->n_ap);
			r = buildtree(UMUL, r, NIL);
		} else {
			f->n_sp = lookup(memref, SNORMAL);
			varneeds |= NEED_MEMREF;
			f->n_type = f->n_sp->stype;
			f = clocal(f);
			SETOFF(sz, al);
			r = buildtree(CALL, f,
			    buildtree(CM, ccopy(ap), bcon(sz/SZCHAR)));
			r = ccast(r, INCREF(dp->n_type), 0, dp->n_df, dp->n_ap);
			r = buildtree(UMUL, r, NIL);
		}
	} else {
		uerror("amd64_builtin_va_arg not supported type");
		goto bad;
	}
	tfree(a);
	return r;
bad:
	uerror("bad argument to __builtin_va_arg");
	return bcon(0);
}

NODE *
amd64_builtin_va_end(NODE *f, NODE *a, TWORD t)
{
	tfree(f);
	tfree(a);
	return bcon(0); /* nothing */
}

NODE *
amd64_builtin_va_copy(NODE *f, NODE *a, TWORD t)
{
	tfree(f);
	f = buildtree(ASSIGN, buildtree(UMUL, a->n_left, NIL),
	    buildtree(UMUL, a->n_right, NIL));
	nfree(a);
	return f;
}

static NODE *
movtoreg(NODE *p, int rno)
{
	NODE *r;

	r = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	regno(r) = rno;
	return clocal(buildtree(ASSIGN, r, p));
}  

static NODE *
movtomem(NODE *p, int off, int reg)
{
	struct symtab s;
	NODE *r, *l;

	s.stype = p->n_type;
	s.squal = 0;
	s.sdf = p->n_df;
	s.sap = p->n_ap;
	s.soffset = off;
	s.sclass = AUTO;

	l = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
	l->n_lval = 0;
	regno(l) = reg;

	r = block(NAME, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	r->n_sp = &s;
	r = stref(block(STREF, l, r, 0, 0, 0));

	return clocal(buildtree(ASSIGN, r, p));
}  


/*
 * AMD64 parameter classification.
 */
static int
argtyp(TWORD t, union dimfun *df, struct attr *ap)
{
	int cl = 0;

	if (t <= ULONG || ISPTR(t) || t == BOOL) {
		cl = ngpr < 6 ? INTEGER : INTMEM;
	} else if (t == FLOAT || t == DOUBLE || t == FIMAG || t == IMAG) {
		cl = nsse < 8 ? SSE : SSEMEM;
	} else if (t == LDOUBLE || t == LIMAG) {
		cl = X87; /* XXX */
	} else if (t == STRTY || t == UNIONTY) {
		int sz = tsize(t, df, ap);

		if (sz <= 2*SZLONG && attr_find(ap, ATTR_COMPLEX) != NULL) {
			cl = nsse < 7 ? STRCPX : STRMEM;
		} else if (sz > 2*SZLONG || ((sz+SZLONG)/SZLONG)+ngpr > 6 ||
		    attr_find(ap, GCC_ATYP_PACKED) != NULL)
			cl = STRMEM;
		else
			cl = STRREG;
	} else
		cerror("FIXME: classify");
	return cl;
}

/*
 * Do the "hard work" in assigning correct destination for arguments.
 * Also convert arguments < INT to inte (default argument promotions).
 * XXX - should be dome elsewhere.
 */
static NODE *
argput(NODE *p)
{
	NODE *q;
	TWORD ty;
	int typ, r, ssz;

	if (p->n_op == CM) {
		p->n_left = argput(p->n_left);
		p->n_right = argput(p->n_right);
		return p;
	}

	/* first arg may be struct return pointer */
	/* XXX - check if varargs; setup al */
	switch (typ = argtyp(p->n_type, p->n_df, p->n_ap)) {
	case INTEGER:
	case SSE:
		if (typ == SSE)
			r = XMM0 + nsse++;
		else
			r = argregsi[ngpr++];
		if (p->n_type < INT || p->n_type == BOOL)
			p = cast(p, INT, 0);
		p = movtoreg(p, r);
		break;

	case X87:
		r = nrsp;
		nrsp += SZLDOUBLE;
		p = movtomem(p, r, STKREG);
		break;

	case SSEMEM:
		r = nrsp;
		nrsp += SZDOUBLE;
		p = movtomem(p, r, STKREG);
		break;

	case INTMEM:
		r = nrsp;
		nrsp += SZLONG;
		p = movtomem(p, r, STKREG);
		break;

	case STRCPX:
	case STRREG: /* Struct in registers */
		/* Cast to long pointer and move to the registers */
		/* XXX can overrun struct size */
		/* XXX check carefully for SSE members */
		ssz = tsize(p->n_type, p->n_df, p->n_ap);

		if (typ == STRCPX) {
			ty = DOUBLE;
			r = XMM0 + nsse++;
		} else {
			ty = LONG;
			r = argregsi[ngpr++];
		}
		if (ssz <= SZLONG) {
			q = cast(p->n_left, INCREF(ty), 0);
			nfree(p);
			q = buildtree(UMUL, q, NIL);
			p = movtoreg(q, r);
		} else if (ssz <= SZLONG*2) {
			NODE *ql, *qr;

			qr = cast(ccopy(p->n_left), INCREF(ty), 0);
			qr = movtoreg(buildtree(UMUL, qr, NIL), r);

			ql = cast(p->n_left, INCREF(ty), 0);
			ql = buildtree(UMUL, buildtree(PLUS, ql, bcon(1)), NIL);
			r = (typ == STRCPX ? XMM0 + nsse++ : argregsi[ngpr++]);
			ql = movtoreg(ql, r);

			nfree(p);
			p = buildtree(CM, ql, qr);
		} else
			cerror("STRREG");
		break;

	case STRMEM: {
		struct symtab s;
		NODE *l, *t;

		q = buildtree(UMUL, p->n_left, NIL);

		s.stype = p->n_type;
		s.squal = 0;
		s.sdf = p->n_df;
		s.sap = p->n_ap;
		s.soffset = nrsp;
		s.sclass = AUTO;

		nrsp += tsize(p->n_type, p->n_df, p->n_ap);

		l = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
		l->n_lval = 0;
		regno(l) = STKREG;

		t = block(NAME, NIL, NIL, p->n_type, p->n_df, p->n_ap);
		t->n_sp = &s;
		t = stref(block(STREF, l, t, 0, 0, 0));

		t = (buildtree(ASSIGN, t, q));
		nfree(p);
		p = t->n_left;
		nfree(t);
		break;
		}

	default:
		cerror("argument %d", typ);
	}
	return p;
}

/*
 * Sort arglist so that register assignments ends up last.
 */
static int
argsort(NODE *p)
{
	NODE *q, *r;
	int rv = 0;

	if (p->n_op != CM) {
		if (p->n_op == ASSIGN && p->n_left->n_op == REG &&
		    coptype(p->n_right->n_op) != LTYPE) {
			q = tempnode(0, p->n_type, p->n_df, p->n_ap);
			r = ccopy(q);
			p->n_right = buildtree(COMOP,
			    buildtree(ASSIGN, q, p->n_right), r);
		}
		return rv;
	}
	if (p->n_right->n_op == CM) {
		/* fixup for small structs in regs */
		q = p->n_right->n_left;
		p->n_right->n_left = p->n_left;
		p->n_left = p->n_right;
		p->n_right = q;
	}
	if (p->n_right->n_op == ASSIGN && p->n_right->n_left->n_op == REG &&
	    coptype(p->n_right->n_right->n_op) != LTYPE) {
		/* move before everything to avoid reg trashing */
		q = tempnode(0, p->n_right->n_type,
		    p->n_right->n_df, p->n_right->n_ap);
		r = ccopy(q);
		p->n_right->n_right = buildtree(COMOP,
		    buildtree(ASSIGN, q, p->n_right->n_right), r);
	}
	if (p->n_right->n_op == ASSIGN && p->n_right->n_left->n_op == REG) {
		if (p->n_left->n_op == CM &&
		    p->n_left->n_right->n_op == STASG) {
			q = p->n_left->n_right;
			p->n_left->n_right = p->n_right;
			p->n_right = q;
			rv = 1;
		} else if (p->n_left->n_op == STASG) {
			q = p->n_left;
			p->n_left = p->n_right;
			p->n_right = q;
			rv = 1;
		}
	}
	return rv | argsort(p->n_left);
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */
NODE *
funcode(NODE *p)
{
	NODE *l, *r;
	TWORD t;

	nsse = ngpr = nrsp = 0;
	/* Check if hidden arg needed */
	/* If so, add it in pass2 */
	if ((l = p->n_left)->n_type == INCREF(FTN)+STRTY ||
	    l->n_type == INCREF(FTN)+UNIONTY) {
		int ssz = tsize(BTYPE(l->n_type), l->n_df, l->n_ap);
		if (ssz > 2*SZLONG)
			ngpr++;
	}

	/* Convert just regs to assign insn's */
	p->n_right = argput(p->n_right);

	/* Must sort arglist so that STASG ends up first */
	/* This avoids registers being clobbered */
	while (argsort(p->n_right))
		;
	/* Check if there are varargs */
	if (nsse || l->n_df == NULL || l->n_df->dfun == NULL) {
		; /* Need RAX */
	} else {
		union arglist *al = l->n_df->dfun;

		for (; al->type != TELLIPSIS; al++) {
			if ((t = al->type) == TNULL)
				return p; /* No need */
			if (BTYPE(t) == STRTY || BTYPE(t) == UNIONTY)
				al++;
			for (; t > BTMASK; t = DECREF(t))
				if (ISARY(t) || ISFTN(t))
					al++;
		}
	}

	/* Always emit number of SSE regs used */
	l = movtoreg(bcon(nsse), RAX);
	if (p->n_right->n_op != CM) {
		p->n_right = block(CM, l, p->n_right, INT, 0, MKAP(INT));
	} else {
		for (r = p->n_right; r->n_left->n_op == CM; r = r->n_left)
			;
		r->n_left = block(CM, l, r->n_left, INT, 0, MKAP(INT));
	}
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
