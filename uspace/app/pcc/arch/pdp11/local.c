/*	$Id: local.c,v 1.7 2011/01/21 21:47:58 ragge Exp $	*/
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


#include "pass1.h"

/*	this file contains code which is dependent on the target machine */

/* clocal() is called to do local transformations on
 * an expression tree preparitory to its being
 * written out in intermediate code.
 *
 * the major essential job is rewriting the
 * automatic variables and arguments in terms of
 * REG and OREG nodes
 * conversion ops which are not necessary are also clobbered here
 * in addition, any special features (such as rewriting
 * exclusive or) are easily handled here as well
 */
NODE *
clocal(NODE *p)
{

	register struct symtab *q;
	register NODE *r, *l;
	register int o;
	int m;

#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	switch( o = p->n_op ){

	case NAME:
		if ((q = p->n_sp) == NULL)
			return p; /* Nothing to care about */

		switch (q->sclass) {

		case PARAM:
		case AUTO:
			/* fake up a structure reference */
			r = block(REG, NIL, NIL, PTR+STRTY, 0, 0);
			r->n_lval = 0;
			r->n_rval = FPREG;
			p = stref(block(STREF, r, p, 0, 0, 0));
			break;

		case STATIC:
			if (q->slevel == 0)
				break;
			p->n_lval = 0;
			break;

		case REGISTER:
			p->n_op = REG;
			p->n_lval = 0;
			p->n_rval = q->soffset;
			break;

		case EXTERN:
		case EXTDEF:
			break;
		}
		break;

	case PCONV:
		/* Remove redundant PCONV's. Be careful */
		l = p->n_left;
		if (l->n_op == ICON) {
			l->n_lval = (unsigned)l->n_lval;
			goto delp;
		}
		if (l->n_type < INT || l->n_type == LONGLONG || 
		    l->n_type == ULONGLONG) {
			/* float etc? */
			p->n_left = block(SCONV, l, NIL,
			    UNSIGNED, 0, MKSUE(UNSIGNED));
			break;
		}
		/* if left is SCONV, cannot remove */
		if (l->n_op == SCONV)
			break;

		/* avoid ADDROF TEMP */
		if (l->n_op == ADDROF && l->n_left->n_op == TEMP)
			break;

		/* if conversion to another pointer type, just remove */
		if (p->n_type > BTMASK && l->n_type > BTMASK)
			goto delp;
		break;

	delp:	l->n_type = p->n_type;
		l->n_qual = p->n_qual;
		l->n_df = p->n_df;
		l->n_sue = p->n_sue;
		nfree(p);
		p = l;
		break;
		
	case SCONV:
		l = p->n_left;

#if 0
		if (p->n_type == l->n_type) {
			nfree(p);
			return l;
		}

		if ((p->n_type & TMASK) == 0 && (l->n_type & TMASK) == 0 &&
		    btdims[p->n_type].suesize == btdims[l->n_type].suesize) {
			if (p->n_type != FLOAT && p->n_type != DOUBLE &&
			    l->n_type != FLOAT && l->n_type != DOUBLE &&
			    l->n_type != LDOUBLE && p->n_type != LDOUBLE) {
				if (l->n_op == NAME || l->n_op == UMUL ||
				    l->n_op == TEMP) {
					l->n_type = p->n_type;
					nfree(p);
					return l;
				}
			}
		}

		if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == INT &&
		    coptype(l->n_op) == BITYPE) {
			l->n_type = p->n_type;
			nfree(p);
			return l;
		}
#endif
		o = l->n_op;
		m = p->n_type;

		if (o == ICON) {
			CONSZ val = l->n_lval;

			if (!ISPTR(m)) /* Pointers don't need to be conv'd */
			    switch (m) {
			case BOOL:
				l->n_lval = l->n_lval != 0;
				break;
			case CHAR:
				l->n_lval = (char)val;
				break;
			case UCHAR:
				l->n_lval = val & 0377;
				break;
			case INT:
				l->n_lval = (short)val;
				break;
			case UNSIGNED:
				l->n_lval = val & 0177777;
				break;
			case ULONG:
				l->n_lval = val & 0xffffffff;
				break;
			case LONG:
				l->n_lval = (long)val;
				break;
			case LONGLONG:
				l->n_lval = (long long)val;
				break;
			case ULONGLONG:
				l->n_lval = val;
				break;
			case VOID:
				break;
			case LDOUBLE:
			case DOUBLE:
			case FLOAT:
				l->n_op = FCON;
				l->n_dcon = FLOAT_CAST(val, l->n_type);
				break;
			default:
				cerror("unknown type %d", m);
			}
			l->n_type = m;
			l->n_sue = MKSUE(m);
			nfree(p);
			return l;
		} else if (l->n_op == FCON) {
			l->n_lval = FLOAT_VAL(l->n_dcon);
			l->n_sp = NULL;
			l->n_op = ICON;
			l->n_type = m;
			l->n_sue = MKSUE(m);
			nfree(p);
			return clocal(l);
		}
		if (DEUNSIGN(p->n_type) == INT &&
		    DEUNSIGN(l->n_type) == INT) {
			nfree(p);
			p = l;
		}
		break;

	case CBRANCH:
		l = p->n_left;
		if (coptype(l->n_op) != BITYPE)
			break;
		if (l->n_left->n_op != SCONV || l->n_right->n_op != ICON)
			break;
		if ((r = l->n_left->n_left)->n_type > INT)
			break;
		/* compare with constant without casting */
		nfree(l->n_left);
		l->n_left = r;
		l->n_right->n_type = l->n_left->n_type;
		break;

	case STASG: /* struct assignment, modify left */
		l = p->n_left;
		if (l->n_type == STRTY)
			p->n_left = buildtree(ADDROF, l, NIL);
		break;

	case PMCONV:
	case PVCONV:
		r = p;
		p = buildtree(o == PMCONV ? MUL : DIV, p->n_left, p->n_right);
		nfree(r);
		break;

	case FORCE:
		/* put return value in return reg */
		p->n_op = ASSIGN;
		p->n_right = p->n_left;
		p->n_left = block(REG, NIL, NIL, p->n_type, 0, MKSUE(INT));
		p->n_left->n_rval = p->n_left->n_type == BOOL ? 
		    RETREG(CHAR) : RETREG(p->n_type);
		break;

	}
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("clocal end: %p\n", p);
		fwalk(p, eprint, 0);
	}
#endif
	return(p);
}

void
myp2tree(NODE *p)
{
	struct symtab *sp;

	if (p->n_op != FCON)
		return;

	sp = inlalloc(sizeof(struct symtab));
	sp->sclass = STATIC;
	sp->ssue = MKSUE(p->n_type);
	sp->slevel = 1; /* fake numeric label */
	sp->soffset = getlab();
	sp->sflags = 0;
	sp->stype = p->n_type;
	sp->squal = (CON >> TSHIFT);

	defloc(sp);
	ninval(0, sp->ssue->suesize, p);

	p->n_op = NAME;
	p->n_lval = 0;
	p->n_sp = sp;
}

/*ARGSUSED*/
int
andable(NODE *p)
{
	return(1);	/* all names can have & taken on them */
}

/*
 * at the end of the arguments of a ftn, set the automatic offset
 */
void
cendarg()
{
	autooff = AUTOINIT;
}

/*
 * Return 1 if a variable of type type is OK to put in register.
 */
int
cisreg(TWORD t)
{
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE ||
	    t == LONGLONG || t == ULONGLONG)
		return 0; /* not yet */
	return 1;
}

/*
 * return a node, for structure references, which is suitable for
 * being added to a pointer of type t, in order to be off bits offset
 * into a structure
 * t, d, and s are the type, dimension offset, and sizeoffset
 * For pdp10, return the type-specific index number which calculation
 * is based on its size. For example, short a[3] would return 3.
 * Be careful about only handling first-level pointers, the following
 * indirections must be fullword.
 */
NODE *
offcon(OFFSZ off, TWORD t, union dimfun *d, struct suedef *sue)
{
	register NODE *p;

	if (xdebug)
		printf("offcon: OFFSZ %lld type %x dim %p siz %d\n",
		    off, t, d, sue->suesize);

	p = bcon(0);
	p->n_lval = off/SZCHAR;	/* Default */
	return(p);
}

/*
 * Allocate off bits on the stack.  p is a tree that when evaluated
 * is the multiply count for off, t is a storeable node where to write
 * the allocated address.
 */
void
spalloc(NODE *t, NODE *p, OFFSZ off)
{
	NODE *sp;

	p = buildtree(MUL, p, bcon(off/SZCHAR)); /* XXX word alignment? */

	/* sub the size from sp */
	sp = block(REG, NIL, NIL, p->n_type, 0, MKSUE(INT));
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	ecomp(buildtree(MINUSEQ, sp, p));

	/* save the address of sp */
	sp = block(REG, NIL, NIL, PTR+INT, t->n_df, t->n_sue);
	sp->n_lval = 0;
	sp->n_rval = STKREG;
	t->n_type = sp->n_type;
	ecomp(buildtree(ASSIGN, t, sp)); /* Emit! */

}

/*
 * Print out a string of characters.
 * Assume that the assembler understands C-style escape
 * sequences.
 */
void
instring(struct symtab *sp)
{
	char *s;
	int val, cnt;

	defloc(sp);

	for (cnt = 0, s = sp->sname; *s != 0; ) {
		if (cnt++ == 0)
			printf(".byte ");
		if (*s++ == '\\')
			val = esccon(&s);
		else
			val = s[-1];
		printf("%o", val & 0377);
		if (cnt > 15) {
			cnt = 0;
			printf("\n");
		} else
			printf(",");
	}
	printf("%s0\n", cnt ? "" : ".byte ");
}

static int inbits, inval;

/*
 * set fsz bits in sequence to zero.
 */
void
zbits(OFFSZ off, int fsz)
{
	int m;

	if (idebug)
		printf("zbits off %lld, fsz %d inbits %d\n", off, fsz, inbits);
	if ((m = (inbits % SZCHAR))) {
		m = SZCHAR - m;
		if (fsz < m) {
			inbits += fsz;
			return;
		} else {
			fsz -= m;
			printf("\t.byte %d\n", inval);
			inval = inbits = 0;
		}
	}
	if (fsz >= SZCHAR) {
		printf(".=.+%o\n", fsz/SZCHAR);
		fsz -= (fsz/SZCHAR) * SZCHAR;
	}
	if (fsz) {
		inval = 0;
		inbits = fsz;
	}
}

/*
 * Initialize a bitfield.
 */
void
infld(CONSZ off, int fsz, CONSZ val)
{
	if (idebug)
		printf("infld off %lld, fsz %d, val %lld inbits %d\n",
		    off, fsz, val, inbits);
	val &= ((CONSZ)1 << fsz)-1;
	while (fsz + inbits >= SZCHAR) {
		inval |= (val << inbits);
		printf("\t.byte %d\n", inval & 255);
		fsz -= (SZCHAR - inbits);
		val >>= (SZCHAR - inbits);
		inval = inbits = 0;
	}
	if (fsz) {
		inval |= (val << inbits);
		inbits += fsz;
	}
}

/*
 * print out a constant node, may be associated with a label.
 * Do not free the node after use.
 * off is bit offset from the beginning of the aggregate
 * fsz is the number of bits this is referring to
 */
void
ninval(CONSZ off, int fsz, NODE *p)
{
#ifdef __pdp11__
	union { float f; double d; short s[4]; int i[2]; } u;
#endif
	struct symtab *q;
	TWORD t;
	int i;

	t = p->n_type;
	if (t > BTMASK)
		t = INT; /* pointer */

	while (p->n_op == SCONV || p->n_op == PCONV) {
		NODE *l = p->n_left;
		l->n_type = p->n_type;
		p = l;
	}

	if (p->n_op != ICON && p->n_op != FCON)
		cerror("ninval: init node not constant");

	if (p->n_op == ICON && p->n_sp != NULL && DEUNSIGN(t) != INT)
		uerror("element not constant");

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
		i = (p->n_lval >> 32);
		p->n_lval &= 0xffffffff;
		p->n_type = INT;
		ninval(off, 32, p);
		p->n_lval = i;
		ninval(off+32, 32, p);
		break;
	case LONG:
	case ULONG:
		printf("%o ; %o\n", (int)((p->n_lval >> 16) & 0177777),
		    (int)(p->n_lval & 0177777));
		break;
	case INT:
	case UNSIGNED:
		printf("%o", (int)(p->n_lval & 0177777));
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)) {
				printf("+" LABFMT, q->soffset);
			} else {
				printf("+%s", q->soname ? q->soname : exname(q->sname));
			}
		}
		printf("\n");
		break;
	case BOOL:
		if (p->n_lval > 1)
			p->n_lval = p->n_lval != 0;
		/* FALLTHROUGH */
	case CHAR:
	case UCHAR:
		printf("\t.byte %o\n", (int)p->n_lval & 0xff);
		break;
#ifdef __pdp11__
	case FLOAT:
		u.f = (float)p->n_dcon;
		printf("%o ; %o\n", u.i[0], u.i[1]);
		break;
	case LDOUBLE:
	case DOUBLE:
		u.d = (double)p->n_dcon;
		printf("%o ; %o ; %o ; %o\n", u.i[0], u.i[1], u.i[2], u.i[3]);
		break;
#else
	/* cross-compiling */
	case FLOAT:
		printf("%o ; %o\n", p->n_dcon.fd1, p->n_dcon.fd2);
		break;
	case LDOUBLE:
	case DOUBLE:
		printf("%o ; %o ; %o ; %o\n", p->n_dcon.fd1, p->n_dcon.fd2,
		    p->n_dcon.fd3, p->n_dcon.fd4);
		break;
#endif
	default:
		cerror("ninval");
	}
}

/* make a name look like an external name in the local machine */
char *
exname(char *p)
{
#define NCHNAM  256
	static char text[NCHNAM+1];
	int i;

	if (p == NULL)
		return "";

	text[0] = '_';
	for (i=1; *p && i<NCHNAM; ++i)
		text[i] = *p++;

	text[i] = '\0';
	text[NCHNAM] = '\0';  /* truncate */

	return (text);

}

/*
 * map types which are not defined on the local machine
 */
TWORD
ctype(TWORD type)
{
	switch (BTYPE(type)) {
	case SHORT:
		MODTYPE(type,INT);
		break;

	case USHORT:
		MODTYPE(type,UNSIGNED);
		break;

	case LDOUBLE:
		MODTYPE(type,DOUBLE);
		break;

	}
	return (type);
}

void
calldec(NODE *p, NODE *q) 
{
}

void
extdec(struct symtab *q)
{
}

/* make a common declaration for id, if reasonable */
void
defzero(struct symtab *sp)
{
	extern int lastloc;
	char *n;
	int off;

	off = tsize(sp->stype, sp->sdf, sp->ssue);
	off = (off+(SZCHAR-1))/SZCHAR;
	n = sp->soname ? sp->soname : exname(sp->sname);
	if (sp->sclass == STATIC) {
		printf(".bss\n");
		if (sp->slevel == 0)
			printf("%s:", n);
		else
			printf(LABFMT ":", sp->soffset);
		printf("	.=.+%o\n", off);
		lastloc = -1;
		return;
	}
	printf(".comm ");
	if (sp->slevel == 0)
		printf("%s,0%o\n", n, off);
	else
		printf(LABFMT ",0%o\n", sp->soffset, off);
}

/*
 * Give target the opportunity of handling pragmas.
 */
int
mypragma(char *str)
{
	return 0;
}

/*
 * Called when a identifier has been declared.
 */
void
fixdef(struct symtab *sp)
{
}

void
pass1_lastchance(struct interpass *ip)
{
}
