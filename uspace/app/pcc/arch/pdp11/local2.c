/*	$Id: local2.c,v 1.8 2009/07/29 12:34:19 ragge Exp $	*/
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

# include "pass2.h"
# include <ctype.h>
# include <string.h>

static int spcoff;
static int argsiz(NODE *p);

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

void
prologue(struct interpass_prolog *ipp)
{
	int addto;

#ifdef LANG_F77
	if (ipp->ipp_vis)
		printf("	.globl %s\n", ipp->ipp_name);
	printf("%s:\n", ipp->ipp_name);
#endif
	printf("jsr	r5,csv\n");
	addto = p2maxautooff;
	if (addto >= AUTOINIT/SZCHAR)
		addto -= AUTOINIT/SZCHAR;
	if (addto & 1)
		addto++;
	if (addto == 2)
		printf("tst	-(sp)\n");
	else if (addto == 4)
		printf("cmp	-(sp),-(sp)\n");
	else if (addto > 4)
		printf("sub	$%o,sp\n", addto);
	spcoff = 0;
}

void
eoftn(struct interpass_prolog *ipp)
{
	if (spcoff)
		comperr("spcoff == %d", spcoff);
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */
	printf("jmp	cret\n");
}

/*
 * add/sub/...
 *
 * Param given:
 */
void
hopcode(int f, int o)
{
	char *str;

	switch (o) {
	case PLUS:
		str = "add";
		break;
	case MINUS:
		str = "sub";
		break;
	case AND:
		str = "and";
		break;
	case OR:
		str = "or";
		break;
	case ER:
		str = "xor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s%c", str, f);
}

/*
 * Return type size in bytes.  Used by R2REGS, arg 2 to offset().
 */
int
tlen(p) NODE *p;
{
	switch(p->n_type) {
		case CHAR:
		case UCHAR:
			return(1);

		case SHORT:
		case USHORT:
			return(SZSHORT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		case INT:
		case UNSIGNED:
		case LONG:
		case ULONG:
			return(SZINT/SZCHAR);

		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}

/*
 * Emit code to compare two long numbers.
 */
static void
twolcomp(NODE *p)
{
	int o = p->n_op;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	if (o >= ULE)
		o -= (ULE-LE);
	switch (o) {
	case NE:
		cb1 = 0;
		cb2 = NE;
		break;
	case EQ:
		cb1 = NE;
		cb2 = 0;
		break;
	case LE:
	case LT:
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		cb1 = LT;
		cb2 = GT;
		break;

	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 2, cb2 += 2;
	expand(p, 0, "cmp	AR,AL\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
        expand(p, 0, "cmp	UR,UL\n");
        cbgen(p->n_op, e);
        deflab(s);
}


/*
 * Generate compare code for long instructions when right node is 0.
 */
static void
lcomp(NODE *p)
{
	switch (p->n_op) {
	case EQ:
		expand(p, FORCC, "tst	AL\n");
		printf("jne	1f\n");
		expand(p, FORCC, "tst	UL\n");
		cbgen(EQ, p->n_label);
		printf("1:\n");
		break;
	case NE:
		expand(p, FORCC, "tst	AL\n");
		cbgen(NE, p->n_label);
		expand(p, FORCC, "tst	UL\n");
		cbgen(NE, p->n_label);
		break;
	case GE:
		expand(p, FORCC, "tst	AL\n");
		cbgen(GE, p->n_label);
		break;
	default:
		comperr("lcomp %p", p);
	} 
}

void
zzzcode(NODE *p, int c)
{
	switch (c) {
	case 'A': /* print out - if not first arg */
		if (spcoff || (p->n_type == FLOAT || p->n_type == DOUBLE))
			printf("-");
		spcoff += argsiz(p);
		break;

	case 'B': /* arg is pointer to block */
		expand(p->n_left, FOREFF, "mov	AL,ZA(sp)\n");
		expand(p->n_left, FOREFF, "sub	CR,(sp)\n");
		break;
		
	case 'C': /* subtract stack after call */
		spcoff -= p->n_qual;
		if (spcoff == 0 && !(p->n_flags & NLOCAL1))
			p->n_qual -= 2;
		if (p->n_qual == 2)
			printf("tst	(sp)+\n");
		else if (p->n_qual == 4)
			printf("cmp	(sp)+,(sp)+\n");
		else if (p->n_qual > 2)
			printf("add	$%o,sp\n", (int)p->n_qual);
		break;

	case 'D': /* long comparisions */
		lcomp(p);
		break;

	case 'E': /* long move */
		rmove(p->n_right->n_reg, p->n_left->n_reg, p->n_type);
		break;

	case 'F': /* long comparision */
		twolcomp(p);
		break;

	case 'G': /* printout a subnode for post-inc */
		adrput(stdout, p->n_left->n_left);
		break;

	case 'H': /* arg with post-inc */
		expand(p->n_left->n_left, FOREFF, "mov	AL,ZA(sp)\n");
		expand(p->n_left->n_left, FOREFF, "inc	AL\n");
		break;

	case 'Q': /* struct assignment, no rv */
		printf("mov	$%o,", p->n_stsize/2);
		expand(p, INAREG, "A1\n");
		printf("1:\n");
		expand(p, INAREG, "mov	(AR)+,(AL)+\n");
		expand(p, INAREG, "dec	A1\n");
		printf("jne	1b\n");
		break;

	case 'R': /* struct assignment with rv */
		printf("mov	$%o,", p->n_stsize/2);
		expand(p, INAREG, "A1\n");
		expand(p, INAREG, "mov	AR,A2\n");
		printf("1:\n");
		expand(p, INAREG, "mov	(A2)+,(AL)+\n");
		expand(p, INAREG, "dec	A1\n");
		printf("jne	1b\n");
		break;

	case '1': /* lower part of double regs */
		p = getlr(p, '1');
		printf("r%c", rnames[p->n_rval][1]);
		break;

	default:
		comperr("zzzcode %c", c);
	}
}

/*ARGSUSED*/
int
rewfld(NODE *p)
{
	return(1);
}

int canaddr(NODE *);
int
canaddr(NODE *p)
{
	int o = p->n_op;

	if (o==NAME || o==REG || o==ICON || o==OREG ||
	    (o==UMUL && shumul(p->n_left, STARNM|SOREG)))
		return(1);
	return(0);
}

/*
 * Does the bitfield shape match?
 */
int
flshape(NODE *p)
{
	int o = p->n_op;

	if (o == OREG || o == REG || o == NAME)
		return SRDIR; /* Direct match */
	if (o == UMUL && shumul(p->n_left, SOREG))
		return SROREG; /* Convert into oreg */
	return SRREG; /* put it into a register */
}

/* INTEMP shapes must not contain any temporary registers */
/* XXX should this go away now? */
int
shtemp(NODE *p)
{
	return 0;
#if 0
	int r;

	if (p->n_op == STARG )
		p = p->n_left;

	switch (p->n_op) {
	case REG:
		return (!istreg(p->n_rval));

	case OREG:
		r = p->n_rval;
		if (R2TEST(r)) {
			if (istreg(R2UPK1(r)))
				return(0);
			r = R2UPK2(r);
		}
		return (!istreg(r));

	case UMUL:
		p = p->n_left;
		return (p->n_op != UMUL && shtemp(p));
	}

	if (optype(p->n_op) != LTYPE)
		return(0);
	return(1);
#endif
}

static void
negcon(FILE *fp, int con)
{
	if (con < 0)
		fprintf(fp, "-"), con = -con;
	fprintf(fp, "%o", con & 0177777);
}

void
adrcon(CONSZ val)
{
	printf("$" CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	int val = p->n_lval;

	switch (p->n_op) {
	case ICON:
		printf("$");
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%o", val & 0177777);
		} else if (p->n_type == LONG || p->n_type == ULONG)
			negcon(fp, val >> 16);
		else
			negcon(fp, val);
		return;

	default:
		comperr("illegal conput, p %p", p);
	}
}

/*ARGSUSED*/
void
insput(NODE *p)
{
	comperr("insput");
}

/*
 * Write out the upper address, like the upper register of a 2-register
 * reference, or the next memory location.
 */
void
upput(NODE *p, int size)
{
	size /= SZINT;
	switch (p->n_op) {
	case NAME:
	case OREG:
		p->n_lval += size;
		adrput(stdout, p);
		p->n_lval -= size;
		break;
	case REG:
		printf("r%c", rnames[p->n_rval][2]);
		break;
	case ICON:
		/* On PDP11 upper value is low 16 bits */
		printf("$");
		negcon(stdout, p->n_lval & 0177777);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

/*
 * output an address, with offsets, from p
 */
void
adrput(FILE *io, NODE *p)
{
	int r;

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {
	case NAME:
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (p->n_lval != 0)
				fprintf(io, "+%o", (int)(p->n_lval&0177777));
		} else
			negcon(io, p->n_lval);
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (R2TEST(r) && R2UPK3(r) == 0)
			printf("*");
		if (p->n_lval)
			negcon(io, p->n_lval);
		if (R2TEST(r)) {
			fprintf(io, "(%s)", rnames[R2UPK1(r)]);
			if (R2UPK3(r) == 1)
				fprintf(io, "+");
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
		/* addressable value of the constant */
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case LONG:
		case ULONG:
			fprintf(io, "r%c", rnames[p->n_rval][1]);
			break;
		default:
			fprintf(io, "%s", rnames[p->n_rval]);
		}
		return;

	case UMUL:
		if (tshape(p, STARNM)) {
			printf("*");
			adrput(io, p->n_left);
			break;
		}
		/* FALLTHROUGH */
	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

static char *
ccbranches[] = {
	"jeq",		/* jumpe */
	"jne",		/* jumpn */
	"jle",		/* jumple */
	"jlt",		/* jumpl */
	"jge",		/* jumpge */
	"jgt",		/* jumpg */
	"jlos",		/* jumple (jlequ) */
	"jlo",		/* jumpl (jlssu) */
	"jhis",		/* jumpge (jgequ) */
	"jhi",		/* jumpg (jgtru) */
};


/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	if (o < EQ || o > UGT)
		comperr("bad conditional branch: %s", opst[o]);
	printf("%s	" LABFMT "\n", ccbranches[o-EQ], lab);
}

#define	IS1CON(p) ((p)->n_op == ICON && (p)->n_lval == 1)

/*
 * Move postfix operators to the next statement, unless they are 
 * within a function call or a branch.
 */
static void
cvtree(NODE *p, struct interpass *ip2)
{
	struct interpass *ip;
	NODE *q;

	if (callop(p->n_op) || p->n_op == CBRANCH)
		return;

	if ((p->n_op == PLUS || p->n_op == MINUS) &&
	    IS1CON(p->n_right) && (q = p->n_left)->n_op == ASSIGN &&
	    treecmp(q->n_left, q->n_right->n_left) &&
	    IS1CON(q->n_right->n_right)) {
		if ((p->n_op == PLUS && q->n_right->n_op == MINUS) ||
		    (p->n_op == MINUS && q->n_right->n_op == PLUS)) {
			nfree(p->n_right);
			*p = *q->n_left;
			if (optype(p->n_op) != LTYPE)
				p->n_left = tcopy(p->n_left);
			ip = ipnode(q);
			DLIST_INSERT_AFTER(ip2, ip, qelem);
			return;
		}
	}
	if (optype(p->n_op) == BITYPE)
		cvtree(p->n_right, ip2);
	if (optype(p->n_op) != LTYPE)
		cvtree(p->n_left, ip2);
}

/*
 * Convert AND to BIC.
 */
static void
fixops(NODE *p, void *arg)
{
	static int fltwritten;

	if (!fltwritten && (p->n_type == FLOAT || p->n_type == DOUBLE)) {
		printf(".globl	fltused\n");
		fltwritten = 1;
	}
	switch (p->n_op) {
	case AND:
		if (p->n_right->n_op == ICON) {
			p->n_right->n_lval = ((~p->n_right->n_lval) & 0177777);
		} else if (p->n_right->n_op == COMPL) {
			NODE *q = p->n_right->n_left;
			nfree(p->n_right);
			p->n_right = q;
		} else
			p->n_right = mkunode(COMPL, p->n_right, 0, p->n_type);
		break;
	case RS:
		p->n_right = mkunode(UMINUS, p->n_right, 0, p->n_right->n_type);
		p->n_op = LS;
		break;
	case EQ:
	case NE: /* Hack not to clear bits if FORCC */
		if (p->n_left->n_op == AND)
			fixops(p->n_left, 0); /* Convert an extra time */
		break;
	}
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader before\n");
		printip(ipole);
	}
#endif
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		walkf(ip->ip_node, fixops, 0);
		canon(ip->ip_node); /* call it early */
	}
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader middle\n");
		printip(ipole);
	}
#endif
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type == IP_NODE)
			cvtree(ip->ip_node, ip);
	}
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader after\n");
		printip(ipole);
	}
#endif
}

/*
 * Remove SCONVs where the left node is an OREG with a smaller type.
 */
static void
delsconv(NODE *p, void *arg)
{
#if 0
	NODE *l;

	if (p->n_op != SCONV || (l = p->n_left)->n_op != OREG)
		return;
	if (DEUNSIGN(p->n_type) == INT && DEUNSIGN(l->n_type) == LONG) {
		p->n_op = OREG;
		p->n_lval = l->n_lval; /* high word */
		p->n_rval = l->n_rval;
		nfree(l);
	}
#endif
	/* Could do this for char etc. also */
}

void
mycanon(NODE *p)
{
	walkf(p, delsconv, 0);
}

void
myoptim(struct interpass *ip)
{
}

void
rmove(int s, int d, TWORD t)
{
	if (t < LONG || t > BTMASK) {
		printf("mov%s	%s,%s\n", t < SHORT ? "b" : "",
		    rnames[s],rnames[d]); /* XXX char should be full reg? */
	} else if (t == LONG || t == ULONG) {
		/* avoid trashing double regs */
		if (d > s)
			printf("mov     r%c,r%c\nmov    r%c,r%c\n",
			    rnames[s][2],rnames[d][2],
			    rnames[s][1],rnames[d][1]);
		else
			printf("mov	r%c,r%c\nmov	r%c,r%c\n",
			    rnames[s][1],rnames[d][1],
			    rnames[s][2],rnames[d][2]);
	} else if (t == FLOAT || t == DOUBLE) {
		printf("movf	%s,%s\n", rnames[s],rnames[d]);
	} else
		comperr("bad float rmove: %d %d %x", s, d, t);

}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	switch (c) {
	case CLASSA:
		return (r[CLASSB] * 2 + r[CLASSA]) < 5;
	case CLASSB:
		if (r[CLASSB] > 1) return 0;
		if (r[CLASSB] == 1 && r[CLASSA] > 0) return 0;
		if (r[CLASSA] > 2) return 0;
		return 1;
	case CLASSC:
		return r[CLASSC] < 8;
	}
	return 0;
}

char *rnames[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "sp", "pc",
	"r01", "r12", "r23", "r34", "XXX", "XXX", "XXX", "XXX",
	"fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "XXX", "XXX",
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t < LONG || t > BTMASK)
		return CLASSA;
	if (t == LONG || t == ULONG)
		return CLASSB;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSC;
	comperr("gclass");
	return CLASSD;
}

static int
argsiz(NODE *p)  
{
	TWORD t = p->n_type;

	if (t == LONG || t == ULONG || t == FLOAT)
		return 4;
	if (t == DOUBLE)
		return 8;
	if (t == STRTY || t == UNIONTY)
		return p->n_stsize;
	return 2;
}

/*
 * Argument specialties.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	/*
	 * Calculate arg sizes.
	 * Mark first arg not to have - before it.
	 */
	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left) {
		p->n_right->n_qual = 0;
		size += argsiz(p->n_right);
	}
	p->n_qual = 0;
	size += argsiz(p);
	p = op->n_right;

	if (p->n_op == CM)
		p = p->n_right;
	if (p->n_type == FLOAT || p->n_type == DOUBLE ||
	    p->n_type == STRTY || p->n_type == UNIONTY)
		op->n_flags |= NLOCAL1;	/* Does not use stack slot */
	else
		op->n_flags &= ~NLOCAL1;
	op->n_qual = size; /* XXX */
}

static int
is1con(NODE *p)
{
	if (p->n_op == ICON && p->n_lval == 1)
		return 1;
	return 0;
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	CONSZ s;

	switch (shape) {
	case SANDSCON:
		s = ~p->n_lval;
		if (s < 65536 || s > -65537)
			return SRDIR;
		break;
	case SINCB: /* Check if subject for post-inc */
		if (p->n_op == ASSIGN && p->n_right->n_op == PLUS &&
		    treecmp(p->n_left, p->n_right->n_left) &&
		    is1con(p->n_right->n_right))
			return SRDIR;
		break;
	case SARGSUB:
		if (p->n_op == MINUS && p->n_right->n_op == ICON &&
		    p->n_left->n_op == REG)
			return SRDIR;
		break;
	case SARGINC:
		if (p->n_op == MINUS && is1con(p->n_right))
			return special(p->n_left, SINCB);
		break;
	}
	return SRNOPE;
}

/*
 * Target-dependent command-line options.
 */
void
mflags(char *str)
{
}

/*
 * Do something target-dependent for xasm arguments.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	return 0;
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	return 0;
}

