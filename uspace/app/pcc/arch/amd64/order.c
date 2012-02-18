/*	$Id: order.c,v 1.14 2011/02/18 17:08:31 ragge Exp $	*/
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


# include "pass2.h"

#include <string.h>

int canaddr(NODE *);

/* is it legal to make an OREG or NAME entry which has an
 * offset of off, (from a register of r), if the
 * resulting thing had type t */
int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	return(0);  /* YES */
}

/*
 * Check if LS and try to make it indexable.
 * Ignore SCONV to long.
 * Return 0 if failed.
 */
static int
findls(NODE *p, int check)
{
	CONSZ c;

	if (p->n_op == SCONV && p->n_type == LONG && p->n_left->n_type == INT)
		p = p->n_left; /* Ignore pointless SCONVs here */
	if (p->n_op != LS || p->n_right->n_op != ICON)
		return 0;
	if ((c = p->n_right->n_lval) != 1 && c != 2 && c != 3)
		return 0;
	if (check == 1 && p->n_left->n_op != REG)
		return 0;
	if (!isreg(p->n_left))
		(void)geninsn(p->n_left, INAREG);
	return 1;
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 *
 * AMD64 (and i386) have a quite powerful addressing scheme:
 * 	:	4(%rax)		4 + %rax
 * 	:	4(%rbx,%rax,8)	4 + %rbx + %rax * 8
 * 	:	4(,%rax)	4 + %rax * 8
 * The 8 above can be 1,2,4 or 8.
 */
void
offstar(NODE *p, int shape)
{
	NODE *l;

	if (x2debug) {
		printf("offstar(%p)\n", p);
		fwalk(p, e2print, 0);
	}

	if (isreg(p))
		return; /* Matched (%rax) */

	if (findls(p, 0))
		return; /* Matched (,%rax,8) */

	if ((p->n_op == PLUS || p->n_op == MINUS) && p->n_left->n_op == ICON) {
		l = p->n_right;
		if (isreg(l))
			return; /* Matched 4(%rax) */
		if (findls(l, 0))
			return; /* Matched 4(,%rax,8) */
		if (l->n_op == PLUS && isreg(l->n_right)) {
			if (findls(l->n_left, 0))
				return; /* Matched 4(%rbx,%rax,8) */
			(void)geninsn(l->n_left, INAREG);
			return; /* Generate 4(%rbx,%rax) */
		}
		(void)geninsn(l, INAREG);
		return; /* Generate 4(%rbx) */
	}

	if (p->n_op == PLUS) {
		if (!isreg(p->n_left)) /* ensure right is REG */
			(void)geninsn(p->n_left, INAREG);
		if (isreg(p->n_right))
			return; /* Matched (%rax,%rbx) */
		if (findls(p->n_right, 0))
			return; /* Matched (%rax,%rbx,4) */
		(void)geninsn(p->n_right, INAREG);
		return; /* Generate (%rbx,%rax) */
	}
		
	(void)geninsn(p, INAREG);
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 * For simple OREGs conversion should already be done.
 */
void
myormake(NODE *q)
{
	static int shtbl[] = { 1,2,4,8 };
	NODE *p, *r;
	CONSZ c = 0;
	int r1, r2, sh;
	int mkconv = 0;
	char *n = "";

#define	risreg(p)	(p->n_op == REG)
	if (x2debug) {
		printf("myormake(%p)\n", q);
		fwalk(q, e2print, 0);
	}
	r1 = r2 = MAXREGS;
	sh = 1;

	r = p = q->n_left;

	if ((p->n_op == PLUS || p->n_op == MINUS) && p->n_left->n_op == ICON) {
		c = p->n_left->n_lval;
		n = p->n_left->n_name;
		p = p->n_right;
	}

	if (p->n_op == PLUS && risreg(p->n_left)) {
		r1 = regno(p->n_left);
		p = p->n_right;
	}

	if (findls(p, 1)) {
		if (p->n_op == SCONV)
			p = p->n_left;
		sh = shtbl[(int)p->n_right->n_lval];
		r2 = regno(p->n_left);
		mkconv = 1;
	} else if (risreg(p)) {
		r2 = regno(p);
		mkconv = 1;
	} //else
	//	comperr("bad myormake tree");

	if (mkconv == 0)
		return;

	q->n_op = OREG;
	q->n_lval = c;
	q->n_rval = R2PACK(r1, r2, sh);
	q->n_name = n;
	tfree(r);
	if (x2debug) {
		printf("myormake converted %p\n", q);
		fwalk(q, e2print, 0);
	}
}

/*
 * Shape matches for UMUL.  Cooperates with offstar().
 */
int
shumul(NODE *p, int shape)
{

	if (x2debug)
		printf("shumul(%p)\n", p);

	/* Turns currently anything into OREG on x86 */
	if (shape & SOREG)
		return SROREG;
	return SRNOPE;
}

/*
 * Rewrite operations on binary operators (like +, -, etc...).
 * Called as a result of table lookup.
 */
int
setbin(NODE *p)
{

	if (x2debug)
		printf("setbin(%p)\n", p);
	return 0;

}

/* setup for assignment operator */
int
setasg(NODE *p, int cookie)
{
	if (x2debug)
		printf("setasg(%p)\n", p);
	return(0);
}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return 0;
}

/*
 * Special handling of some instruction register allocation.
 */
struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case SCONV:
		if ((q->ltype & TINT) &&
		    q->rtype == (TLONGLONG|TULONGLONG|TLONG|TULONG)) {
			static struct rspecial s[] = { 
				{ NLEFT, RAX }, { NRES, RAX }, { 0 } };
			return s;
		}
		break;

	case DIV:
		{
			static struct rspecial s[] = {
				{ NEVER, RAX }, { NEVER, RDX },
				{ NLEFT, RAX }, { NRES, RAX },
				{ NORIGHT, RDX }, { NORIGHT, RAX }, { 0 } };
			return s;
		}
		break;

	case MOD:
		if (q->ltype & TUCHAR) {
			static struct rspecial s[] = {
				{ NEVER, RAX },
				{ NLEFT, RAX }, { NRES, RAX },
				{ NORIGHT, RAX }, { 0 } };
			return s;
		} else {
			static struct rspecial s[] = {
				{ NEVER, RAX }, { NEVER, RDX },
				{ NLEFT, RAX }, { NRES, RDX },
				{ NORIGHT, RDX }, { NORIGHT, RAX }, { 0 } };
			return s;
		}
		break;

	case STARG:
		{
			static struct rspecial s[] = {
				{ NEVER, RDI }, 
				{ NLEFT, RSI },
				{ NEVER, RCX }, { 0 } };
			return s;
		}

	case STASG:
		{
			static struct rspecial s[] = {
				{ NEVER, RDI }, 
				{ NRIGHT, RSI }, { NOLEFT, RSI },
				{ NOLEFT, RCX }, { NORIGHT, RCX },
				{ NEVER, RCX }, { 0 } };
			return s;
		}

	case MUL:
		if (q->lshape == SAREG) {
			static struct rspecial s[] = {
				{ NEVER, RAX },
				{ NLEFT, RAX }, { NRES, RAX }, { 0 } };
			return s;
		}
		break;

	case LS:
	case RS:
		{
			static struct rspecial s[] = {
				{ NRIGHT, RCX }, { NOLEFT, RCX }, { 0 } };
			return s;
		}
		break;

	default:
		break;
	}
	comperr("nspecial entry %d", q - table);
	return 0; /* XXX gcc */
}

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0; /* nothing differs on x86 */
}

/*
 * set registers in calling conventions live.
 */
int *
livecall(NODE *p)
{
	static int r[NTEMPREG+1];
	NODE *q;
	int cr = 0;

	if (optype(p->n_op) != BITYPE)
		return r[0] = -1, r;

	for (q = p->n_right; q->n_op == CM; q = q->n_left) {
		if (q->n_right->n_op == ASSIGN &&
		    q->n_right->n_left->n_op == REG)
			r[cr++] = regno(q->n_right->n_left);
	}
	if (q->n_op == ASSIGN && q->n_left->n_op == REG)
		r[cr++] = regno(q->n_left);
	r[cr++] = -1;
	return r;
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
