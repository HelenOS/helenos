/*	$Id: optim2.c,v 1.79 2010/06/04 07:18:46 ragge Exp $	*/
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

#include "pass2.h"

#include <string.h>
#include <stdlib.h>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define	BDEBUG(x)	if (b2debug) printf x

#define	mktemp(n, t)	mklnode(TEMP, 0, n, t)

#define	CHADD(bb,c)	{ if (bb->ch[0] == 0) bb->ch[0] = c; \
			  else if (bb->ch[1] == 0) bb->ch[1] = c; \
			  else comperr("triple cfnodes"); }
#define	FORCH(cn, chp)	\
	for (cn = &chp[0]; cn < &chp[2] && cn[0]; cn++)

/* main switch for new things not yet ready for all-day use */
/* #define ENABLE_NEW */


static int dfsnum;

void saveip(struct interpass *ip);
void deljumps(struct p2env *);
void optdump(struct interpass *ip);
void printip(struct interpass *pole);

static struct varinfo defsites;
struct interpass *storesave;

void bblocks_build(struct p2env *);
void cfg_build(struct p2env *);
void cfg_dfs(struct basicblock *bb, unsigned int parent, 
	     struct bblockinfo *bbinfo);
void dominators(struct p2env *);
struct basicblock *
ancestorwithlowestsemi(struct basicblock *bblock, struct bblockinfo *bbinfo);
void link(struct basicblock *parent, struct basicblock *child);
void computeDF(struct p2env *, struct basicblock *bblock);
void printDF(struct p2env *p2e);
void findTemps(struct interpass *ip);
void placePhiFunctions(struct p2env *);
void renamevar(struct p2env *p2e,struct basicblock *bblock);
void removephi(struct p2env *p2e);
void remunreach(struct p2env *);
static void liveanal(struct p2env *p2e);
static void printip2(struct interpass *);

/* create "proper" basic blocks, add labels where needed (so bblocks have labels) */
/* run before bb generate */
static void add_labels(struct p2env*) ;

/* Perform trace scheduling, try to get rid of gotos as much as possible */
void TraceSchedule(struct p2env*) ;

#ifdef ENABLE_NEW
static void do_cse(struct p2env* p2e) ;
#endif

/* Walk the complete set, performing a function on each node. 
 * if type is given, apply function on only that type */
void WalkAll(struct p2env* p2e, void (*f) (NODE*, void*), void* arg, int type) ;

void BBLiveDead(struct basicblock* bblock, int what, unsigned int variable) ;

/* Fill the live/dead code */
void LiveDead(struct p2env* p2e, int what, unsigned int variable) ;

#ifdef PCC_DEBUG
void printflowdiagram(struct p2env *, char *);
#endif

void
optimize(struct p2env *p2e)
{
	struct interpass *ipole = &p2e->ipole;

	if (b2debug) {
		printf("initial links\n");
		printip(ipole);
	}

	if (xdeljumps)
		deljumps(p2e); /* Delete redundant jumps and dead code */

	if (xssaflag)
		add_labels(p2e) ;
#ifdef ENABLE_NEW
	do_cse(p2e);
#endif

#ifdef PCC_DEBUG
	if (b2debug) {
		printf("links after deljumps\n");
		printip(ipole);
	}
#endif
	if (xssaflag || xtemps) {
		bblocks_build(p2e);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e);
	
#ifdef PCC_DEBUG
		printflowdiagram(p2e, "first");
#endif
	}
	if (xssaflag) {
		BDEBUG(("Calling liveanal\n"));
		liveanal(p2e);
		BDEBUG(("Calling dominators\n"));
		dominators(p2e);
		BDEBUG(("Calling computeDF\n"));
		computeDF(p2e, DLIST_NEXT(&p2e->bblocks, bbelem));

		if (b2debug) {
			printDF(p2e);
		}

		BDEBUG(("Calling placePhiFunctions\n"));

		placePhiFunctions(p2e);

		BDEBUG(("Calling renamevar\n"));

		renamevar(p2e,DLIST_NEXT(&p2e->bblocks, bbelem));

		BDEBUG(("Calling removephi\n"));

#ifdef PCC_DEBUG
		printflowdiagram(p2e, "ssa");
#endif

		removephi(p2e);

		BDEBUG(("Calling remunreach\n"));
/*		remunreach(p2e); */
		
		/*
		 * Recalculate basic blocks and cfg that was destroyed
		 * by removephi
		 */
		/* first, clean up all what deljumps should have done, and more */

		/* TODO: add the basic blocks done by the ssa code by hand. 
		 * The trace scheduler should not change the order in
		 * which blocks are executed or what data is calculated.
		 * Thus, the BBlock order should remain correct.
		 */

#ifdef ENABLE_NEW
		bblocks_build(p2e, &labinfo, &bbinfo);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e, &labinfo);

		TraceSchedule(p2e);
#ifdef PCC_DEBUG
		printflowdiagram(p2e, &labinfo, &bbinfo,"sched_trace");

		if (b2debug) {
			printf("after tracesched\n");
			printip(ipole);
			fflush(stdout) ;
		}
#endif
#endif

		/* Now, clean up the gotos we do not need any longer */
		if (xdeljumps)
			deljumps(p2e); /* Delete redundant jumps and dead code */

		bblocks_build(p2e);
		BDEBUG(("Calling cfg_build\n"));
		cfg_build(p2e);

#ifdef PCC_DEBUG
		printflowdiagram(p2e, "no_phi");

		if (b2debug) {
			printf("new tree\n");
			printip(ipole);
		}
#endif
	}

#ifdef PCC_DEBUG
	{
		int i;
		for (i = NIPPREGS; i--; )
			if (p2e->epp->ipp_regs[i] != 0)
				comperr("register error");
	}
#endif

	myoptim(ipole);
}

/*
 * Delete unused labels, excess of labels, gotos to gotos.
 * This routine can be made much more efficient.
 *
 * Layout of the statement list here (_must_ look this way!):
 *	PROLOG
 *	LABEL	- states beginning of function argument moves
 *	...code to save/move arguments
 *	LABEL	- states beginning of execution code
 *	...code + labels in function in function
 *	EPILOG
 *
 * This version of deljumps is based on the c2 implementation
 * that were included in 2BSD.
 */
#define	LABEL 1
#define	JBR	2
#define	CBR	3
#define	STMT	4
#define	EROU	5
struct dlnod {
	int op;
	struct interpass *dlip;
	struct dlnod *forw;
	struct dlnod *back;
	struct dlnod *ref;
	int labno;
	int refc;
};

#ifdef DLJDEBUG
static void
dumplink(struct dlnod *dl)
{
	printf("dumplink %p\n", dl);
	for (; dl; dl = dl->forw) {
		if (dl->op == STMT) {
			printf("STMT(%p)\n", dl);
			fwalk(dl->dlip->ip_node, e2print, 0);
		} else if (dl->op == EROU) {
			printf("EROU(%p)\n", dl);
		} else {
			static char *str[] = { 0, "LABEL", "JBR", "CBR" };
			printf("%s(%p) %d refc %d ref %p\n", str[dl->op], 
			    dl, dl->labno, dl->refc, dl->ref);
		}
	}
	printf("end dumplink\n");
}
#endif

/*
 * Create the linked list that we can work on.
 */
static void
listsetup(struct interpass *ipole, struct dlnod *dl)
{
	struct interpass *ip = DLIST_NEXT(ipole, qelem);
	struct interpass *nip;
	struct dlnod *p, *lastp;
	NODE *q;

	lastp = dl;
	while (ip->type != IP_DEFLAB)
		ip = DLIST_NEXT(ip,qelem);
	ip = DLIST_NEXT(ip,qelem);
	while (ip->type != IP_DEFLAB)
		ip = DLIST_NEXT(ip,qelem);
	/* Now ip is at the beginning */
	for (;;) {
		ip = DLIST_NEXT(ip,qelem);
		if (ip == ipole)
			break;
		p = tmpalloc(sizeof(struct dlnod));
		p->labno = 0;
		p->dlip = ip;
		switch (ip->type) {
		case IP_DEFLAB:
			p->op = LABEL;
			p->labno = ip->ip_lbl;
			break;

		case IP_NODE:
			q = ip->ip_node;
			switch (q->n_op) {
			case GOTO:
				p->op = JBR;
				p->labno = q->n_left->n_lval;
				break;
			case CBRANCH:
				p->op = CBR;
				p->labno = q->n_right->n_lval;
				break;
			case ASSIGN:
				/* remove ASSIGN to self for regs */
				if (q->n_left->n_op == REG && 
				    q->n_right->n_op == REG &&
				    regno(q->n_left) == regno(q->n_right)) {
					nip = DLIST_PREV(ip, qelem);
					tfree(q);
					DLIST_REMOVE(ip, qelem);
					ip = nip;
					continue;
				}
				/* FALLTHROUGH */
			default:
				p->op = STMT;
				break;
			}
			break;

		case IP_ASM:
			p->op = STMT;
			break;

		case IP_EPILOG:
			p->op = EROU;
			break;

		default:
			comperr("listsetup: bad ip node %d", ip->type);
		}
		p->forw = 0;
		p->back = lastp;
		lastp->forw = p;
		lastp = p;
		p->ref = 0;
	}
}

static struct dlnod *
nonlab(struct dlnod *p)
{
	while (p && p->op==LABEL)
		p = p->forw;
	return(p);
}

static void
iprem(struct dlnod *p)
{
	if (p->dlip->type == IP_NODE)
		tfree(p->dlip->ip_node);
	DLIST_REMOVE(p->dlip, qelem);
}

static void
decref(struct dlnod *p)
{
	if (--p->refc <= 0) {
		iprem(p);
		p->back->forw = p->forw;
		p->forw->back = p->back;
	}
}

static void
setlab(struct dlnod *p, int labno)
{
	p->labno = labno;
	if (p->op == JBR)
		p->dlip->ip_node->n_left->n_lval = labno;
	else if (p->op == CBR) {
		p->dlip->ip_node->n_right->n_lval = labno;
		p->dlip->ip_node->n_left->n_label = labno;
	} else
		comperr("setlab bad op %d", p->op);
}

/*
 * Label reference counting and removal of unused labels.
 */
#define	LABHS 127
static void
refcount(struct p2env *p2e, struct dlnod *dl)
{
	struct dlnod *p, *lp;
	struct dlnod *labhash[LABHS];
	struct dlnod **hp, *tp;

	/* Clear label hash */
	for (hp = labhash; hp < &labhash[LABHS];)
		*hp++ = 0;
	/* Enter labels into hash.  Later overwrites earlier */
	for (p = dl->forw; p!=0; p = p->forw)
		if (p->op==LABEL) {
			labhash[p->labno % LABHS] = p;
			p->refc = 0;
		}

	/* search for jumps to labels and fill in reference */
	for (p = dl->forw; p!=0; p = p->forw) {
		if (p->op==JBR || p->op==CBR) {
			p->ref = 0;
			lp = labhash[p->labno % LABHS];
			if (lp==0 || p->labno!=lp->labno)
			    for (lp = dl->forw; lp!=0; lp = lp->forw) {
				if (lp->op==LABEL && p->labno==lp->labno)
					break;
			    }
			if (lp) {
				tp = nonlab(lp)->back;
				if (tp!=lp) {
					setlab(p, tp->labno);
					lp = tp;
				}
				p->ref = lp;
				lp->refc++;
			}
		}
	}
	for (p = dl->forw; p!=0; p = p->forw)
		if (p->op==LABEL && p->refc==0 && (lp = nonlab(p))->op)
			decref(p);
}

static int nchange;

static struct dlnod *
codemove(struct dlnod *p)
{
	struct dlnod *p1, *p2, *p3;
#ifdef notyet
	struct dlnod *t, *tl;
	int n;
#endif

	p1 = p;
	if (p1->op!=JBR || (p2 = p1->ref)==0)
		return(p1);
	while (p2->op == LABEL)
		if ((p2 = p2->back) == 0)
			return(p1);
	if (p2->op!=JBR)
		goto ivloop;
	if (p1==p2)
		return(p1);
	p2 = p2->forw;
	p3 = p1->ref;
	while (p3) {
		if (p3->op==JBR) {
			if (p1==p3 || p1->forw==p3 || p1->back==p3)
				return(p1);
			nchange++;
			p1->back->forw = p2;
			p1->dlip->qelem.q_back->qelem.q_forw = p2->dlip;

			p1->forw->back = p3;
			p1->dlip->qelem.q_forw->qelem.q_back = p3->dlip;


			p2->back->forw = p3->forw;
			p2->dlip->qelem.q_back->qelem.q_forw = p3->forw->dlip;

			p3->forw->back = p2->back;
			p3->dlip->qelem.q_forw->qelem.q_back = p2->back->dlip;

			p2->back = p1->back;
			p2->dlip->qelem.q_back = p1->dlip->qelem.q_back;

			p3->forw = p1->forw;
			p3->dlip->qelem.q_forw = p1->forw->dlip;

			decref(p1->ref);
			if (p1->dlip->type == IP_NODE)
				tfree(p1->dlip->ip_node);

			return(p2);
		} else
			p3 = p3->forw;
	}
	return(p1);

ivloop:
	if (p1->forw->op!=LABEL)
		return(p1);
	return(p1);

#ifdef notyet
	p3 = p2 = p2->forw;
	n = 16;
	do {
		if ((p3 = p3->forw) == 0 || p3==p1 || --n==0)
			return(p1);
	} while (p3->op!=CBR || p3->labno!=p1->forw->labno);
	do 
		if ((p1 = p1->back) == 0)
			return(p);
	while (p1!=p3);
	p1 = p;
	tl = insertl(p1);
	p3->subop = revbr[p3->subop];
	decref(p3->ref);
		p2->back->forw = p1;
	p3->forw->back = p1;
	p1->back->forw = p2;
	p1->forw->back = p3;
	t = p1->back;
	p1->back = p2->back;
	p2->back = t;
	t = p1->forw;
	p1->forw = p3->forw;
	p3->forw = t;
	p2 = insertl(p1->forw);
	p3->labno = p2->labno;
	p3->ref = p2;
	decref(tl);
	if (tl->refc<=0)
		nrlab--;
	nchange++;
	return(p3);
#endif
}

static void
iterate(struct p2env *p2e, struct dlnod *dl)
{
	struct dlnod *p, *rp, *p1;
	extern int negrel[];
	extern size_t negrelsize;
	int i;

	nchange = 0;
	for (p = dl->forw; p!=0; p = p->forw) {
		if ((p->op==JBR||p->op==CBR) && p->ref) {
			/* Resolves:
			 * jbr L7
			 * ...
			 * L7: jbr L8
			 */
			rp = nonlab(p->ref);
			if (rp->op==JBR && rp->labno && p->labno!=rp->labno) {
				setlab(p, rp->labno);
				decref(p->ref);
				rp->ref->refc++;
				p->ref = rp->ref;
				nchange++;
			}
		}
		if (p->op==CBR && (p1 = p->forw)->op==JBR) {
			/* Resolves:
			 * cbr L7
			 * jbr L8
			 * L7:
			 */
			rp = p->ref;
			do
				rp = rp->back;
			while (rp->op==LABEL);
			if (rp==p1) {
				decref(p->ref);
				p->ref = p1->ref;
				setlab(p, p1->labno);

				iprem(p1);

				p1->forw->back = p;
				p->forw = p1->forw;

				i = p->dlip->ip_node->n_left->n_op;
				if (i < EQ || i - EQ >= (int)negrelsize)
					comperr("deljumps: unexpected op");
				p->dlip->ip_node->n_left->n_op = negrel[i - EQ];
				nchange++;
			}
		}
		if (p->op == JBR) {
			/* Removes dead code */
			while (p->forw && p->forw->op!=LABEL &&
			    p->forw->op!=EROU) {
				nchange++;
				if (p->forw->ref)
					decref(p->forw->ref);

				iprem(p->forw);

				p->forw = p->forw->forw;
				p->forw->back = p;
			}
			rp = p->forw;
			while (rp && rp->op==LABEL) {
				if (p->ref == rp) {
					p->back->forw = p->forw;
					p->forw->back = p->back;
					iprem(p);
					p = p->back;
					decref(rp);
					nchange++;
					break;
				}
				rp = rp->forw;
			}
		}
		if (p->op == JBR) {
			/* xjump(p); * needs tree rewrite; not yet */
			p = codemove(p);
		}
	}
}

void
deljumps(struct p2env *p2e)
{
	struct interpass *ipole = &p2e->ipole;
	struct dlnod dln;
	MARK mark;

	markset(&mark);

	memset(&dln, 0, sizeof(dln));
	listsetup(ipole, &dln);
	do {
		refcount(p2e, &dln);
		do {
			iterate(p2e, &dln);
		} while (nchange);
#ifdef notyet
		comjump();
#endif
	} while (nchange);

	markfree(&mark);
}

void
optdump(struct interpass *ip)
{
	static char *nm[] = { "node", "prolog", "newblk", "epilog", "locctr",
		"deflab", "defnam", "asm" };
	printf("type %s\n", nm[ip->type-1]);
	switch (ip->type) {
	case IP_NODE:
#ifdef PCC_DEBUG
		fwalk(ip->ip_node, e2print, 0);
#endif
		break;
	case IP_DEFLAB:
		printf("label " LABFMT "\n", ip->ip_lbl);
		break;
	case IP_ASM:
		printf(": %s\n", ip->ip_asm);
		break;
	}
}

/*
 * Build the basic blocks, algorithm 9.1, pp 529 in Compilers.
 *
 * Also fills the labelinfo struct with information about which bblocks
 * that contain which label.
 */

void
bblocks_build(struct p2env *p2e)
{
	struct interpass *ipole = &p2e->ipole;
	struct interpass *ip;
	struct basicblock *bb = NULL;
	int low, high;
	int count = 0;
	int i;

	BDEBUG(("bblocks_build (%p, %p)\n", &p2e->labinfo, &p2e->bbinfo));
	low = p2e->ipp->ip_lblnum;
	high = p2e->epp->ip_lblnum;

	/* 
	 * First statement is a leader.
	 * Any statement that is target of a jump is a leader.
	 * Any statement that immediately follows a jump is a leader.
	 */
	DLIST_INIT(&p2e->bblocks, bbelem);
	DLIST_FOREACH(ip, ipole, qelem) {
		if (bb == NULL || (ip->type == IP_EPILOG) ||
		    (ip->type == IP_DEFLAB) || (ip->type == IP_DEFNAM)) {
			bb = tmpalloc(sizeof(struct basicblock));
			bb->first = ip;
			SLIST_INIT(&bb->parents);
			bb->ch[0] = bb->ch[1] = NULL;
			bb->dfnum = 0;
			bb->dfparent = 0;
			bb->semi = 0;
			bb->ancestor = 0;
			bb->idom = 0;
			bb->samedom = 0;
			bb->bucket = NULL;
			bb->df = NULL;
			bb->dfchildren = NULL;
			bb->Aorig = NULL;
			bb->Aphi = NULL;
			SLIST_INIT(&bb->phi);
			bb->bbnum = count;
			DLIST_INSERT_BEFORE(&p2e->bblocks, bb, bbelem);
			count++;
		}
		bb->last = ip;
		if ((ip->type == IP_NODE) && (ip->ip_node->n_op == GOTO || 
		    ip->ip_node->n_op == CBRANCH))
			bb = NULL;
		if (ip->type == IP_PROLOG)
			bb = NULL;
	}
	p2e->nbblocks = count;

	if (b2debug) {
		printf("Basic blocks in func: %d, low %d, high %d\n",
		    count, low, high);
		DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
			printf("bb(%d) %p: first %p last %p\n", bb->bbnum, bb,
			    bb->first, bb->last);
		}
	}

	p2e->labinfo.low = low;
	p2e->labinfo.size = high - low + 1;
	p2e->labinfo.arr = tmpalloc(p2e->labinfo.size * sizeof(struct basicblock *));
	for (i = 0; i < p2e->labinfo.size; i++) {
		p2e->labinfo.arr[i] = NULL;
	}
	
	p2e->bbinfo.size = count + 1;
	p2e->bbinfo.arr = tmpalloc(p2e->bbinfo.size * sizeof(struct basicblock *));
	for (i = 0; i < p2e->bbinfo.size; i++) {
		p2e->bbinfo.arr[i] = NULL;
	}

	/* Build the label table */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		if (bb->first->type == IP_DEFLAB)
			p2e->labinfo.arr[bb->first->ip_lbl - low] = bb;
	}

	if (b2debug) {
		DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
			printf("bblock %d\n", bb->bbnum);
			for (ip = bb->first; ip != bb->last;
			    ip = DLIST_NEXT(ip, qelem)) {
				printip2(ip);
			}
			printip2(ip);
		}

		printf("Label table:\n");
		for (i = 0; i < p2e->labinfo.size; i++)
			if (p2e->labinfo.arr[i])
				printf("Label %d bblock %p\n", i+low,
				    p2e->labinfo.arr[i]);
	}
}

/*
 * Build the control flow graph.
 */

void
cfg_build(struct p2env *p2e)
{
	/* Child and parent nodes */
	struct cfgnode *cnode; 
	struct cfgnode *pnode;
	struct basicblock *bb;
	
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {

		if (bb->first->type == IP_EPILOG) {
			break;
		}

		cnode = tmpalloc(sizeof(struct cfgnode));
		pnode = tmpalloc(sizeof(struct cfgnode));
		pnode->bblock = bb;

		if ((bb->last->type == IP_NODE) && 
		    (bb->last->ip_node->n_op == GOTO) &&
		    (bb->last->ip_node->n_left->n_op == ICON))  {
			if (bb->last->ip_node->n_left->n_lval - p2e->labinfo.low > 
			    p2e->labinfo.size) {
				comperr("Label out of range: %d, base %d", 
					bb->last->ip_node->n_left->n_lval, 
					p2e->labinfo.low);
			}
			cnode->bblock = p2e->labinfo.arr[bb->last->ip_node->n_left->n_lval - p2e->labinfo.low];
			SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
			CHADD(bb, cnode);
			continue;
		}
		if ((bb->last->type == IP_NODE) && 
		    (bb->last->ip_node->n_op == CBRANCH)) {
			if (bb->last->ip_node->n_right->n_lval - p2e->labinfo.low > 
			    p2e->labinfo.size) 
				comperr("Label out of range: %d", 
					bb->last->ip_node->n_left->n_lval);

			cnode->bblock = p2e->labinfo.arr[bb->last->ip_node->n_right->n_lval - p2e->labinfo.low];
			SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
			CHADD(bb, cnode);
			cnode = tmpalloc(sizeof(struct cfgnode));
			pnode = tmpalloc(sizeof(struct cfgnode));
			pnode->bblock = bb;
		}

		cnode->bblock = DLIST_NEXT(bb, bbelem);
		SLIST_INSERT_LAST(&cnode->bblock->parents, pnode, cfgelem);
		CHADD(bb, cnode);
	}
}

void
cfg_dfs(struct basicblock *bb, unsigned int parent, struct bblockinfo *bbinfo)
{
	struct cfgnode **cn;

	if (bb->dfnum != 0)
		return;

	bb->dfnum = ++dfsnum;
	bb->dfparent = parent;
	bbinfo->arr[bb->dfnum] = bb;
	FORCH(cn, bb->ch)
		cfg_dfs((*cn)->bblock, bb->dfnum, bbinfo);
	/* Don't bring in unreachable nodes in the future */
	bbinfo->size = dfsnum + 1;
}

static bittype *
setalloc(int nelem)
{
	bittype *b;
	int sz = (nelem+NUMBITS-1)/NUMBITS;

	b = tmpalloc(sz * sizeof(bittype));
	memset(b, 0, sz * sizeof(bittype));
	return b;
}

/*
 * Algorithm 19.9, pp 414 from Appel.
 */

void
dominators(struct p2env *p2e)
{
	struct cfgnode *cnode;
	struct basicblock *bb, *y, *v;
	struct basicblock *s, *sprime, *p;
	int h, i;

	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		bb->bucket = setalloc(p2e->bbinfo.size);
		bb->df = setalloc(p2e->bbinfo.size);
		bb->dfchildren = setalloc(p2e->bbinfo.size);
	}

	dfsnum = 0;
	cfg_dfs(DLIST_NEXT(&p2e->bblocks, bbelem), 0, &p2e->bbinfo);

	if (b2debug) {
		struct basicblock *bbb;
		struct cfgnode *ccnode, **cn;

		DLIST_FOREACH(bbb, &p2e->bblocks, bbelem) {
			printf("Basic block %d, parents: ", bbb->dfnum);
			SLIST_FOREACH(ccnode, &bbb->parents, cfgelem) {
				printf("%d, ", ccnode->bblock->dfnum);
			}
			printf("\nChildren: ");
			FORCH(cn, bbb->ch)
				printf("%d, ", (*cn)->bblock->dfnum);
			printf("\n");
		}
	}

	for(h = p2e->bbinfo.size - 1; h > 1; h--) {
		bb = p2e->bbinfo.arr[h];
		p = s = p2e->bbinfo.arr[bb->dfparent];
		SLIST_FOREACH(cnode, &bb->parents, cfgelem) {
			if (cnode->bblock->dfnum ==0)
				continue; /* Ignore unreachable code */

			if (cnode->bblock->dfnum <= bb->dfnum) 
				sprime = cnode->bblock;
			else 
				sprime = p2e->bbinfo.arr[ancestorwithlowestsemi
					      (cnode->bblock, &p2e->bbinfo)->semi];
			if (sprime->dfnum < s->dfnum)
				s = sprime;
		}
		bb->semi = s->dfnum;
		BITSET(s->bucket, bb->dfnum);
		link(p, bb);
		for (i = 1; i < p2e->bbinfo.size; i++) {
			if(TESTBIT(p->bucket, i)) {
				v = p2e->bbinfo.arr[i];
				y = ancestorwithlowestsemi(v, &p2e->bbinfo);
				if (y->semi == v->semi) 
					v->idom = p->dfnum;
				else
					v->samedom = y->dfnum;
			}
		}
		memset(p->bucket, 0, (p2e->bbinfo.size + 7)/8);
	}

	if (b2debug) {
		printf("Num\tSemi\tAncest\tidom\n");
		DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
			printf("%d\t%d\t%d\t%d\n", bb->dfnum, bb->semi,
			    bb->ancestor, bb->idom);
		}
	}

	for(h = 2; h < p2e->bbinfo.size; h++) {
		bb = p2e->bbinfo.arr[h];
		if (bb->samedom != 0) {
			bb->idom = p2e->bbinfo.arr[bb->samedom]->idom;
		}
	}
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		if (bb->idom != 0 && bb->idom != bb->dfnum) {
			BDEBUG(("Setting child %d of %d\n",
			    bb->dfnum, p2e->bbinfo.arr[bb->idom]->dfnum));
			BITSET(p2e->bbinfo.arr[bb->idom]->dfchildren, bb->dfnum);
		}
	}
}


struct basicblock *
ancestorwithlowestsemi(struct basicblock *bblock, struct bblockinfo *bbinfo)
{
	struct basicblock *u = bblock;
	struct basicblock *v = bblock;

	while (v->ancestor != 0) {
		if (bbinfo->arr[v->semi]->dfnum < 
		    bbinfo->arr[u->semi]->dfnum) 
			u = v;
		v = bbinfo->arr[v->ancestor];
	}
	return u;
}

void
link(struct basicblock *parent, struct basicblock *child)
{
	child->ancestor = parent->dfnum;
}

void
computeDF(struct p2env *p2e, struct basicblock *bblock)
{
	struct cfgnode **cn;
	int h, i;
	
	FORCH(cn, bblock->ch) {
		if ((*cn)->bblock->idom != bblock->dfnum)
			BITSET(bblock->df, (*cn)->bblock->dfnum);
	}
	for (h = 1; h < p2e->bbinfo.size; h++) {
		if (!TESTBIT(bblock->dfchildren, h))
			continue;
		computeDF(p2e, p2e->bbinfo.arr[h]);
		for (i = 1; i < p2e->bbinfo.size; i++) {
			if (TESTBIT(p2e->bbinfo.arr[h]->df, i) && 
			    (p2e->bbinfo.arr[i] == bblock ||
			     (bblock->dfnum != p2e->bbinfo.arr[i]->idom))) 
			    BITSET(bblock->df, i);
		}
	}
}

void printDF(struct p2env *p2e)
{
	struct basicblock *bb;
	int i;

	printf("Dominance frontiers:\n");
    
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		printf("bb %d : ", bb->dfnum);
	
		for (i=1; i < p2e->bbinfo.size;i++) {
			if (TESTBIT(bb->df,i)) {
				printf("%d ",i);
			}
		}
	    
		printf("\n");
	}
    
}



static struct basicblock *currbb;
static struct interpass *currip;

/* Helper function for findTemps, Find assignment nodes. */
static void
searchasg(NODE *p, void *arg)
{
	struct pvarinfo *pv;
	int tempnr;
	struct varstack *stacke;
    
	if (p->n_op != ASSIGN)
		return;

	if (p->n_left->n_op != TEMP)
		return;

	tempnr=regno(p->n_left)-defsites.low;
    
	BITSET(currbb->Aorig, tempnr);
	
	pv = tmpcalloc(sizeof(struct pvarinfo));
	pv->next = defsites.arr[tempnr];
	pv->bb = currbb;
	pv->n_type = p->n_left->n_type;
	
	defsites.arr[tempnr] = pv;
	
	
	if (SLIST_FIRST(&defsites.stack[tempnr])==NULL) {
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=0;
		SLIST_INSERT_FIRST(&defsites.stack[tempnr],stacke,varstackelem);
	}
}

/* Walk the interpass looking for assignment nodes. */
void findTemps(struct interpass *ip)
{
	if (ip->type != IP_NODE)
		return;

	currip = ip;

	walkf(ip->ip_node, searchasg, 0);
}

/*
 * Algorithm 19.6 from Appel.
 */

void
placePhiFunctions(struct p2env *p2e)
{
	struct basicblock *bb;
	struct basicblock *y;
	struct interpass *ip;
	int maxtmp, i, j, k;
	struct pvarinfo *n;
	struct cfgnode *cnode;
	TWORD ntype;
	struct pvarinfo *pv;
	struct phiinfo *phi;
	int phifound;

	bb = DLIST_NEXT(&p2e->bblocks, bbelem);
	defsites.low = ((struct interpass_prolog *)bb->first)->ip_tmpnum;
	bb = DLIST_PREV(&p2e->bblocks, bbelem);
	maxtmp = ((struct interpass_prolog *)bb->first)->ip_tmpnum;
	defsites.size = maxtmp - defsites.low + 1;
	defsites.arr = tmpcalloc(defsites.size*sizeof(struct pvarinfo *));
	defsites.stack = tmpcalloc(defsites.size*sizeof(SLIST_HEAD(, varstack)));
	
	for (i=0;i<defsites.size;i++)
		SLIST_INIT(&defsites.stack[i]);	
	
	/* Find all defsites */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		currbb = bb;
		ip = bb->first;
		bb->Aorig = setalloc(defsites.size);
		bb->Aphi = setalloc(defsites.size);
		
		while (ip != bb->last) {
			findTemps(ip);
			ip = DLIST_NEXT(ip, qelem);
		}
		/* Make sure we get the last statement in the bblock */
		findTemps(ip);
	}

	/* For each variable */
	for (i = 0; i < defsites.size; i++) {
		/* While W not empty */
		while (defsites.arr[i] != NULL) {
			/* Remove some node n from W */
			n = defsites.arr[i];
			defsites.arr[i] = n->next;
			/* For each y in n->bb->df */
			for (j = 0; j < p2e->bbinfo.size; j++) {
				if (!TESTBIT(n->bb->df, j))
					continue;
				
				if (TESTBIT(p2e->bbinfo.arr[j]->Aphi, i))
					continue;

				y=p2e->bbinfo.arr[j];
				ntype = n->n_type;
				k = 0;
				/* Amount of predecessors for y */
				SLIST_FOREACH(cnode, &y->parents, cfgelem) 
					k++;
				/* Construct phi(...) 
				*/
			    
				phifound=0;
			    
				SLIST_FOREACH(phi, &y->phi, phielem) {
				    if (phi->tmpregno==i+defsites.low)
					phifound++;
				}
			    
				if (phifound==0) {
					if (b2debug)
					    printf("Phi in %d(%d) (%p) for %d\n",
					    y->dfnum,y->bbnum,y,i+defsites.low);

					/* If no live in, no phi node needed */
					if (!TESTBIT(y->in,
					    (i+defsites.low-p2e->ipp->ip_tmpnum+MAXREGS))) {
					if (b2debug)
					printf("tmp %d bb %d unused, no phi\n",
					    i+defsites.low, y->bbnum);
						/* No live in */
						BITSET(p2e->bbinfo.arr[j]->Aphi, i);
						continue;
					}

					phi = tmpcalloc(sizeof(struct phiinfo));
			    
					phi->tmpregno=i+defsites.low;
					phi->size=k;
					phi->n_type=ntype;
					phi->intmpregno=tmpcalloc(k*sizeof(int));
			    
					SLIST_INSERT_LAST(&y->phi,phi,phielem);
				} else {
				    if (b2debug)
					printf("Phi already in %d for %d\n",y->dfnum,i+defsites.low);
				}

				BITSET(p2e->bbinfo.arr[j]->Aphi, i);
				if (!TESTBIT(p2e->bbinfo.arr[j]->Aorig, i)) {
					pv = tmpalloc(sizeof(struct pvarinfo));
					pv->bb = y;
				        pv->n_type=ntype;
					pv->next = defsites.arr[i];
					defsites.arr[i] = pv;
				}
			}
		}
	}
}

/* Helper function for renamevar. */
static void
renamevarhelper(struct p2env *p2e,NODE *t,void *poplistarg)
{	
	SLIST_HEAD(, varstack) *poplist=poplistarg;
	int opty;
	int tempnr;
	int newtempnr;
	int x;
	struct varstack *stacke;
	
	if (t->n_op == ASSIGN && t->n_left->n_op == TEMP) {
		renamevarhelper(p2e,t->n_right,poplist);
				
		tempnr=regno(t->n_left)-defsites.low;
		
		newtempnr=p2e->epp->ip_tmpnum++;
		regno(t->n_left)=newtempnr;
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=newtempnr;
		SLIST_INSERT_FIRST(&defsites.stack[tempnr],stacke,varstackelem);
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=tempnr;
		SLIST_INSERT_FIRST(poplist,stacke,varstackelem);
	} else {
		if (t->n_op == TEMP) {
			tempnr=regno(t)-defsites.low;
		
			if (SLIST_FIRST(&defsites.stack[tempnr])!=NULL) {
				x=SLIST_FIRST(&defsites.stack[tempnr])->tmpregno;
				regno(t)=x;
			}
		}
		
		opty = optype(t->n_op);
		
		if (opty != LTYPE)
			renamevarhelper(p2e, t->n_left,poplist);
		if (opty == BITYPE)
			renamevarhelper(p2e, t->n_right,poplist);
	}
}


void
renamevar(struct p2env *p2e,struct basicblock *bb)
{
    	struct interpass *ip;
	int h,j;
	SLIST_HEAD(, varstack) poplist;
	struct varstack *stacke;
	struct cfgnode *cfgn2, **cn;
	int tmpregno,newtmpregno;
	struct phiinfo *phi;

	SLIST_INIT(&poplist);

	SLIST_FOREACH(phi,&bb->phi,phielem) {
		tmpregno=phi->tmpregno-defsites.low;
		
		newtmpregno=p2e->epp->ip_tmpnum++;
		phi->newtmpregno=newtmpregno;

		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=newtmpregno;
		SLIST_INSERT_FIRST(&defsites.stack[tmpregno],stacke,varstackelem);
		
		stacke=tmpcalloc(sizeof (struct varstack));
		stacke->tmpregno=tmpregno;
		SLIST_INSERT_FIRST(&poplist,stacke,varstackelem);		
	}


	ip=bb->first;

	while (1) {		
		if ( ip->type == IP_NODE) {
			renamevarhelper(p2e,ip->ip_node,&poplist);
		}

		if (ip==bb->last)
			break;

		ip = DLIST_NEXT(ip, qelem);
	}

	FORCH(cn, bb->ch) {
		j=0;

		SLIST_FOREACH(cfgn2, &(*cn)->bblock->parents, cfgelem) { 
			if (cfgn2->bblock->dfnum==bb->dfnum)
				break;
			
			j++;
		}

		SLIST_FOREACH(phi,&(*cn)->bblock->phi,phielem) {
			phi->intmpregno[j]=SLIST_FIRST(&defsites.stack[phi->tmpregno-defsites.low])->tmpregno;
		}
	}

	for (h = 1; h < p2e->bbinfo.size; h++) {
		if (!TESTBIT(bb->dfchildren, h))
			continue;

		renamevar(p2e,p2e->bbinfo.arr[h]);
	}

	SLIST_FOREACH(stacke,&poplist,varstackelem) {
		tmpregno=stacke->tmpregno;
		
		defsites.stack[tmpregno].q_forw=defsites.stack[tmpregno].q_forw->varstackelem.q_forw;
	}
}

enum pred_type {
    pred_unknown    = 0,
    pred_goto       = 1,
    pred_cond       = 2,
    pred_falltrough = 3,
} ;

void
removephi(struct p2env *p2e)
{
	struct basicblock *bb,*bbparent;
	struct cfgnode *cfgn;
	struct phiinfo *phi;
	int i;
	struct interpass *ip;
	struct interpass *pip;
	TWORD n_type;

	enum pred_type complex = pred_unknown ;

	int label=0;
	int newlabel;

	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {		
		SLIST_FOREACH(phi,&bb->phi,phielem) {
			/* Look at only one, notice break at end */
			i=0;
			
			SLIST_FOREACH(cfgn, &bb->parents, cfgelem) { 
				bbparent=cfgn->bblock;
				
				pip=bbparent->last;
				
				complex = pred_unknown ;
				
				BDEBUG(("removephi: %p in %d",pip,bb->dfnum));

				if (pip->type == IP_NODE && pip->ip_node->n_op == GOTO) {
					BDEBUG((" GOTO "));
					label = (int)pip->ip_node->n_left->n_lval;
					complex = pred_goto ;
				} else if (pip->type == IP_NODE && pip->ip_node->n_op == CBRANCH) {
					BDEBUG((" CBRANCH "));
					label = (int)pip->ip_node->n_right->n_lval;
					
					if (bb==p2e->labinfo.arr[label - p2e->ipp->ip_lblnum])
						complex = pred_cond ;
					else
						complex = pred_falltrough ;

				} else if (DLIST_PREV(bb, bbelem) == bbparent) {
					complex = pred_falltrough ;
				} else {
					    /* PANIC */
					comperr("Assumption blown in rem-phi") ;
				}
       
				BDEBUG((" Complex: %d ",complex)) ;

				switch (complex) {
				  case pred_goto:
					/* gotos can only go to this place. No bounce tab needed */
					SLIST_FOREACH(phi,&bb->phi,phielem) {
						if (phi->intmpregno[i]>0) {
							n_type=phi->n_type;
							ip = ipnode(mkbinode(ASSIGN,
							     mktemp(phi->newtmpregno, n_type),
							     mktemp(phi->intmpregno[i],n_type),
							     n_type));
							BDEBUG(("(%p, %d -> %d) ", ip, phi->intmpregno[i], phi->newtmpregno));
				
							DLIST_INSERT_BEFORE((bbparent->last), ip, qelem);
						}
					}
					break ;
				  case pred_cond:
					/* Here, we need a jump pad */
					newlabel=getlab2();
			
					ip = tmpalloc(sizeof(struct interpass));
					ip->type = IP_DEFLAB;
					/* Line number?? ip->lineno; */
					ip->ip_lbl = newlabel;
					DLIST_INSERT_BEFORE((bb->first), ip, qelem);

					SLIST_FOREACH(phi,&bb->phi,phielem) {
						if (phi->intmpregno[i]>0) {
							n_type=phi->n_type;
							ip = ipnode(mkbinode(ASSIGN,
							     mktemp(phi->newtmpregno, n_type),
							     mktemp(phi->intmpregno[i],n_type),
							     n_type));

							BDEBUG(("(%p, %d -> %d) ", ip, phi->intmpregno[i], phi->newtmpregno));
							DLIST_INSERT_BEFORE((bb->first), ip, qelem);
						}
					}
					/* add a jump to us */
					ip = ipnode(mkunode(GOTO, mklnode(ICON, label, 0, INT), 0, INT));
					DLIST_INSERT_BEFORE((bb->first), ip, qelem);
					pip->ip_node->n_right->n_lval=newlabel;
					if (!logop(pip->ip_node->n_left->n_op))
						comperr("SSA not logop");
					pip->ip_node->n_left->n_label=newlabel;
					break ;
				  case pred_falltrough:
					if (bb->first->type == IP_DEFLAB) { 
						label = bb->first->ip_lbl; 
						BDEBUG(("falltrough label %d\n", label));
					} else {
						comperr("BBlock has no label?") ;
					}

					/* 
					 * add a jump to us. We _will_ be, or already have, added code in between.
					 * The code is created in the wrong order and switched at the insert, thus
					 * comming out correctly
					 */

					ip = ipnode(mkunode(GOTO, mklnode(ICON, label, 0, INT), 0, INT));
					DLIST_INSERT_AFTER((bbparent->last), ip, qelem);

					/* Add the code to the end, add a jump to us. */
					SLIST_FOREACH(phi,&bb->phi,phielem) {
						if (phi->intmpregno[i]>0) {
							n_type=phi->n_type;
							ip = ipnode(mkbinode(ASSIGN,
								mktemp(phi->newtmpregno, n_type),
								mktemp(phi->intmpregno[i],n_type),
								n_type));

							BDEBUG(("(%p, %d -> %d) ", ip, phi->intmpregno[i], phi->newtmpregno));
							DLIST_INSERT_AFTER((bbparent->last), ip, qelem);
						}
					}
					break ;
				default:
					comperr("assumption blown, complex is %d\n", complex) ;
				}
				BDEBUG(("\n"));
				i++;
			}
			break;
		}
	}
}

    
/*
 * Remove unreachable nodes in the CFG.
 */ 

void
remunreach(struct p2env *p2e)
{
	struct basicblock *bb, *nbb;
	struct interpass *next, *ctree;

	bb = DLIST_NEXT(&p2e->bblocks, bbelem);
	while (bb != &p2e->bblocks) {
		nbb = DLIST_NEXT(bb, bbelem);

		/* Code with dfnum 0 is unreachable */
		if (bb->dfnum != 0) {
			bb = nbb;
			continue;
		}

		/* Need the epilogue node for other parts of the
		   compiler, set its label to 0 and backend will
		   handle it. */ 
		if (bb->first->type == IP_EPILOG) {
			bb->first->ip_lbl = 0;
			bb = nbb;
			continue;
		}

		next = bb->first;
		do {
			ctree = next;
			next = DLIST_NEXT(ctree, qelem);
			
			if (ctree->type == IP_NODE)
				tfree(ctree->ip_node);
			DLIST_REMOVE(ctree, qelem);
		} while (ctree != bb->last);
			
		DLIST_REMOVE(bb, bbelem);
		bb = nbb;
	}
}

static void
printip2(struct interpass *ip)
{
	static char *foo[] = {
	   0, "NODE", "PROLOG", "STKOFF", "EPILOG", "DEFLAB", "DEFNAM", "ASM" };
	struct interpass_prolog *ipplg, *epplg;
	unsigned i;

	if (ip->type > MAXIP)
		printf("IP(%d) (%p): ", ip->type, ip);
	else
		printf("%s (%p): ", foo[ip->type], ip);
	switch (ip->type) {
	case IP_NODE: printf("\n");
#ifdef PCC_DEBUG
	fwalk(ip->ip_node, e2print, 0); break;
#endif
	case IP_PROLOG:
		ipplg = (struct interpass_prolog *)ip;
		printf("%s %s regs",
		    ipplg->ipp_name, ipplg->ipp_vis ? "(local)" : "");
		for (i = 0; i < NIPPREGS; i++)
			printf("%s0x%lx", i? ":" : " ",
			    (long) ipplg->ipp_regs[i]);
		printf(" autos %d mintemp %d minlbl %d\n",
		    ipplg->ipp_autos, ipplg->ip_tmpnum, ipplg->ip_lblnum);
		break;
	case IP_EPILOG:
		epplg = (struct interpass_prolog *)ip;
		printf("%s %s regs",
		    epplg->ipp_name, epplg->ipp_vis ? "(local)" : "");
		for (i = 0; i < NIPPREGS; i++)
			printf("%s0x%lx", i? ":" : " ",
			    (long) epplg->ipp_regs[i]);
		printf(" autos %d mintemp %d minlbl %d\n",
		    epplg->ipp_autos, epplg->ip_tmpnum, epplg->ip_lblnum);
		break;
	case IP_DEFLAB: printf(LABFMT "\n", ip->ip_lbl); break;
	case IP_DEFNAM: printf("\n"); break;
	case IP_ASM: printf("%s\n", ip->ip_asm); break;
	default:
		break;
	}
}

void
printip(struct interpass *pole)
{
	struct interpass *ip;

	DLIST_FOREACH(ip, pole, qelem)
		printip2(ip);
}

#ifdef PCC_DEBUG
void flownodeprint(NODE *p,FILE *flowdiagramfile);

void
flownodeprint(NODE *p,FILE *flowdiagramfile)
{	
	int opty;
	char *o;

	fprintf(flowdiagramfile,"{");

	o=opst[p->n_op];
	
	while (*o != 0) {
		if (*o=='<' || *o=='>')
			fputc('\\',flowdiagramfile);
		
		fputc(*o,flowdiagramfile);
		o++;
	}
	
	
	switch( p->n_op ) {			
		case REG:
			fprintf(flowdiagramfile, " %s", rnames[p->n_rval] );
			break;
			
		case TEMP:
			fprintf(flowdiagramfile, " %d", regno(p));
			break;
			
		case XASM:
		case XARG:
			fprintf(flowdiagramfile, " '%s'", p->n_name);
			break;
			
		case ICON:
		case NAME:
		case OREG:
			fprintf(flowdiagramfile, " " );
			adrput(flowdiagramfile, p );
			break;
			
		case STCALL:
		case USTCALL:
		case STARG:
		case STASG:
			fprintf(flowdiagramfile, " size=%d", p->n_stsize );
			fprintf(flowdiagramfile, " align=%d", p->n_stalign );
			break;
	}
	
	opty = optype(p->n_op);
	
	if (opty != LTYPE) {
		fprintf(flowdiagramfile,"| {");
	
		flownodeprint(p->n_left,flowdiagramfile);
	
		if (opty == BITYPE) {
			fprintf(flowdiagramfile,"|");
			flownodeprint(p->n_right,flowdiagramfile);
		}
		fprintf(flowdiagramfile,"}");
	}
	
	fprintf(flowdiagramfile,"}");
}

void
printflowdiagram(struct p2env *p2e, char *type) {
	struct basicblock *bbb;
	struct cfgnode **cn;
	struct interpass *ip;
	struct interpass_prolog *plg;
	struct phiinfo *phi;
	char *name;
	char *filename;
	int filenamesize;
	char *ext=".dot";
	FILE *flowdiagramfile;
	
	if (!g2debug)
		return;
	
	bbb=DLIST_NEXT(&p2e->bblocks, bbelem);
	ip=bbb->first;

	if (ip->type != IP_PROLOG)
		return;
	plg = (struct interpass_prolog *)ip;

	name=plg->ipp_name;
	
	filenamesize=strlen(name)+1+strlen(type)+strlen(ext)+1;
	filename=tmpalloc(filenamesize);
	snprintf(filename,filenamesize,"%s-%s%s",name,type,ext);
	
	flowdiagramfile=fopen(filename,"w");
	
	fprintf(flowdiagramfile,"digraph {\n");
	fprintf(flowdiagramfile,"rankdir=LR\n");
	
	DLIST_FOREACH(bbb, &p2e->bblocks, bbelem) {
		ip=bbb->first;
		
		fprintf(flowdiagramfile,"bb%p [shape=record ",bbb);
		
		if (ip->type==IP_PROLOG)
			fprintf(flowdiagramfile,"root ");

		fprintf(flowdiagramfile,"label=\"");
		
		SLIST_FOREACH(phi,&bbb->phi,phielem) {
			fprintf(flowdiagramfile,"Phi %d|",phi->tmpregno);
		}		
		
		
		while (1) {
			switch (ip->type) {
				case IP_NODE: 
					flownodeprint(ip->ip_node,flowdiagramfile);
					break;
					
				case IP_DEFLAB: 
					fprintf(flowdiagramfile,"Label: %d", ip->ip_lbl);
					break;
					
				case IP_PROLOG:
					plg = (struct interpass_prolog *)ip;
	
					fprintf(flowdiagramfile,"%s %s",plg->ipp_name,type);
					break;
			}
			
			fprintf(flowdiagramfile,"|");
			fprintf(flowdiagramfile,"|");
			
			if (ip==bbb->last)
				break;
			
			ip = DLIST_NEXT(ip, qelem);
		}
		fprintf(flowdiagramfile,"\"]\n");
		
		FORCH(cn, bbb->ch) {
			char *color="black";
			struct interpass *pip=bbb->last;

			if (pip->type == IP_NODE && pip->ip_node->n_op == CBRANCH) {
				int label = (int)pip->ip_node->n_right->n_lval;
				
				if ((*cn)->bblock==p2e->labinfo.arr[label - p2e->ipp->ip_lblnum])
					color="red";
			}
			
			fprintf(flowdiagramfile,"bb%p -> bb%p [color=%s]\n", bbb,(*cn)->bblock,color);
		}
	}
	
	fprintf(flowdiagramfile,"}\n");
	fclose(flowdiagramfile);
}

#endif

/* walk all the programm */
void WalkAll(struct p2env* p2e, void (*f) (NODE*, void*), void* arg, int type)
{
	struct interpass *ipole = &p2e->ipole;
	struct interpass *ip ;
	if (0 != type) {
		DLIST_FOREACH(ip, ipole, qelem) {
			if (ip->type == IP_NODE && ip->ip_node->n_op == type)
				walkf(ip->ip_node, f, arg) ;
		}
	} else {
		DLIST_FOREACH(ip, ipole, qelem) {
			if (ip->type == IP_NODE)
				walkf(ip->ip_node, f, arg) ;
		}
	}
}
#if 0
static int is_goto_label(struct interpass*g, struct interpass* l)
{
	if (!g || !l)
		return 0 ;
	if (g->type != IP_NODE) return 0 ;
	if (l->type != IP_DEFLAB) return 0 ;
	if (g->ip_node->n_op != GOTO) return 0 ;
	if (g->ip_node->n_left->n_lval != l->ip_lbl) return 0 ;
	return 1 ;
}
#endif

/*
 * iterate over the basic blocks. 
 * In case a block has only one successor and that one has only one pred, and the link is a goto:
 * place the second one immediately behind the first one (in terms of nodes, means cut&resplice). 
 * This should take care of a lot of jumps.
 * one problem: Cannot move the first or last basic block (prolog/epilog). This is taken care of by
 * moving block #1 to #2, not #2 to #1
 * More complex (on the back cooker;) : first coalesc the inner loops (L1 cache), then go from inside out.
 */

static unsigned long count_blocks(struct p2env* p2e)
{
	struct basicblock* bb ;
	unsigned long count = 0 ;
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		++count ;
	}
	return count ;
}

struct block_map {
	struct basicblock* block ;
	unsigned long index ;
	unsigned long thread ;
} ;

static unsigned long map_blocks(struct p2env* p2e, struct block_map* map, unsigned long count)
{
	struct basicblock* bb ;
	unsigned long indx = 0 ;
	int ignore = 2 ;
	unsigned long thread ;
	unsigned long changes ;

	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		map[indx].block = bb ;
		map[indx].index = indx ;

		/* ignore the first 2 labels, maybe up to 3 BBs */
		if (ignore) {
			if (bb->first->type == IP_DEFLAB) 
				--ignore;

			map[indx].thread = 1 ;	/* these are "fixed" */
		} else
			map[indx].thread = 0 ;

		indx++ ;
	}

	thread = 1 ;
	do {
		changes = 0 ;
		
		for (indx=0; indx < count; indx++) {
			/* find block without trace */
			if (map[indx].thread == 0) {
				/* do new thread */
				struct cfgnode **cn ;
				struct basicblock *block2 = 0;
				unsigned long i ;
				unsigned long added ;
				
				BDEBUG (("new thread %ld at block %ld\n", thread, indx)) ;

				bb = map[indx].block ;
				do {
					added = 0 ;

					for (i=0; i < count; i++) {
						if (map[i].block == bb && map[i].thread == 0) {
							map[i].thread = thread ;

							BDEBUG(("adding block %ld to trace %ld\n", i, thread)) ;

							changes ++ ;
							added++ ;

							/* 
							 * pick one from followers. For now (simple), pick the 
							 * one who is directly following us. If none of that exists,
							 * this code picks the last one.
							 */

							FORCH(cn, bb->ch) {
								block2=(*cn)->bblock ;
#if 1
								if (i+1 < count && map[i+1].block == block2)
									break ; /* pick that one */
#else
								if (block2) break ;
#endif
							}

							if (block2)
								bb = block2 ;
						}
					}
				} while (added) ;
				thread++ ;
			}
		}
	} while (changes);

	/* Last block is also a thread on it's own, and the highest one. */
/*
	thread++ ;
	map[count-1].thread = thread ;
*/
	if (b2debug) {
		printf("Threads\n");
		for (indx=0; indx < count; indx++) {
			printf("Block #%ld (lbl %d) Thread %ld\n", indx, map[indx].block->first->ip_lbl, map[indx].thread);
		}
	}
	return thread ;
}


void TraceSchedule(struct p2env* p2e)
{
	struct block_map* map ;
	unsigned long block_count = count_blocks(p2e);
	unsigned long i ;
	unsigned long threads;
	struct interpass *front, *back ;

	map = tmpalloc(block_count * sizeof(struct block_map));

	threads = map_blocks(p2e, map, block_count) ;

	back = map[0].block->last ;
	for (i=1; i < block_count; i++) {
		/* collect the trace */
		unsigned long j ;
		unsigned long thread = map[i].thread ;
		if (thread) {
			BDEBUG(("Thread %ld\n", thread)) ;
			for (j=i; j < block_count; j++) {
				if (map[j].thread == thread) {
					front = map[j].block->first ;

					BDEBUG(("Trace %ld, old BB %ld, next BB %ld\t",
						thread, i, j)) ;
					BDEBUG(("Label %d\n", front->ip_lbl)) ;
					DLIST_NEXT(back, qelem) = front ;
					DLIST_PREV(front, qelem) = back ;
					map[j].thread = 0 ;	/* done */
					back = map[j].block->last ;
					DLIST_NEXT(back, qelem) = 0 ;
				}
			}
		}
	}
	DLIST_NEXT(back, qelem) = &(p2e->ipole) ;
	DLIST_PREV(&p2e->ipole, qelem) = back ;
}

static void add_labels(struct p2env* p2e)
{
	struct interpass *ipole = &p2e->ipole ;
	struct interpass *ip ;

	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type == IP_NODE && ip->ip_node->n_op == CBRANCH) {
			struct interpass *n = DLIST_NEXT(ip, qelem);
			if (n && n->type != IP_DEFLAB) {
				struct interpass* lab;
				int newlabel=getlab2() ;

				BDEBUG(("add_label L%d\n", newlabel));

				lab = tmpalloc(sizeof(struct interpass));
				lab->type = IP_DEFLAB;
				/* Line number?? ip->lineno; */
				lab->ip_lbl = newlabel;
				DLIST_INSERT_AFTER(ip, lab, qelem);
			}
		}
	}
}

/* node map */
#ifdef ENABLE_NEW
struct node_map {
	NODE* node ;		/* the node */
	unsigned int node_num ; /* node is equal to that one */
	unsigned int var_num ;	/* node computes this variable */
} ;

static unsigned long nodes_counter ;
static void node_map_count_walker(NODE* n, void* x)
{
	nodes_counter ++ ;
}

static void do_cse(struct p2env* p2e)
{
	nodes_counter = 0 ;
	WalkAll(p2e, node_map_count_walker, 0, 0) ;
	BDEBUG(("Found %ld nodes\n", nodes_counter)) ;
}
#endif

#define BITALLOC(ptr,all,sz) { \
	int sz__s = BIT2BYTE(sz); ptr = all(sz__s); memset(ptr, 0, sz__s); }
#define VALIDREG(p)	(p->n_op == REG && TESTBIT(validregs, regno(p)))
#define XCHECK(x) if (x < 0 || x >= xbits) printf("x out of range %d\n", x)
#define RUP(x) (((x)+NUMBITS-1)/NUMBITS)
#define SETCOPY(t,f,i,n) for (i = 0; i < RUP(n); i++) t[i] = f[i]
#define SETSET(t,f,i,n) for (i = 0; i < RUP(n); i++) t[i] |= f[i]
#define SETCLEAR(t,f,i,n) for (i = 0; i < RUP(n); i++) t[i] &= ~f[i]
#define SETCMP(v,t,f,i,n) for (i = 0; i < RUP(n); i++) \
	if (t[i] != f[i]) v = 1

static int xxx, xbits;
/*
 * Set/clear long term liveness for regs and temps.
 */
static void
unionize(NODE *p, struct basicblock *bb, int suboff)
{
	int o, ty;

	if ((o = p->n_op) == TEMP || VALIDREG(p)) {
		int b = regno(p);
		if (o == TEMP)
			b = b - suboff + MAXREGS;
XCHECK(b);
		BITSET(bb->gen, b);
	}
	if (asgop(o)) {
		if (p->n_left->n_op == TEMP || VALIDREG(p)) {
			int b = regno(p->n_left);
			if (p->n_left->n_op == TEMP)
				b = b - suboff + MAXREGS;
XCHECK(b);
			BITCLEAR(bb->gen, b);
			BITSET(bb->killed, b);
			unionize(p->n_right, bb, suboff);
			return;
		}
	}
	ty = optype(o);
	if (ty != LTYPE)
		unionize(p->n_left, bb, suboff);
	if (ty == BITYPE)
		unionize(p->n_right, bb, suboff);
}

/*
 * Found an extended assembler node, so growel out gen/killed nodes.
 */
static void
xasmionize(NODE *p, void *arg)
{
	struct basicblock *bb = arg;
	int cw, b;

	if (p->n_op == ICON && p->n_type == STRTY)
		return; /* dummy end marker */

	cw = xasmcode(p->n_name);
	if (XASMVAL(cw) == 'n' || XASMVAL(cw) == 'm')
		return; /* no flow analysis */
	p = p->n_left;
 
	if (XASMVAL(cw) == 'g' && p->n_op != TEMP && p->n_op != REG)
		return; /* no flow analysis */

	b = regno(p);
#if 0
	if (XASMVAL(cw) == 'r' && p->n_op == TEMP)
		addnotspill(b);
#endif
#define MKTOFF(r)	((r) - xxx)
	if (XASMISOUT(cw)) {
		if (p->n_op == TEMP) {
			BITCLEAR(bb->gen, MKTOFF(b));
			BITSET(bb->killed, MKTOFF(b));
		} else if (p->n_op == REG) {
			BITCLEAR(bb->gen, b);
			BITSET(bb->killed, b);	 
		} else
			uerror("bad xasm node type %d", p->n_op);
	}
	if (XASMISINP(cw)) {
		if (p->n_op == TEMP) {
			BITSET(bb->gen, MKTOFF(b));
		} else if (p->n_op == REG) {
			BITSET(bb->gen, b);
		} else if (optype(p->n_op) != LTYPE) {
			if (XASMVAL(cw) == 'r')
				uerror("couldn't find available register");
			else
				uerror("bad xasm node type2");
		}
	}
}

/*
 * Do variable liveness analysis.  Only analyze the long-lived
 * variables, and save the live-on-exit temporaries in a bit-field
 * at the end of each basic block. This bit-field is later used
 * when doing short-range liveness analysis in Build().
 */
void
liveanal(struct p2env *p2e)
{
	struct basicblock *bb;
	struct interpass *ip;
	bittype *saved;
	int i, mintemp, again;

	xbits = p2e->epp->ip_tmpnum - p2e->ipp->ip_tmpnum + MAXREGS;
	mintemp = p2e->ipp->ip_tmpnum;

	/* Just fetch space for the temporaries from heap */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		BITALLOC(bb->gen,tmpalloc,xbits);
		BITALLOC(bb->killed,tmpalloc,xbits);
		BITALLOC(bb->in,tmpalloc,xbits);
		BITALLOC(bb->out,tmpalloc,xbits);
	}
	BITALLOC(saved,tmpalloc,xbits);

	xxx = mintemp;
	/*
	 * generate the gen-killed sets for all basic blocks.
	 */
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		for (ip = bb->last; ; ip = DLIST_PREV(ip, qelem)) {
			/* gen/killed is 'p', this node is 'n' */
			if (ip->type == IP_NODE) {
				if (ip->ip_node->n_op == XASM)
					flist(ip->ip_node->n_left,
					    xasmionize, bb);
				else
					unionize(ip->ip_node, bb, mintemp);
			}
			if (ip == bb->first)
				break;
		}
		memcpy(bb->in, bb->gen, BIT2BYTE(xbits));
#ifdef PCC_DEBUG
#define PRTRG(x) printf("%d ", i < MAXREGS ? i : i + p2e->ipp->ip_tmpnum-MAXREGS)
		if (b2debug > 1) {
			printf("basic block %d\ngen: ", bb->bbnum);
			for (i = 0; i < xbits; i++)
				if (TESTBIT(bb->gen, i))
					PRTRG(i);
			printf("\nkilled: ");
			for (i = 0; i < xbits; i++)
				if (TESTBIT(bb->killed, i))
					PRTRG(i);
			printf("\n");
		}
#endif
	}
	/* do liveness analysis on basic block level */
	do {
		struct cfgnode **cn;
		int j;

		again = 0;
		/* XXX - loop should be in reversed execution-order */
		DLIST_FOREACH_REVERSE(bb, &p2e->bblocks, bbelem) {
			SETCOPY(saved, bb->out, j, xbits);
			FORCH(cn, bb->ch) {
				SETSET(bb->out, cn[0]->bblock->in, j, xbits);
			}
			SETCMP(again, saved, bb->out, j, xbits);
			SETCOPY(saved, bb->in, j, xbits);
			SETCOPY(bb->in, bb->out, j, xbits);
			SETCLEAR(bb->in, bb->killed, j, xbits);
			SETSET(bb->in, bb->gen, j, xbits);
			SETCMP(again, saved, bb->in, j, xbits);
		}
	} while (again);

#ifdef PCC_DEBUG
	DLIST_FOREACH(bb, &p2e->bblocks, bbelem) {
		if (b2debug) {
			printf("all basic block %d\nin: ", bb->bbnum);
			for (i = 0; i < xbits; i++)
				if (TESTBIT(bb->in, i))
					PRTRG(i);
			printf("\nout: ");
			for (i = 0; i < xbits; i++)
				if (TESTBIT(bb->out, i))
					PRTRG(i);
			printf("\n");
		}
	}
#endif
}
