/*	$Id: inline.c,v 1.37.2.2 2011/02/26 11:31:45 ragge Exp $	*/
/*
 * Copyright (c) 2003, 2008 Anders Magnusson (ragge@ludd.luth.se).
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


#include "pass1.h"

#include <stdarg.h>

/*
 * Simple description of how the inlining works:
 * A function found with the keyword "inline" is always saved.
 * If it also has the keyword "extern" it is written out thereafter.
 * If it has the keyword "static" it will be written out if it is referenced.
 * inlining will only be done if -xinline is given, and only if it is 
 * possible to inline the function.
 */
static void printip(struct interpass *pole);

struct ntds {
	int temp;
	TWORD type;
	union dimfun *df;
	struct attr *attr;
};

/*
 * ilink from ipole points to the next struct in the list of functions.
 */
static struct istat {
	SLIST_ENTRY(istat) link;
	struct symtab *sp;
	int flags;
#define	CANINL	1	/* function is possible to inline */
#define	WRITTEN	2	/* function is written out */
#define	REFD	4	/* Referenced but not yet written out */
	struct ntds *nt;/* Array of arg temp type data */
	int nargs;	/* number of args in array */
	int retval;	/* number of return temporary, if any */
	struct interpass shead;
} *cifun;

static SLIST_HEAD(, istat) ipole = { NULL, &ipole.q_forw };
static int nlabs;

#define	IP_REF	(MAXIP+1)
#ifdef PCC_DEBUG
#define	SDEBUG(x)	if (sdebug) printf x
#else
#define	SDEBUG(x)
#endif

int isinlining;
int inlnodecnt, inlstatcnt;

#define	SZSI	sizeof(struct istat)
#define	ialloc() memset(permalloc(SZSI), 0, SZSI); inlstatcnt++

static void
tcnt(NODE *p, void *arg)
{
	inlnodecnt++;
	if (nlabs > 1 && (p->n_op == REG || p->n_op == OREG) &&
	    regno(p) == FPREG)
		SLIST_FIRST(&ipole)->flags &= ~CANINL; /* no stack refs */
	if (p->n_op == NAME || p->n_op == ICON)
		p->n_sp = NULL; /* let symtabs be freed for inline funcs */
	if (nflag)
		printf("locking node %p\n", p);
}

static struct istat *
findfun(struct symtab *sp)
{
	struct istat *is;

	SLIST_FOREACH(is, &ipole, link)
		if (is->sp == sp)
			return is;
	return NULL;
}

static void
refnode(struct symtab *sp)
{
	struct interpass *ip;

	SDEBUG(("refnode(%s)\n", sp->sname));

	ip = permalloc(sizeof(*ip));
	ip->type = IP_REF;
	ip->ip_name = (char *)sp;
	inline_addarg(ip);
}

void
inline_addarg(struct interpass *ip)
{
	extern NODE *cftnod;

	SDEBUG(("inline_addarg(%p)\n", ip));
	DLIST_INSERT_BEFORE(&cifun->shead, ip, qelem);
	if (ip->type == IP_DEFLAB)
		nlabs++;
	if (ip->type == IP_NODE)
		walkf(ip->ip_node, tcnt, 0); /* Count as saved */
	if (cftnod)
		cifun->retval = regno(cftnod);
}

/*
 * Called to setup for inlining of a new function.
 */
void
inline_start(struct symtab *sp)
{
	struct istat *is;

	SDEBUG(("inline_start(\"%s\")\n", sp->sname));

	if (isinlining)
		cerror("already inlining function");

	if ((is = findfun(sp)) != 0) {
		if (!DLIST_ISEMPTY(&is->shead, qelem))
			uerror("inline function already defined");
	} else {
		is = ialloc();
		is->sp = sp;
		SLIST_INSERT_FIRST(&ipole, is, link);
		DLIST_INIT(&is->shead, qelem);
	}
	cifun = is;
	nlabs = 0;
	isinlining++;
}

/*
 * End of an inline function. In C99 an inline function declared "extern"
 * should also have external linkage and are therefore printed out.
 * But; this is the opposite for gcc inline functions, hence special
 * care must be taken to handle that specific case.
 */
void
inline_end()
{

	SDEBUG(("inline_end()\n"));

	if (sdebug)printip(&cifun->shead);
	isinlining = 0;

	if (attr_find(cifun->sp->sap, GCC_ATYP_GNU_INLINE)) {
		if (cifun->sp->sclass == EXTDEF)
			cifun->sp->sclass = 0;
		else
			cifun->sp->sclass = EXTDEF;
	}

	if (cifun->sp->sclass == EXTDEF) {
		cifun->flags |= REFD;
		inline_prtout();
	}
}

/*
 * Called when an inline function is found, to be sure that it will
 * be written out.
 * The function may not be defined when inline_ref() is called.
 */
void
inline_ref(struct symtab *sp)
{
	struct istat *w;

	SDEBUG(("inline_ref(\"%s\")\n", sp->sname));
	if (sp->sclass == SNULL)
		return; /* only inline, no references */
	if (isinlining) {
		refnode(sp);
	} else {
		SLIST_FOREACH(w,&ipole, link) {
			if (w->sp != sp)
				continue;
			w->flags |= REFD;
			return;
		}
		/* function not yet defined, print out when found */
		w = ialloc();
		w->sp = sp;
		w->flags |= REFD;
		SLIST_INSERT_FIRST(&ipole, w, link);
		DLIST_INIT(&w->shead, qelem);
	}
}

static void
puto(struct istat *w)
{
	struct interpass_prolog *ipp, *epp, *pp;
	struct interpass *ip, *nip;
	extern int crslab;
	int lbloff = 0;

	/* Copy the saved function and print it out */
	ipp = 0; /* XXX data flow analysis */
	DLIST_FOREACH(ip, &w->shead, qelem) {
		switch (ip->type) {
		case IP_EPILOG:
		case IP_PROLOG:
			if (ip->type == IP_PROLOG) {
				ipp = (struct interpass_prolog *)ip;
				/* fix label offsets */
				lbloff = crslab - ipp->ip_lblnum;
			} else {
				epp = (struct interpass_prolog *)ip;
				crslab += (epp->ip_lblnum - ipp->ip_lblnum);
			}
			pp = tmpalloc(sizeof(struct interpass_prolog));
			memcpy(pp, ip, sizeof(struct interpass_prolog));
			pp->ip_lblnum += lbloff;
#ifdef PCC_DEBUG
			if (ip->type == IP_EPILOG && crslab != pp->ip_lblnum)
				cerror("puto: %d != %d", crslab, pp->ip_lblnum);
#endif
			pass2_compile((struct interpass *)pp);
			break;

		case IP_REF:
			inline_ref((struct symtab *)ip->ip_name);
			break;

		default:
			nip = tmpalloc(sizeof(struct interpass));
			*nip = *ip;
			if (nip->type == IP_NODE) {
				NODE *p;

				p = nip->ip_node = ccopy(nip->ip_node);
				if (p->n_op == GOTO)
					p->n_left->n_lval += lbloff;
				else if (p->n_op == CBRANCH)
					p->n_right->n_lval += lbloff;
			} else if (nip->type == IP_DEFLAB)
				nip->ip_lbl += lbloff;
			pass2_compile(nip);
			break;
		}
	}
	w->flags |= WRITTEN;
}

/*
 * printout functions that are referenced.
 */
void
inline_prtout()
{
	struct istat *w;
	int gotone = 0;

	SLIST_FOREACH(w, &ipole, link) {
		if ((w->flags & (REFD|WRITTEN)) == REFD &&
		    !DLIST_ISEMPTY(&w->shead, qelem)) {
			defloc(w->sp);
			puto(w);
			w->flags |= WRITTEN;
			gotone++;
		}
	}
	if (gotone)
		inline_prtout();
}

#if 1
static void
printip(struct interpass *pole)
{
	static char *foo[] = {
	   0, "NODE", "PROLOG", "STKOFF", "EPILOG", "DEFLAB", "DEFNAM", "ASM" };
	struct interpass *ip;
	struct interpass_prolog *ipplg, *epplg;

	DLIST_FOREACH(ip, pole, qelem) {
		if (ip->type > MAXIP)
			printf("IP(%d) (%p): ", ip->type, ip);
		else
			printf("%s (%p): ", foo[ip->type], ip);
		switch (ip->type) {
		case IP_NODE: printf("\n");
#ifdef PCC_DEBUG
			fwalk(ip->ip_node, eprint, 0); break;
#endif
		case IP_PROLOG:
			ipplg = (struct interpass_prolog *)ip;
			printf("%s %s regs %lx autos %d mintemp %d minlbl %d\n",
			    ipplg->ipp_name, ipplg->ipp_vis ? "(local)" : "",
			    (long)ipplg->ipp_regs[0], ipplg->ipp_autos,
			    ipplg->ip_tmpnum, ipplg->ip_lblnum);
			break;
		case IP_EPILOG:
			epplg = (struct interpass_prolog *)ip;
			printf("%s %s regs %lx autos %d mintemp %d minlbl %d\n",
			    epplg->ipp_name, epplg->ipp_vis ? "(local)" : "",
			    (long)epplg->ipp_regs[0], epplg->ipp_autos,
			    epplg->ip_tmpnum, epplg->ip_lblnum);
			break;
		case IP_DEFLAB: printf(LABFMT "\n", ip->ip_lbl); break;
		case IP_DEFNAM: printf("\n"); break;
		case IP_ASM: printf("%s\n", ip->ip_asm); break;
		default:
			break;
		}
	}
}
#endif

static int toff;

static NODE *
mnode(struct ntds *nt, NODE *p)
{
	NODE *q;
	int num = nt->temp + toff;

	if (p->n_op == CM) {
		q = p->n_right;
		q = tempnode(num, nt->type, nt->df, nt->attr);
		nt--;
		p->n_right = buildtree(ASSIGN, q, p->n_right);
		p->n_left = mnode(nt, p->n_left);
		p->n_op = COMOP;
	} else {
		p = pconvert(p);
		q = tempnode(num, nt->type, nt->df, nt->attr);
		p = buildtree(ASSIGN, q, p);
	}
	return p;
}

static void
rtmps(NODE *p, void *arg)
{
	if (p->n_op == TEMP)
		regno(p) += toff;
}

/*
 * Inline a function. Returns the return value.
 * There are two major things that must be converted when 
 * inlining a function:
 * - Label numbers must be updated with an offset.
 * - The stack block must be relocated (add to REG or OREG).
 * - Temporaries should be updated (but no must)
 */
NODE *
inlinetree(struct symtab *sp, NODE *f, NODE *ap)
{
	extern int crslab, tvaloff;
	struct istat *is = findfun(sp);
	struct interpass *ip, *ipf, *ipl;
	int lmin, stksz, l0, l1, l2, gainl;
	OFFSZ stkoff;
	NODE *p, *rp;

	if (is == NULL || nerrors) {
		inline_ref(sp); /* prototype of not yet declared inline ftn */
		return NIL;
	}

	SDEBUG(("inlinetree(%p,%p) OK %d\n", f, ap, is->flags & CANINL));

	gainl = attr_find(sp->sap, GCC_ATYP_ALW_INL) != NULL;

	if ((is->flags & CANINL) == 0 && gainl)
		werror("cannot inline but always_inline");

	if ((is->flags & CANINL) == 0 || (xinline == 0 && gainl == 0)) {
		if (is->sp->sclass == STATIC || is->sp->sclass == USTATIC)
			inline_ref(sp);
		return NIL;
	}

	if (isinlining && cifun->sp == sp) {
		/* Do not try to inline ourselves */
		inline_ref(sp);
		return NIL;
	}

#ifdef mach_i386
	if (kflag) {
		is->flags |= REFD; /* if static inline, emit */
		return NIL; /* XXX cannot handle hidden ebx arg */
	}
#endif

	stkoff = stksz = 0;
	/* emit jumps to surround inline function */
	branch(l0 = getlab());
	plabel(l1 = getlab());
	l2 = getlab();
	SDEBUG(("branch labels %d,%d,%d\n", l0, l1, l2));

	ipf = DLIST_NEXT(&is->shead, qelem); /* prolog */
	ipl = DLIST_PREV(&is->shead, qelem); /* epilog */

	/* Fix label & temp offsets */
#define	IPP(x) ((struct interpass_prolog *)x)
	SDEBUG(("pre-offsets crslab %d tvaloff %d\n", crslab, tvaloff));
	lmin = crslab - IPP(ipf)->ip_lblnum;
	crslab += (IPP(ipl)->ip_lblnum - IPP(ipf)->ip_lblnum) + 1;
	toff = tvaloff - IPP(ipf)->ip_tmpnum;
	tvaloff += (IPP(ipl)->ip_tmpnum - IPP(ipf)->ip_tmpnum) + 1;
	SDEBUG(("offsets crslab %d lmin %d tvaloff %d toff %d\n",
	    crslab, lmin, tvaloff, toff));

	/* traverse until first real label */
	ipf = DLIST_NEXT(ipf, qelem);
	do
		ipf = DLIST_NEXT(ipf, qelem);
	while (ipf->type != IP_DEFLAB);

	/* traverse backwards to last label */
	do
		ipl = DLIST_PREV(ipl, qelem);
	while (ipl->type != IP_DEFLAB);

	/* So, walk over all statements and emit them */
	for (ip = ipf; ip != ipl; ip = DLIST_NEXT(ip, qelem)) {
		switch (ip->type) {
		case IP_NODE:
			p = ccopy(ip->ip_node);
			if (p->n_op == GOTO)
				p->n_left->n_lval += lmin;
			else if (p->n_op == CBRANCH)
				p->n_right->n_lval += lmin;
			walkf(p, rtmps, 0);
#ifdef PCC_DEBUG
			if (sdebug) {
				printf("converted node\n");
				fwalk(ip->ip_node, eprint, 0);
				fwalk(p, eprint, 0);
			}
#endif
			send_passt(IP_NODE, p);
			break;

		case IP_DEFLAB:
			SDEBUG(("converted label %d to %d\n",
			    ip->ip_lbl, ip->ip_lbl + lmin));
			send_passt(IP_DEFLAB, ip->ip_lbl + lmin);
			break;

		case IP_ASM:
			send_passt(IP_ASM, ip->ip_asm);
			break;

		case IP_REF:
			inline_ref((struct symtab *)ip->ip_name);
			break;

		default:
			cerror("bad inline stmt %d", ip->type);
		}
	}
	SDEBUG(("last label %d to %d\n", ip->ip_lbl, ip->ip_lbl + lmin));
	send_passt(IP_DEFLAB, ip->ip_lbl + lmin);

	branch(l2);
	plabel(l0);

	rp = block(GOTO, bcon(l1), NIL, INT, 0, MKAP(INT));
	if (is->retval)
		p = tempnode(is->retval + toff, DECREF(sp->stype),
		    sp->sdf, sp->sap);
	else
		p = bcon(0);
	rp = buildtree(COMOP, rp, p);

	if (is->nargs) {
		p = mnode(&is->nt[is->nargs-1], ap);
		rp = buildtree(COMOP, p, rp);
	}

	tfree(f);
	return rp;
}

void
inline_args(struct symtab **sp, int nargs)
{
	struct istat *cf;
	int i;

	SDEBUG(("inline_args\n"));
	cf = cifun;
	/*
	 * First handle arguments.  We currently do not inline anything if:
	 * - function has varargs
	 * - function args are volatile, checked if no temp node is asg'd.
	 */
	if (nargs) {
		for (i = 0; i < nargs; i++)
			if ((sp[i]->sflags & STNODE) == 0)
				return; /* not temporary */
		cf->nt = permalloc(sizeof(struct ntds)*nargs);
		for (i = 0; i < nargs; i++) {
			cf->nt[i].temp = sp[i]->soffset;
			cf->nt[i].type = sp[i]->stype;
			cf->nt[i].df = sp[i]->sdf;
			cf->nt[i].attr = sp[i]->sap;
		}
	}
	cf->nargs = nargs;
	cf->flags |= CANINL;
}
