/*	$Id: local2.c,v 1.41 2011/02/18 16:52:37 ragge Exp $	*/
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
# include <ctype.h>
# include <string.h>

static int stkpos;

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

static int regoff[MAXREGS];
static TWORD ftype;
char *rbyte[], *rshort[], *rlong[];

/*
 * Print out the prolog assembler.
 * addto and regoff are already calculated.
 */
static void
prtprolog(struct interpass_prolog *ipp, int addto)
{
	int i;

	/* XXX should look if there is any need to emit this */
	printf("\tpushq %%rbp\n");
	printf("\tmovq %%rsp,%%rbp\n");
	addto = (addto+15) & ~15; /* 16-byte aligned */
	if (addto)
		printf("\tsubq $%d,%%rsp\n", addto);

	/* save permanent registers */
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			fprintf(stdout, "\tmovq %s,-%d(%s)\n",
			    rnames[i], regoff[i], rnames[FPREG]);
}

/*
 * calculate stack size and offsets
 */
static int
offcalc(struct interpass_prolog *ipp)
{
	int i, addto;

	addto = p2maxautooff;
	if (addto >= AUTOINIT/SZCHAR)
		addto -= AUTOINIT/SZCHAR;
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i)) {
			addto += SZLONG/SZCHAR;
			regoff[i] = addto;
		}
	return addto;
}

void
prologue(struct interpass_prolog *ipp)
{
	int addto;

	ftype = ipp->ipp_type;

#ifdef LANG_F77
	if (ipp->ipp_vis)
		printf("	.globl %s\n", ipp->ipp_name);
	printf("	.align 16\n");
	printf("%s:\n", ipp->ipp_name);
#endif
	/*
	 * We here know what register to save and how much to 
	 * add to the stack.
	 */
	addto = offcalc(ipp);
	prtprolog(ipp, addto);
}

void
eoftn(struct interpass_prolog *ipp)
{
	int i;

	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	/* return from function code */
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			fprintf(stdout, "	movq -%d(%s),%s\n",
			    regoff[i], rnames[FPREG], rnames[i]);

	/* struct return needs special treatment */
	if (ftype == STRTY || ftype == UNIONTY) {
		printf("	movl 8(%%ebp),%%eax\n");
		printf("	leave\n");
		printf("	ret $%d\n", 4);
	} else {
		printf("	leave\n");
		printf("	ret\n");
	}
#ifndef MACHOABI
	printf("\t.size %s,.-%s\n", ipp->ipp_name, ipp->ipp_name);
#endif
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
tlen(NODE *p)
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
			return(SZINT/SZCHAR);

		case LONG:
		case ULONG:
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
 * Compare two floating point numbers.
 */
static void
fcomp(NODE *p)	
{

	if (p->n_left->n_op != REG)
		comperr("bad compare %p\n", p);
	if ((p->n_su & DORIGHT) == 0)
		expand(p, 0, "	fxch\n");
	expand(p, 0, "	fucomip %st(1),%st\n");	/* emit compare insn  */
	expand(p, 0, "	fstp %st(0)\n");	/* pop fromstack */
	zzzcode(p, 'U');
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	CONSZ val;

	if (p->n_op == ASSIGN)
		p = p->n_left;
	switch (**cp) {
	case 'S':
		printf("%d", UPKFSZ(p->n_rval));
		break;
	case 'H':
		printf("%d", UPKFOFF(p->n_rval));
		break;
	case 'M':
	case 'N':
		val = (((((CONSZ)1 << (UPKFSZ(p->n_rval)-1))-1)<<1)|1);
		val <<= UPKFOFF(p->n_rval);
		if (p->n_type > UNSIGNED)
			printf("0x%llx", (**cp == 'M' ? val : ~val));
		else
			printf("0x%llx", (**cp == 'M' ? val : ~val)&0xffffffff);
		break;
	default:
		comperr("fldexpand");
	}
	return 1;
}

static void
bfext(NODE *p)
{
	int ch = 0, sz = 0;

	if (ISUNSIGNED(p->n_right->n_type))
		return;
	switch (p->n_right->n_type) {
	case CHAR:
		ch = 'b';
		sz = 8;
		break;
	case SHORT:
		ch = 'w';
		sz = 16;
		break;
	case INT:
		ch = 'l';
		sz = 32;
		break;
	case LONG:
		ch = 'q';
		sz = 64;
		break;
	default:
		comperr("bfext");
	}

	sz -= UPKFSZ(p->n_left->n_rval);
	printf("\tshl%c $%d,", ch, sz);
	adrput(stdout, getlr(p, 'D'));
	printf("\n\tsar%c $%d,", ch, sz);
	adrput(stdout, getlr(p, 'D'));
	printf("\n");
}

static void
stasg(NODE *p)
{
	expand(p, INAREG, "	leaq AL,%rdi\n");
	if (p->n_stsize >= 8)
		printf("\tmovl $%d,%%ecx\n\trep movsq\n", p->n_stsize >> 3);
	if (p->n_stsize & 3)
		printf("\tmovsl\n");
	if (p->n_stsize & 2)
		printf("\tmovsw\n");
	if (p->n_stsize & 1)
		printf("\tmovsb\n");
}

#define	E(x)	expand(p, 0, x)
/*
 * Generate code to convert an unsigned long to xmm float/double.
 */
static void
ultofd(NODE *p)
{

	E("	movq AL,A1\n");
	E("	testq A1,A1\n");
	E("	js 2f\n");
	E("	cvtsi2sZfq A1,A3\n");
	E("	jmp 3f\n");
	E("2:\n");
	E("	movq A1,A2\n");
	E("	shrq A2\n");
	E("	andq $1,A1\n");
	E("	orq A1,A2\n");
	E("	cvtsi2sZfq A2,A3\n");
	E("	addsZf A3,A3\n");
	E("3:\n");
}

/*
 * Generate code to convert an x87 long double to an unsigned long.
 * This is ugly :-/
 */
static void
ldtoul(NODE *p)
{
	int r;

	r = getlr(p, '1')->n_rval;

	E("	subq $16,%rsp\n");
	E("	movl $0x5f000000,(%rsp)\n"); /* More than long can have */
	E("	flds (%rsp)\n");
	if (p->n_left->n_op == REG) {
		E("	fxch\n");
	} else
		E("	fldt AL\n");
	E("	fucomi %st(1), %st\n");
	E("	jae 2f\n");

	E("	fstp %st(1)\n");	 /* Pop huge val from stack */
	E("	fnstcw (%rsp)\n");	 /* store cw */
	E("	movw $0x0f3f,4(%rsp)\n");/* round towards 0 */
	E("	fldcw 4(%rsp)\n");	 /* new cw */
	E("	fistpll 8(%rsp)\n");	 /* save val */
	E("	fldcw (%rsp)\n");	 /* fetch old cw */
	E("	movq 8(%rsp),A1\n");

	E("	jmp 3f\n");

	E("2:\n");

	E("	fsubp %st, %st(1)\n");
	E("	fnstcw (%rsp)\n");	
	E("	movw $0x0f3f,4(%rsp)\n");
	E("	fldcw 4(%rsp)\n");
	E("	fistpll 8(%rsp)\n");
	E("	fldcw (%rsp)\n");
	E("	movabsq $0x8000000000000000,A1\n");
	E("	xorq 8(%rsp),A1\n");

	E("3:	addq $16,%rsp\n");
}

/*
 * Generate code to convert an SSE float/double to an unsigned long.
 */     
static void     
fdtoul(NODE *p) 
{
	if (p->n_left->n_type == FLOAT)
		E("	movabsq $0x5f000000,A1\n");
	else
		E("	movabsq $0x43e0000000000000,A1\n");
	E("	movd A1,A3\n");
	E("	ucomisZg A3,AL\n");
	E("	jae 2f\n");
	E("	cvttsZg2siq AL,A1\n");
	E("	jmp 3f\n");
	E("2:\n");
	E("	subsZg A3,AL\n");
	E("	cvttsZg2siq AL,A1\n");
	E("	movabsq $0x8000000000000000,A2\n");
	E("	xorq A2,A1\n");
	E("3:\n");
}
#undef E

void
zzzcode(NODE *p, int c)
{
	NODE *l;
	int pr, lr, s;
	char **rt;

	switch (c) {
	case 'A': /* swap st0 and st1 if right is evaluated second */
		if ((p->n_su & DORIGHT) == 0) {
			if (logop(p->n_op))
				printf("	fxch\n");
			else
				printf("r");
		}
		break;

	case 'B': /* ldouble to unsigned long cast */
		ldtoul(p);
		break;

	case 'b': /* float/double to unsigned long cast */
		fdtoul(p);
		break;

	case 'C':  /* remove from stack after subroutine call */
		pr = p->n_qual;
		if (p->n_op == UCALL)
			return; /* XXX remove ZC from UCALL */
		if (pr)
			printf("	addq $%d, %s\n", pr, rnames[RSP]);
		break;

	case 'E': /* Perform bitfield sign-extension */
		bfext(p);
		break;

	case 'F': /* Structure argument */
		printf("	subq $%d,%%rsp\n", p->n_stsize);
		printf("	movq %%rsp,%%rsi\n");
		stasg(p);
		break;

	case 'G': /* Floating point compare */
		fcomp(p);
		break;

	case 'j': /* convert unsigned long to f/d */
		ultofd(p);
		break;

	case 'M': /* Output sconv move, if needed */
		l = getlr(p, 'L');
		/* XXX fixneed: regnum */
		pr = DECRA(p->n_reg, 0);
		lr = DECRA(l->n_reg, 0);
		if (pr == lr)
			break;
		printf("	movb %s,%s\n", rbyte[lr], rbyte[pr]);
		l->n_rval = l->n_reg = p->n_reg; /* XXX - not pretty */
		break;

	case 'N': /* output long reg name */
		printf("%s", rlong[getlr(p, '1')->n_rval]);
		break;

	case 'P': /* Put hidden argument in rdi */
		printf("\tleaq -%d(%%rbp),%%rdi\n", stkpos);
		break;

        case 'Q': /* emit struct assign */
		stasg(p);
		break;

	case 'R': /* print opname based on right type */
	case 'L': /* print opname based on left type */
		switch (getlr(p, c)->n_type) {
		case CHAR: case UCHAR: s = 'b'; break;
		case SHORT: case USHORT: s = 'w'; break;
		case INT: case UNSIGNED: s = 'l'; break;
		default: s = 'q'; break;
		printf("%c", s);
		}
		break;

	case 'U': { /* output branch insn for ucomi */
		static char *fpcb[] = { "jz", "jnz", "jbe", "jc", "jnc", "ja" };
		if (p->n_op < EQ || p->n_op > GT)
			comperr("bad fp branch");
		if (p->n_op == NE || p->n_op == GT || p->n_op == GE)
			expand(p, 0, "	jp LC\n");
		else if (p->n_op == EQ)
			printf("\tjp 1f\n");
		printf("	%s ", fpcb[p->n_op - EQ]);
		expand(p, 0, "LC\n");
		if (p->n_op == EQ)
			printf("1:\n");
		break;
		}

	case '8': /* special reg name printout (64-bit) */
	case '1': /* special reg name printout (32-bit) */
		l = getlr(p, '1');
		rt = c == '8' ? rnames : rlong;
		printf("%s", rt[l->n_rval]);
		break;

	case 'g':
		p = p->n_left;
		/* FALLTHROUGH */
	case 'f': /* float or double */
		printf("%c", p->n_type == FLOAT ? 's' : 'd');
		break;

	case 'q': /* int or long */
		printf("%c", p->n_left->n_type == LONG ? 'q' : ' ');
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
	    (o==UMUL && shumul(p->n_left, SOREG)))
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

void
adrcon(CONSZ val)
{
	printf("$" CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	long val = p->n_lval;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%ld", val);
		} else
			fprintf(fp, "%ld", val);
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
 * XXX - not needed on amd64
 */
void
upput(NODE *p, int size)
{

	size /= SZCHAR;
	switch (p->n_op) {
	case REG:
		fprintf(stdout, "%%%s", &rnames[p->n_rval][3]);
		break;

	case NAME:
	case OREG:
		p->n_lval += size;
		adrput(stdout, p);
		p->n_lval -= size;
		break;
	case ICON:
		fprintf(stdout, "$" CONFMT, p->n_lval >> 32);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

void
adrput(FILE *io, NODE *p)
{
	int r;
	char **rc;
	/* output an address, with offsets, from p */

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {

	case NAME:
		if (p->n_name[0] != '\0') {
			if (p->n_lval != 0)
				fprintf(io, CONFMT "+", p->n_lval);
			fprintf(io, "%s(%%rip)", p->n_name);
		} else
			fprintf(io, CONFMT, p->n_lval);
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (p->n_lval)
			fprintf(io, "%lld", p->n_lval);
		if (R2TEST(r)) {
			int r1 = R2UPK1(r);
			int r2 = R2UPK2(r);
			int sh = R2UPK3(r);

			fprintf(io, "(%s,%s,%d)", 
			    r1 == MAXREGS ? "" : rnames[r1],
			    r2 == MAXREGS ? "" : rnames[r2], sh);
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
#ifdef PCC_DEBUG
		/* Sanitycheck for PIC, to catch adressable constants */
		if (kflag && p->n_name[0]) {
			static int foo;

			if (foo++ == 0) {
				printf("\nfailing...\n");
				fwalk(p, e2print, 0);
				comperr("pass2 conput");
			}
		}
#endif
		/* addressable value of the constant */
		fputc('$', io);
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case CHAR:
		case UCHAR:
			rc = rbyte;
			break;
		case SHORT:
		case USHORT:
			rc = rshort;
			break;
		case INT:
		case UNSIGNED:
			rc = rlong;
			break;
		default:
			rc = rnames;
			break;
		}
		fprintf(io, "%s", rc[p->n_rval]);
		return;

	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

static char *
ccbranches[] = {
	"je",		/* jumpe */
	"jne",		/* jumpn */
	"jle",		/* jumple */
	"jl",		/* jumpl */
	"jge",		/* jumpge */
	"jg",		/* jumpg */
	"jbe",		/* jumple (jlequ) */
	"jb",		/* jumpl (jlssu) */
	"jae",		/* jumpge (jgequ) */
	"ja",		/* jumpg (jgtru) */
};


/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	if (o < EQ || o > UGT)
		comperr("bad conditional branch: %s", opst[o]);
	printf("	%s " LABFMT "\n", ccbranches[o-EQ], lab);
}

static void
fixcalls(NODE *p, void *arg)
{
	/* Prepare for struct return by allocating bounce space on stack */
	switch (p->n_op) {
	case STCALL:
	case USTCALL:
		if (p->n_stsize+p2autooff > stkpos)
			stkpos = p->n_stsize+p2autooff;
		break;
	}
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

	stkpos = p2autooff;
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		walkf(ip->ip_node, fixcalls, 0);
	}
	if (stkpos > p2autooff)
		p2autooff = stkpos;
	if (stkpos > p2maxautooff)
		p2maxautooff = stkpos;
	if (x2debug)
		printip(ipole);
}

/*
 * Remove some PCONVs after OREGs are created.
 */
static void
pconv2(NODE *p, void *arg)
{
	NODE *q;

	if (p->n_op == PLUS) {
		if (p->n_type == (PTR|SHORT) || p->n_type == (PTR|USHORT)) {
			if (p->n_right->n_op != ICON)
				return;
			if (p->n_left->n_op != PCONV)
				return;
			if (p->n_left->n_left->n_op != OREG)
				return;
			q = p->n_left->n_left;
			nfree(p->n_left);
			p->n_left = q;
			/*
			 * This will be converted to another OREG later.
			 */
		}
	}
}

void
mycanon(NODE *p)
{
	walkf(p, pconv2, 0);
}

void
myoptim(struct interpass *ip)
{
}

void
rmove(int s, int d, TWORD t)
{

	switch (t) {
	case INT:
	case UNSIGNED:
		printf("	movl %s,%s\n", rlong[s], rlong[d]);
		break;
	case CHAR:
	case UCHAR:
		printf("	movb %s,%s\n", rbyte[s], rbyte[d]);
		break;
	case SHORT:
	case USHORT:
		printf("	movw %s,%s\n", rshort[s], rshort[d]);
		break;
	case FLOAT:
		printf("	movss %s,%s\n", rnames[s], rnames[d]);
		break;
	case DOUBLE:
		printf("	movsd %s,%s\n", rnames[s], rnames[d]);
		break;
	case LDOUBLE:
#ifdef notdef
		/* a=b()*c(); will generate this */
		/* XXX can it fail anyway? */
		comperr("bad float rmove: %d %d", s, d);
#endif
		break;
	default:
		printf("	movq %s,%s\n", rnames[s], rnames[d]);
		break;
	}
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
		return r[CLASSA] < 14;
	case CLASSB:
		return r[CLASSB] < 16;
	case CLASSC:
		return r[CLASSC] < CREGCNT;
	}
	return 0; /* XXX gcc */
}

char *rnames[] = {
	"%rax", "%rdx", "%rcx", "%rbx", "%rsi", "%rdi", "%rbp", "%rsp",
	"%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
	"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
	"%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14",
	"%xmm15",
};

/* register names for shorter sizes */
char *rbyte[] = {
	"%al", "%dl", "%cl", "%bl", "%sil", "%dil", "%bpl", "%spl",
	"%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b", 
};
char *rshort[] = {
	"%ax", "%dx", "%cx", "%bx", "%si", "%di", "%bp", "%sp",
	"%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w", 
};
char *rlong[] = {
	"%eax", "%edx", "%ecx", "%ebx", "%esi", "%edi", "%ebp", "%esp",
	"%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d", 
};


/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t == LDOUBLE)
		return CLASSC;
	if (t == FLOAT || t == DOUBLE)
		return CLASSB;
	return CLASSA;
}

static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;

	if (p->n_left->n_op == REG)
		return 0; /* not on stack */
	if (t == LDOUBLE)
		return 16;
	if (p->n_op == STASG)
		return p->n_stsize;
	return 8;
}

/*
 * Calculate argument sizes.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left)
		size += argsiz(p->n_right);
	size += argsiz(p);
	size = (size+15) & ~15;
	if (size)
		printf("	subq $%d,%s\n", size, rnames[RSP]);
	op->n_qual = size; /* XXX */
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	int o = p->n_op;

	switch (shape) {
	case SFUNCALL:
		if (o == STCALL || o == USTCALL)
			return SRREG;
		break;
	case SPCON:
		if (o != ICON || p->n_name[0] ||
		    p->n_lval < 0 || p->n_lval > 0x7fffffff)
			break;
		return SRDIR;
	case SMIXOR:
		return tshape(p, SZERO);
	case SMILWXOR:
		if (o != ICON || p->n_name[0] ||
		    p->n_lval == 0 || p->n_lval & 0xffffffff)
			break;
		return SRDIR;
	case SMIHWXOR:
		if (o != ICON || p->n_name[0] ||
		     p->n_lval == 0 || (p->n_lval >> 32) != 0)
			break;
		return SRDIR;
	case SCON32:
		if (o != ICON || p->n_name[0])
			break;
		if (p->n_lval < MIN_INT || p->n_lval > MAX_INT)
			break;
		return SRDIR;
	default:
		cerror("special: %x\n", shape);
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
	struct interpass *ip2;
	int Cmax[] = { 31, 63, 127, 0xffff, 3, 255 };
	NODE *in = 0, *ut = 0;
	TWORD t;
	char *w;
	int reg;
	int c, cw, v;

	cw = xasmcode(p->n_name);
	if (cw & (XASMASG|XASMINOUT))
		ut = p->n_left;
	if ((cw & XASMASG) == 0)
		in = p->n_left;

	c = XASMVAL(cw);
retry:	switch (c) {
	case 'D': reg = RDI; break;
	case 'S': reg = RSI; break;
	case 'A': 
	case 'a': reg = RAX; break;
	case 'b': reg = RBX; break;
	case 'c': reg = RCX; break;
	case 'd': reg = RDX; break;

	case 'x':
	case 'q':
	case 't':
	case 'u':
		p->n_name = tmpstrdup(p->n_name);
		w = strchr(p->n_name, c);
		*w = 'r'; /* now reg */
		return c == 'q' || c == 'x' ? 0 : 1;

	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
		if (p->n_left->n_op != ICON) {
			if ((c = XASMVAL1(cw)))
				goto retry;
			uerror("xasm arg not constant");
		}
		v = p->n_left->n_lval;
		if ((c == 'K' && v < -128) ||
		    (c == 'L' && v != 0xff && v != 0xffff) ||
		    (c != 'K' && v < 0) ||
		    (v > Cmax[c-'I']))
			uerror("xasm val out of range");
		p->n_name = "i";
		return 1;

	default:
		return 0;
	}
	/* If there are requested either memory or register, delete memory */
	w = p->n_name = tmpstrdup(p->n_name);
	if (*w == '=')
		w++;
	*w++ = 'r';
	*w = 0;

	t = p->n_left->n_type;

	if (t == FLOAT || t == DOUBLE) {
		p->n_label = CLASSB;
		reg += 16;
	} else if (t == LDOUBLE) {
		p->n_label = CLASSC;
		reg += 32;
	} else
		p->n_label = CLASSA;

	if (in && ut)
		in = tcopy(in);
	p->n_left = mklnode(REG, 0, reg, t);
	if (ut) {
		ip2 = ipnode(mkbinode(ASSIGN, ut, tcopy(p->n_left), t));
		DLIST_INSERT_AFTER(ip, ip2, qelem);
	}
	if (in) {
		ip2 = ipnode(mkbinode(ASSIGN, tcopy(p->n_left), in, t));
		DLIST_INSERT_BEFORE(ip, ip2, qelem);
	}

	return 1;
}

void
targarg(char *w, void *arg, int n)
{
	NODE **ary = arg;
	NODE *p, *q;

	if (w[1] < '0' || w[1] > (n + '0'))
		uerror("bad xasm arg number %c", w[1]);
	if (w[1] == (n + '0'))
		p = ary[(int)w[1]-'0' - 1]; /* XXX */
	else
		p = ary[(int)w[1]-'0'];
	p = p->n_left;

	if (optype(p->n_op) != LTYPE)
		comperr("bad xarg op %d", p->n_op);
	q = tcopy(p);
	if (q->n_op == REG) {
		if (*w == 'k') {
			q->n_type = INT;
		} else if (*w != 'w') {
			cerror("targarg"); /* XXX ??? */
			if (q->n_type > UCHAR) {
				regno(q) = regno(q)*2+8;
				if (*w == 'h')
					regno(q)++;
			}
			q->n_type = INT;
		} else
			q->n_type = SHORT;
	}
	adrput(stdout, q);
	tfree(q);
}

/*
 * target-specific conversion of numeric arguments.
 */
int
numconv(void *ip, void *p1, void *q1)
{
	NODE *p = p1, *q = q1;
	int cw = xasmcode(q->n_name);

	switch (XASMVAL(cw)) {
	case 'a':
	case 'b':
	case 'c':
	case 'd':
		p->n_name = tmpcalloc(2);
		p->n_name[0] = XASMVAL(cw);
		return 1;
	default:
		return 0;
	}
}

static struct {
	char *name; int num;
} xcr[] = {
	{ "rax", RAX },
	{ "rbx", RBX },
	{ "rcx", RCX },
	{ "rdx", RDX },
	{ "rsi", RSI },
	{ "rdi", RDI },
	{ "st", 040 },
	{ "st(0)", 040 },
	{ "st(1)", 041 },
	{ "st(2)", 042 },
	{ "st(3)", 043 },
	{ "st(4)", 044 },
	{ "st(5)", 045 },
	{ "st(6)", 046 },
	{ "st(7)", 047 },
	{ NULL, 0 },
};

/*
 * Check for other names of the xasm constraints registers.
 */
int xasmconstregs(char *s)
{
	int i;

	for (i = 0; xcr[i].name; i++)
		if (strcmp(xcr[i].name, s) == 0)
			return xcr[i].num;
	return -1;
}

