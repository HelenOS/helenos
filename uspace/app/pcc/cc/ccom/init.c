/*	$Id: init.c,v 1.62 2011/02/19 17:23:39 ragge Exp $	*/

/*
 * Copyright (c) 2004, 2007 Anders Magnusson (ragge@ludd.ltu.se).
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

#include "pass1.h"
#include <string.h>

/*
 * The following machine-dependent routines may be called during
 * initialization:
 * 
 * zbits(OFFSZ, int)	- sets int bits of zero at position OFFSZ.
 * infld(CONSZ off, int fsz, CONSZ val)
 *			- sets the bitfield val starting at off and size fsz.
 * ninval(CONSZ off, int fsz, NODE *)
 *			- prints an integer constant which may have
 *			  a label associated with it, located at off and
 *			  size fsz.
 *
 * Initialization may be of different kind:
 * - Initialization at compile-time, all values are constants and laid
 *   out in memory. Static or extern variables outside functions.
 * - Initialization at run-time, written to their values as code.
 *
 * Currently run-time-initialized variables are only initialized by using
 * move instructions.  An optimization might be to detect that it is
 * initialized with constants and therefore copied from readonly memory.
 */

/*
 * The base element(s) of an initialized variable is kept in a linked 
 * list, allocated while initialized.
 *
 * When a scalar is found, entries are popped of the instk until it's
 * possible to find an entry for a new scalar; then onstk() is called 
 * to get the correct type and size of that scalar.
 *
 * If a right brace is found, pop the stack until a matching left brace
 * were found while filling the elements with zeros.  This left brace is
 * also marking where the current level is for designated initializations.
 *
 * Position entries are increased when traversing back down into the stack.
 */

/*
 * Good-to-know entries from symtab:
 *	soffset - # of bits from beginning of this structure.
 */

/*
 * TO FIX:
 * - Alignment of structs on like i386 char members.
 */

int idebug;

/*
 * Struct used in array initialisation.
 */
static struct instk {
	struct	instk *in_prev; /* linked list */
	struct	symtab *in_lnk;	/* member in structure initializations */
	struct	symtab *in_sym; /* symtab index */
	union	dimfun *in_df;	/* dimenston of array */
	TWORD	in_t;		/* type for this level */
	int	in_n;		/* number of arrays seen so far */
	int	in_fl;	/* flag which says if this level is controlled by {} */
} *pstk, pbase;

static struct symtab *csym;

#ifdef PCC_DEBUG
static void prtstk(struct instk *in);
#endif

/*
 * Linked lists for initializations.
 */
struct ilist {
	struct ilist *next;
	CONSZ off;	/* bit offset of this entry */
	int fsz;	/* bit size of this entry */
	NODE *n;	/* node containing this data info */
};

struct llist {
	SLIST_ENTRY(llist) next;
	CONSZ begsz;	/* bit offset of this entry */
	struct ilist *il;
};
static SLIST_HEAD(llh, llist) lpole;
static CONSZ basesz;
static int numents; /* # of array entries allocated */

static struct initctx {
	struct initctx *prev;
	struct instk *pstk;
	struct symtab *psym;
	struct llh lpole;
	CONSZ basesz;
	int numents;
} *inilnk;

static struct ilist *
getil(struct ilist *next, CONSZ b, int sz, NODE *n)
{
	struct ilist *il = tmpalloc(sizeof(struct ilist));

	il->off = b;
	il->fsz = sz;
	il->n = n;
	il->next = next;
	return il;
}

/*
 * Allocate a new struct defining a block of initializers appended to the
 * end of the llist. Return that entry.
 */
static struct llist *
getll(void)
{
	struct llist *ll;

	ll = tmpalloc(sizeof(struct llist));
	ll->begsz = numents * basesz;
	ll->il = NULL;
	SLIST_INSERT_LAST(&lpole, ll, next);
	numents++;
	return ll;
}

/*
 * Return structure containing off bitnumber.
 * Allocate more entries, if needed.
 */
static struct llist *
setll(OFFSZ off)
{
	struct llist *ll = NULL;

	/* Ensure that we have enough entries */
	while (off >= basesz * numents)
		 ll = getll();

	if (ll != NULL && ll->begsz <= off && ll->begsz + basesz > off)
		return ll;

	SLIST_FOREACH(ll, &lpole, next)
		if (ll->begsz <= off && ll->begsz + basesz > off)
			break;
	return ll; /* ``cannot fail'' */
}

/*
 * beginning of initialization; allocate space to store initialized data.
 * remember storage class for writeout in endinit().
 * p is the newly declarated type.
 */
void
beginit(struct symtab *sp)
{
	struct initctx *ict;
	struct instk *is = &pbase;

#ifdef PCC_DEBUG
	if (idebug)
		printf("beginit(%p), sclass %s\n", sp, scnames(sp->sclass));
#endif

	if (pstk) {
#ifdef PCC_DEBUG
		if (idebug)
			printf("beginit: saving ctx pstk %p\n", pstk);
#endif
		/* save old context */
		ict = tmpalloc(sizeof(struct initctx));
		ict->prev = inilnk;
		inilnk = ict;
		ict->pstk = pstk;
		ict->psym = csym;
		ict->lpole = lpole;
		ict->basesz = basesz;
		ict->numents = numents;
		is = tmpalloc(sizeof(struct instk));
	}
	csym = sp;

	numents = 0; /* no entries in array list */
	if (ISARY(sp->stype)) {
		basesz = tsize(DECREF(sp->stype), sp->sdf+1, sp->sap);
		if (basesz == 0) {
			uerror("array has incomplete type");
			basesz = SZINT;
		}
	} else
		basesz = tsize(DECREF(sp->stype), sp->sdf, sp->sap);
	SLIST_INIT(&lpole);

	/* first element */
	if (ISSOU(sp->stype)) {
		is->in_lnk = strmemb(sp->sap);
	} else
		is->in_lnk = NULL;
	is->in_n = 0;
	is->in_t = sp->stype;
	is->in_sym = sp;
	is->in_df = sp->sdf;
	is->in_fl = 0;
	is->in_prev = NULL;
	pstk = is;
}

/*
 * Push a new entry on the initializer stack.
 * The new entry will be "decremented" to the new sub-type of the previous
 * entry when called.
 * Popping of entries is done elsewhere.
 */
static void
stkpush(void)
{
	struct instk *is;
	struct symtab *sq, *sp;
	TWORD t;

	if (pstk == NULL) {
		sp = csym;
		t = 0;
	} else {
		t = pstk->in_t;
		sp = pstk->in_sym;
	}

#ifdef PCC_DEBUG
	if (idebug) {
		printf("stkpush: '%s' %s ", sp->sname, scnames(sp->sclass));
		tprint(stdout, t, 0);
	}
#endif

	/*
	 * Figure out what the next initializer will be, and push it on 
	 * the stack.  If this is an array, just decrement type, if it
	 * is a struct or union, extract the next element.
	 */
	is = tmpalloc(sizeof(struct instk));
	is->in_fl = 0;
	is->in_n = 0;
	if (pstk == NULL) {
		/* stack empty */
		is->in_lnk = ISSOU(sp->stype) ? strmemb(sp->sap) : NULL;
		is->in_t = sp->stype;
		is->in_sym = sp;
		is->in_df = sp->sdf;
	} else if (ISSOU(t)) {
		sq = pstk->in_lnk;
		if (sq == NULL) {
			uerror("excess of initializing elements");
		} else {
			is->in_lnk = ISSOU(sq->stype) ? strmemb(sq->sap) : NULL;
			is->in_t = sq->stype;
			is->in_sym = sq;
			is->in_df = sq->sdf;
		}
	} else if (ISARY(t)) {
		is->in_lnk = ISSOU(DECREF(t)) ? strmemb(pstk->in_sym->sap) : 0;
		is->in_t = DECREF(t);
		is->in_sym = sp;
		if (pstk->in_df->ddim != NOOFFSET && pstk->in_df->ddim &&
		    pstk->in_n >= pstk->in_df->ddim) {
			werror("excess of initializing elements");
			pstk->in_n--;
		}
		is->in_df = pstk->in_df+1;
	} else
		uerror("too many left braces");
	is->in_prev = pstk;
	pstk = is;

#ifdef PCC_DEBUG
	if (idebug) {
		printf(" newtype ");
		tprint(stdout, is->in_t, 0);
		printf("\n");
	}
#endif
}

/*
 * pop down to either next level that can handle a new initializer or
 * to the next braced level.
 */
static void
stkpop(void)
{
#ifdef PCC_DEBUG
	if (idebug)
		printf("stkpop\n");
#endif
	for (; pstk; pstk = pstk->in_prev) {
		if (pstk->in_t == STRTY && pstk->in_lnk != NULL) {
			pstk->in_lnk = pstk->in_lnk->snext;
			if (pstk->in_lnk != NULL)
				break;
		}
		if (ISSOU(pstk->in_t) && pstk->in_fl)
			break; /* need } */
		if (ISARY(pstk->in_t)) {
			pstk->in_n++;
			if (pstk->in_fl)
				break;
			if (pstk->in_df->ddim == NOOFFSET ||
			    pstk->in_n < pstk->in_df->ddim)
				break; /* ger more elements */
		}
	}
#ifdef PCC_DEBUG
	if (idebug > 1)
		prtstk(pstk);
#endif
}

/*
 * Count how many elements an array may consist of.
 */
static int
acalc(struct instk *is, int n)
{
	if (is == NULL || !ISARY(is->in_t))
		return 0;
	return acalc(is->in_prev, n * is->in_df->ddim) + n * is->in_n;
}

/*
 * Find current bit offset of the top element on the stack from
 * the beginning of the aggregate.
 */
static CONSZ
findoff(void)
{
	struct instk *is;
	OFFSZ off;

#ifdef PCC_DEBUG
	if (ISARY(pstk->in_t))
		cerror("findoff on bad type %x", pstk->in_t);
#endif

	/*
	 * Offset calculations. If:
	 * - previous type is STRTY, soffset has in-struct offset.
	 * - this type is ARY, offset is ninit*stsize.
	 */
	for (off = 0, is = pstk; is; is = is->in_prev) {
		if (is->in_prev && is->in_prev->in_t == STRTY)
			off += is->in_sym->soffset;
		if (ISARY(is->in_t)) {
			/* suesize is the basic type, so adjust */
			TWORD t = is->in_t;
			OFFSZ o;
			while (ISARY(t))
				t = DECREF(t);
			if (ISPTR(t)) {
				o = SZPOINT(t); /* XXX use tsize() */
			} else {
				o = tsize(t, is->in_sym->sdf, is->in_sym->sap);
			}
			off += o * acalc(is, 1);
			while (is->in_prev && ISARY(is->in_prev->in_t)) {
				if (is->in_prev->in_prev &&
				    is->in_prev->in_prev->in_t == STRTY)
					off += is->in_sym->soffset;
				is = is->in_prev;
			}
		}
	}
#ifdef PCC_DEBUG
	if (idebug>1) {
		printf("findoff: off %lld\n", off);
		prtstk(pstk);
	}
#endif
	return off;
}

/*
 * Insert the node p with size fsz at position off.
 * Bit fields are already dealt with, so a node of correct type
 * with correct alignment and correct bit offset is given.
 */
static void
nsetval(CONSZ off, int fsz, NODE *p)
{
	struct llist *ll;
	struct ilist *il;

	if (idebug>1)
		printf("setval: off %lld fsz %d p %p\n", off, fsz, p);

	if (fsz == 0)
		return;

	ll = setll(off);
	off -= ll->begsz;
	if (ll->il == NULL) {
		ll->il = getil(NULL, off, fsz, p);
	} else {
		il = ll->il;
		if (il->off > off) {
			ll->il = getil(ll->il, off, fsz, p);
		} else {
			for (il = ll->il; il->next; il = il->next)
				if (il->off <= off && il->next->off > off)
					break;
			if (il->off == off) {
				/* replace */
				nfree(il->n);
				il->n = p;
			} else
				il->next = getil(il->next, off, fsz, p);
		}
	}
}

/*
 * take care of generating a value for the initializer p
 * inoff has the current offset (last bit written)
 * in the current word being generated
 * Returns the offset.
 */
CONSZ
scalinit(NODE *p)
{
	CONSZ woff;
	NODE *q;
	int fsz;

#ifdef PCC_DEBUG
	if (idebug > 2) {
		printf("scalinit(%p)\n", p);
		fwalk(p, eprint, 0);
		prtstk(pstk);
	}
#endif

	if (nerrors)
		return 0;

	p = optim(p);

#ifdef notdef /* leave to the target to decide if useable */
	if (csym->sclass != AUTO && p->n_op != ICON &&
	    p->n_op != FCON && p->n_op != NAME)
		cerror("scalinit not leaf");
#endif

	/* Out of elements? */
	if (pstk == NULL) {
		uerror("excess of initializing elements");
		return 0;
	}

	/*
	 * Get to the simple type if needed.
	 */
	while (ISSOU(pstk->in_t) || ISARY(pstk->in_t)) {
		stkpush();
		/* If we are doing auto struct init */
		if (ISSOU(pstk->in_t) && ISSOU(p->n_type) &&
		    suemeq(pstk->in_sym->sap, p->n_ap))
			break;
	}

	if (ISSOU(pstk->in_t) == 0) {
		/* let buildtree do typechecking (and casting) */
		q = block(NAME, NIL,NIL, pstk->in_t, pstk->in_df,
		    pstk->in_sym->sap);
		p = buildtree(ASSIGN, q, p);
		nfree(p->n_left);
		q = optim(p->n_right);
		nfree(p);
	} else
		q = p;

	woff = findoff();

	/* bitfield sizes are special */
	if (pstk->in_sym->sclass & FIELD)
		fsz = -(pstk->in_sym->sclass & FLDSIZ);
	else
		fsz = (int)tsize(pstk->in_t, pstk->in_sym->sdf,
		    pstk->in_sym->sap);

	nsetval(woff, fsz, q);

	stkpop();
#ifdef PCC_DEBUG
	if (idebug > 2) {
		printf("scalinit e(%p)\n", q);
	}
#endif
	return woff;
}

/*
 * Generate code to insert a value into a bitfield.
 */
static void
insbf(OFFSZ off, int fsz, int val)
{
	struct symtab sym;
	NODE *p, *r;
	TWORD typ;

#ifdef PCC_DEBUG
	if (idebug > 1)
		printf("insbf: off %lld fsz %d val %d\n", off, fsz, val);
#endif

	if (fsz == 0)
		return;

	/* small opt: do char instead of bf asg */
	if ((off & (ALCHAR-1)) == 0 && fsz == SZCHAR)
		typ = CHAR;
	else
		typ = INT;
	/* Fake a struct reference */
	p = buildtree(ADDROF, nametree(csym), NIL);
	sym.stype = typ;
	sym.squal = 0;
	sym.sdf = 0;
	sym.sap = MKAP(typ);
	sym.soffset = (int)off;
	sym.sclass = (char)(typ == INT ? FIELD | fsz : MOU);
	r = xbcon(0, &sym, typ);
	p = block(STREF, p, r, INT, 0, MKAP(INT));
	ecode(buildtree(ASSIGN, stref(p), bcon(val)));
}

/*
 * Clear a bitfield, starting at off and size fsz.
 */
static void
clearbf(OFFSZ off, OFFSZ fsz)
{
	/* Pad up to the next even initializer */
	if ((off & (ALCHAR-1)) || (fsz < SZCHAR)) {
		int ba = (int)(((off + (SZCHAR-1)) & ~(SZCHAR-1)) - off);
		if (ba > fsz)
			ba = (int)fsz;
		insbf(off, ba, 0);
		off += ba;
		fsz -= ba;
	}
	while (fsz >= SZCHAR) {
		insbf(off, SZCHAR, 0);
		off += SZCHAR;
		fsz -= SZCHAR;
	}
	if (fsz)
		insbf(off, fsz, 0);
}

/*
 * final step of initialization.
 * print out init nodes and generate copy code (if needed).
 */
void
endinit(void)
{
	struct llist *ll;
	struct ilist *il;
	int fsz;
	OFFSZ lastoff, tbit;

#ifdef PCC_DEBUG
	if (idebug)
		printf("endinit()\n");
#endif

	if (csym->sclass != AUTO)
		defloc(csym);

	/* Calculate total block size */
	if (ISARY(csym->stype) && csym->sdf->ddim == NOOFFSET) {
		tbit = numents*basesz; /* open-ended arrays */
		csym->sdf->ddim = numents;
		if (csym->sclass == AUTO) { /* Get stack space */
			csym->soffset = NOOFFSET;
			oalloc(csym, &autooff);
		}
	} else
		tbit = tsize(csym->stype, csym->sdf, csym->sap);

	/* Traverse all entries and print'em out */
	lastoff = 0;
	SLIST_FOREACH(ll, &lpole, next) {
		for (il = ll->il; il; il = il->next) {
#ifdef PCC_DEBUG
			if (idebug > 1) {
				printf("off %lld size %d val %lld type ",
				    ll->begsz+il->off, il->fsz, il->n->n_lval);
				tprint(stdout, il->n->n_type, 0);
				printf("\n");
			}
#endif
			fsz = il->fsz;
			if (csym->sclass == AUTO) {
				struct symtab sym;
				NODE *p, *r, *n;

				if (ll->begsz + il->off > lastoff)
					clearbf(lastoff,
					    (ll->begsz + il->off) - lastoff);

				/* Fake a struct reference */
				p = buildtree(ADDROF, nametree(csym), NIL);
				n = il->n;
				sym.stype = n->n_type;
				sym.squal = n->n_qual;
				sym.sdf = n->n_df;
				sym.sap = n->n_ap;
				sym.soffset = (int)(ll->begsz + il->off);
				sym.sclass = (char)(fsz < 0 ? FIELD | -fsz : 0);
				r = xbcon(0, &sym, INT);
				p = block(STREF, p, r, INT, 0, MKAP(INT));
				ecomp(buildtree(ASSIGN, stref(p), il->n));
				if (fsz < 0)
					fsz = -fsz;

			} else {
				if (ll->begsz + il->off > lastoff)
					zbits(lastoff,
					    (ll->begsz + il->off) - lastoff);
				if (fsz < 0) {
					fsz = -fsz;
					infld(il->off, fsz, il->n->n_lval);
				} else
					ninval(il->off, fsz, il->n);
				tfree(il->n);
			}
			lastoff = ll->begsz + il->off + fsz;
		}
	}
	if (csym->sclass == AUTO) {
		clearbf(lastoff, tbit-lastoff);
	} else
		zbits(lastoff, tbit-lastoff);
	
	endictx();
}

void
endictx(void)
{
	struct initctx *ict = inilnk;

	if (ict == NULL)
		return;

	pstk = ict->pstk;
	csym = ict->psym;
	lpole = ict->lpole;
	basesz = ict->basesz;
	numents = ict->numents;
	inilnk = inilnk->prev;
#ifdef PCC_DEBUG
	if (idebug)
		printf("endinit: restoring ctx pstk %p\n", pstk);
#endif
}

/*
 * process an initializer's left brace
 */
void
ilbrace()
{

#ifdef PCC_DEBUG
	if (idebug)
		printf("ilbrace()\n");
#endif

	if (pstk == NULL)
		return;

	stkpush();
	pstk->in_fl = 1; /* mark lbrace */
#ifdef PCC_DEBUG
	if (idebug > 1)
		prtstk(pstk);
#endif
}

/*
 * called when a '}' is seen
 */
void
irbrace()
{
#ifdef PCC_DEBUG
	if (idebug)
		printf("irbrace()\n");
	if (idebug > 2)
		prtstk(pstk);
#endif

	if (pstk == NULL)
		return;

	/* Got right brace, search for corresponding in the stack */
	for (; pstk->in_prev != NULL; pstk = pstk->in_prev) {
		if(!pstk->in_fl)
			continue;

		/* we have one now */

		pstk->in_fl = 0;  /* cancel { */
		if (ISARY(pstk->in_t))
			pstk->in_n = pstk->in_df->ddim;
		else if (pstk->in_t == STRTY) {
			while (pstk->in_lnk != NULL &&
			    pstk->in_lnk->snext != NULL)
				pstk->in_lnk = pstk->in_lnk->snext;
		}
		stkpop();
		return;
	}
}

/*
 * Create a new init stack based on given elements.
 */
static void
mkstack(NODE *p)
{

#ifdef PCC_DEBUG
	if (idebug) {
		printf("mkstack: %p\n", p);
		if (idebug > 1 && p)
			fwalk(p, eprint, 0);
	}
#endif

	if (p == NULL)
		return;
	mkstack(p->n_left);

	switch (p->n_op) {
	case LB: /* Array index */
		if (p->n_right->n_op != ICON)
			cerror("mkstack");
		if (!ISARY(pstk->in_t))
			uerror("array indexing non-array");
		pstk->in_n = (int)p->n_right->n_lval;
		nfree(p->n_right);
		break;

	case NAME:
		if (pstk->in_lnk) {
			for (; pstk->in_lnk; pstk->in_lnk = pstk->in_lnk->snext)
				if (pstk->in_lnk->sname == (char *)p->n_sp)
					break;
			if (pstk->in_lnk == NULL)
				uerror("member missing");
		} else {
			uerror("not a struct/union");
		}
		break;
	default:
		cerror("mkstack2");
	}
	nfree(p);
	stkpush();

}

/*
 * Initialize a specific element, as per C99.
 */
void
desinit(NODE *p)
{
	int op = p->n_op;

	if (pstk == NULL)
		stkpush(); /* passed end of array */
	while (pstk->in_prev && pstk->in_fl == 0)
		pstk = pstk->in_prev; /* Empty stack */

	if (ISSOU(pstk->in_t))
		pstk->in_lnk = strmemb(pstk->in_sym->sap);

	mkstack(p);	/* Setup for assignment */

	/* pop one step if SOU, ilbrace will push */
	if (op == NAME || op == LB)
		pstk = pstk->in_prev;

#ifdef PCC_DEBUG
	if (idebug > 1) {
		printf("desinit e\n");
		prtstk(pstk);
	}
#endif
}

/*
 * Convert a string to an array of char/wchar for asginit.
 */
static void
strcvt(NODE *p)
{
	NODE *q = p;
	char *s;
	int i;

#ifdef mach_arm
	/* XXX */
	if (p->n_op == UMUL && p->n_left->n_op == ADDROF)
		p = p->n_left->n_left;
#endif

	for (s = p->n_sp->sname; *s != 0; ) {
		if (*s++ == '\\') {
			i = esccon(&s);  
		} else
			i = (unsigned char)s[-1];
		asginit(bcon(i));
	} 
	tfree(q);
}

/*
 * Do an assignment to a struct element.
 */
void
asginit(NODE *p)
{
	int g;

#ifdef PCC_DEBUG
	if (idebug)
		printf("asginit %p\n", p);
	if (idebug > 1 && p)
		fwalk(p, eprint, 0);
#endif

	/* convert string to array of char/wchar */
	if (p && (DEUNSIGN(p->n_type) == ARY+CHAR ||
	    p->n_type == ARY+WCHAR_TYPE)) {
		TWORD t;

		t = p->n_type == ARY+WCHAR_TYPE ? ARY+WCHAR_TYPE : ARY+CHAR;
		/*
		 * ...but only if next element is ARY+CHAR, otherwise 
		 * just fall through.
		 */

		/* HACKHACKHACK */
		struct instk *is = pstk;

		if (pstk == NULL)
			stkpush();
		while (ISSOU(pstk->in_t) || ISARY(pstk->in_t))
			stkpush();
		if (pstk->in_prev && 
		    (DEUNSIGN(pstk->in_prev->in_t) == t ||
		    pstk->in_prev->in_t == t)) {
			pstk = pstk->in_prev;
			if ((g = pstk->in_fl) == 0)
				pstk->in_fl = 1; /* simulate ilbrace */

			strcvt(p);
			if (g == 0)
				irbrace(); /* will fill with zeroes */
			return;
		} else
			pstk = is; /* no array of char */
		/* END HACKHACKHACK */
	}

	if (p == NULL) { /* only end of compound stmt */
		irbrace();
	} else /* assign next element */
		scalinit(p);
}

#ifdef PCC_DEBUG
void
prtstk(struct instk *in)
{
	int i, o = 0;

	printf("init stack:\n");
	for (; in != NULL; in = in->in_prev) {
		for (i = 0; i < o; i++)
			printf("  ");
		printf("%p) '%s' ", in, in->in_sym->sname);
		tprint(stdout, in->in_t, 0);
		printf(" %s ", scnames(in->in_sym->sclass));
		if (in->in_df /* && in->in_df->ddim */)
		    printf("arydim=%d ", in->in_df->ddim);
		printf("ninit=%d ", in->in_n);
		if (BTYPE(in->in_t) == STRTY || ISARY(in->in_t))
			printf("stsize=%d ",
			    (int)tsize(in->in_t, in->in_df, in->in_sym->sap));
		if (in->in_fl) printf("{ ");
		printf("soff=%d ", in->in_sym->soffset);
		if (in->in_t == STRTY) {
			if (in->in_lnk)
				printf("curel %s ", in->in_lnk->sname);
			else
				printf("END struct");
		}
		printf("\n");
		o++;
	}
}
#endif

/*
 * Do a simple initialization.
 * At block 0, just print out the value, at higher levels generate
 * appropriate code.
 */
void
simpleinit(struct symtab *sp, NODE *p)
{
	NODE *q, *r, *nt;
	TWORD t;
	int sz;

	/* May be an initialization of an array of char by a string */
	if ((DEUNSIGN(p->n_type) == ARY+CHAR &&
	    DEUNSIGN(sp->stype) == ARY+CHAR) ||
	    (DEUNSIGN(p->n_type) == DEUNSIGN(ARY+WCHAR_TYPE) &&
	    DEUNSIGN(sp->stype) == DEUNSIGN(ARY+WCHAR_TYPE))) {
		/* Handle "aaa" as { 'a', 'a', 'a' } */
		beginit(sp);
		strcvt(p);
		if (csym->sdf->ddim == NOOFFSET)
			scalinit(bcon(0)); /* Null-term arrays */
		endinit();
		return;
	}

	nt = nametree(sp);
	switch (sp->sclass) {
	case STATIC:
	case EXTDEF:
		q = nt;
#ifndef NO_COMPLEX
		if (ANYCX(q) || ANYCX(p)) {
			r = cxop(ASSIGN, q, p);
			/* XXX must unwind the code generated here */
			/* We can rely on correct code generated */
			p = r->n_left->n_right->n_left;
			r->n_left->n_right->n_left = bcon(0);
			tfree(r);
			defloc(sp);
			r = p->n_left->n_right;
			sz = (int)tsize(r->n_type, r->n_df, r->n_ap);
			ninval(0, sz, r);
			ninval(0, sz, p->n_right->n_right);
			tfree(p);
			break;
		}
#endif
		p = optim(buildtree(ASSIGN, nt, p));
		defloc(sp);
		q = p->n_right;
		t = q->n_type;
		sz = (int)tsize(t, q->n_df, q->n_ap);
		ninval(0, sz, q);
		tfree(p);
		break;

	case AUTO:
	case REGISTER:
		if (ISARY(sp->stype))
			cerror("no array init");
		q = nt;
#ifndef NO_COMPLEX

		if (ANYCX(q) || ANYCX(p))
			r = cxop(ASSIGN, q, p);
		else
#endif
			r = buildtree(ASSIGN, q, p);
		ecomp(r);
		break;

	default:
		uerror("illegal initialization");
	}
}
