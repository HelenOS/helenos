/*	$Id: local2.c,v 1.154.2.2 2011/02/26 07:17:34 ragge Exp $	*/
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

#if defined(PECOFFABI) || defined(MACHOABI)
#define EXPREFIX	"_"
#else
#define EXPREFIX	""
#endif


static int stkpos;

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

static int regoff[7];
static TWORD ftype;

/*
 * Print out the prolog assembler.
 * addto and regoff are already calculated.
 */
static void
prtprolog(struct interpass_prolog *ipp, int addto)
{
	int i;

	printf("	pushl %%ebp\n");
	printf("	movl %%esp,%%ebp\n");
#if defined(MACHOABI)
	printf("	subl $8,%%esp\n");	/* 16-byte stack alignment */
#endif
	if (addto)
		printf("	subl $%d,%%esp\n", addto);
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			fprintf(stdout, "	movl %s,-%d(%s)\n",
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
			addto += SZINT/SZCHAR;
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
	printf("	.align 4\n");
	printf("%s:\n", ipp->ipp_name);
#endif
	/*
	 * We here know what register to save and how much to 
	 * add to the stack.
	 */
	addto = offcalc(ipp);
#if defined(MACHOABI)
	addto = (addto + 15) & ~15;	/* stack alignment */
#endif
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
			fprintf(stdout, "	movl -%d(%s),%s\n",
			    regoff[i], rnames[FPREG], rnames[i]);

	/* struct return needs special treatment */
	if (ftype == STRTY || ftype == UNIONTY) {
		printf("	movl 8(%%ebp),%%eax\n");
		printf("	leave\n");
		printf("	ret $%d\n", 4 + ipp->ipp_argstacksize);
	} else {
		printf("	leave\n");
		if (ipp->ipp_argstacksize)
			printf("	ret $%d\n", ipp->ipp_argstacksize);
		else
			printf("	ret\n");
	}

#if defined(ELFABI)
	printf("\t.size " EXPREFIX "%s,.-" EXPREFIX "%s\n", ipp->ipp_name,
	    ipp->ipp_name);
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
 * Emit code to compare two longlong numbers.
 */
static void
twollcomp(NODE *p)
{
	int u;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	u = p->n_op;
	switch (p->n_op) {
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
		u += (ULE-LE);
		/* FALLTHROUGH */
	case ULE:
	case ULT:
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case UGE:
	case UGT:
		cb1 = LT;
		cb2 = GT;
		break;
	
	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 4, cb2 += 4;
	expand(p, 0, "	cmpl UR,UL\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
	expand(p, 0, "	cmpl AR,AL\n");
	cbgen(u, e);
	deflab(s);
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
		val = (CONSZ)1 << UPKFSZ(p->n_rval);
		--val;
		val <<= UPKFOFF(p->n_rval);
		printf("0x%llx", (**cp == 'M' ? val : ~val) & 0xffffffff);
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
	case LONG:
		ch = 'l';
		sz = 32;
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

/* long long bitfield assign */
static void
llbf(NODE *p)
{
	NODE *q;
	char buf[50];
	CONSZ m, n;
	int o, s;
	int ml, mh, nl, nh;

	q = p->n_left;
	o = UPKFOFF(q->n_rval);
	s = UPKFSZ(q->n_rval);
	m = (CONSZ)1 << (s-1);
	m--;
	m = (m << 1) | 1;
	m <<= o;
	n = ~m;

	ml = m & 0xffffffff;
	nl = n & 0xffffffff;
	mh = (m >> 32) & 0xffffffff;
	nh = (n >> 32) & 0xffffffff;

#define	S(...)	snprintf(buf, sizeof buf, __VA_ARGS__); expand(p, 0, buf)

	if (o < 32) { /* lower 32 buts */
		S("	andl $0x%x,AL\n", nl);
		S("	movl AR,A1\n");
		S("	sall $%d,A1\n", o);
		S("	andl $0x%x,A1\n", ml);
		S("	orl A1,AL\n");
	}
	if ((o+s) >= 32) { /* upper 32 bits */
		S("	andl $0x%x,UL\n", nh);
		S("	movl UR,A1\n");
		S("	sall $%d,A1\n", o);
		S("	movl AR,U1\n");
		S("	shrl $%d,U1\n", 32-o);
		S("	orl U1,A1\n");
		S("	andl $0x%x,A1\n", mh);
		S("	orl A1,UL\n");
	}
#undef S
//	fwalk(p, e2print, 0);

	
}

/*
 * Push a structure on stack as argument.
 * the scratch registers are already free here
 */
static void
starg(NODE *p)
{
	FILE *fp = stdout;

#if defined(MACHOABI)
	fprintf(fp, "	subl $%d,%%esp\n", p->n_stsize);
	fprintf(fp, "	subl $4,%%esp\n");
	fprintf(fp, "	pushl $%d\n", p->n_stsize);
	expand(p, 0, "	pushl AL\n");
	expand(p, 0, "	leal 12(%esp),A1\n");
	expand(p, 0, "	pushl A1\n");
	if (kflag) {
		fprintf(fp, "	call L%s$stub\n", EXPREFIX "memcpy");
		addstub(&stublist, EXPREFIX "memcpy");
	} else {
		fprintf(fp, "	call %s\n", EXPREFIX "memcpy");
	}
	fprintf(fp, "	addl $16,%%esp\n");
#else
	fprintf(fp, "	subl $%d,%%esp\n", (p->n_stsize+3) & ~3);
	fprintf(fp, "	pushl $%d\n", p->n_stsize);
	expand(p, 0, "	pushl AL\n");
	expand(p, 0, "	leal 8(%esp),A1\n");
	expand(p, 0, "	pushl A1\n");
	fprintf(fp, "	call %s%s\n", EXPREFIX "memcpy", kflag ? "@PLT" : "");
	fprintf(fp, "	addl $12,%%esp\n");
#endif
}

/*
 * Compare two floating point numbers.
 */
static void
fcomp(NODE *p)  
{
	static char *fpcb[] = { "jz", "jnz", "jbe", "jc", "jnc", "ja" };

	if ((p->n_su & DORIGHT) == 0)
		expand(p, 0, "  fxch\n");
	expand(p, 0, "  fucomip %st(1),%st\n");	/* emit compare insn  */
	expand(p, 0, "  fstp %st(0)\n");	/* pop fromstack */

	if (p->n_op == NE || p->n_op == GT || p->n_op == GE)
		expand(p, 0, "  jp LC\n");
	else if (p->n_op == EQ)
		printf("\tjp 1f\n");
	printf("	%s ", fpcb[p->n_op - EQ]);
	expand(p, 0, "LC\n");
	if (p->n_op == EQ)
		printf("1:\n");
}

/*
 * Convert an unsigned long long to floating point number.
 */
static void
ulltofp(NODE *p)
{
	static int loadlab;
	int jmplab;

	if (loadlab == 0) {
		loadlab = getlab2();
		expand(p, 0, "	.data\n");
		printf(LABFMT ":	.long 0,0x80000000,0x403f\n", loadlab);
		expand(p, 0, "	.text\n");
	}
	jmplab = getlab2();
	expand(p, 0, "	pushl UL\n	pushl AL\n");
	expand(p, 0, "	fildq (%esp)\n");
	expand(p, 0, "	addl $8,%esp\n");
	expand(p, 0, "	cmpl $0,UL\n");
	printf("	jge " LABFMT "\n", jmplab);
	printf("	fldt " LABFMT "\n", loadlab);
	printf("	faddp %%st,%%st(1)\n");
	printf(LABFMT ":\n", jmplab);
}

static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;

	if (t < LONGLONG || t == FLOAT || t > BTMASK)
		return 4;
	if (t == LONGLONG || t == ULONGLONG || t == DOUBLE)
		return 8;
	if (t == LDOUBLE)
		return 12;
	if (t == STRTY || t == UNIONTY)
		return (p->n_stsize+3) & ~3;
	comperr("argsiz");
	return 0;
}

static void
fcast(NODE *p)
{
	TWORD t = p->n_type;
	int sz, c;

	if (t >= p->n_left->n_type)
		return; /* cast to more precision */
	if (t == FLOAT)
		sz = 4, c = 's';
	else
		sz = 8, c = 'l';

	printf("	sub $%d,%%esp\n", sz);
	printf("	fstp%c (%%esp)\n", c);
	printf("	fld%c (%%esp)\n", c);
	printf("	add $%d,%%esp\n", sz);
}

static void
llshft(NODE *p)
{
	char *d[3];

	if (p->n_op == LS) {
		d[0] = "l", d[1] = "%eax", d[2] = "%edx";
	} else
		d[0] = "r", d[1] = "%edx", d[2] = "%eax";

	printf("\tsh%sdl %s,%s\n",d[0], d[1], d[2]);
	printf("\ts%s%sl %%cl,%s\n", p->n_op == RS &&
	    p->n_left->n_type == ULONGLONG ? "h" : "a", d[0], d[1]);
	printf("\ttestb $32,%%cl\n");
	printf("\tje 1f\n");
	printf("\tmovl %s,%s\n", d[1], d[2]);
	if (p->n_op == RS && p->n_left->n_type == LONGLONG)
		printf("\tsarl $31,%%edx\n");
	else
		printf("\txorl %s,%s\n",d[1],d[1]);
	printf("1:\n");
}

void
zzzcode(NODE *p, int c)
{
	NODE *l;
	int pr, lr, s;
	char *ch;

	switch (c) {
	case 'A': /* swap st0 and st1 if right is evaluated second */
		if ((p->n_su & DORIGHT) == 0) {
			if (logop(p->n_op))
				printf("	fxch\n");
			else
				printf("r");
		}
		break;

	case 'B': { /* packed bitfield ops */
		int sz, off;

		l = p->n_left;
		sz = UPKFSZ(l->n_rval);
		off = UPKFOFF(l->n_rval);
		if (sz + off <= SZINT)
			break;
		/* lower already printed */
		expand(p, INAREG, "	movl AR,A1\n");
		expand(p, INAREG, "	andl $M,UL\n");
		printf("	sarl $%d,", SZINT-off);
		expand(p, INAREG, "A1\n");
		expand(p, INAREG, "	andl $N,A1\n");
		expand(p, INAREG, "	orl A1,UL\n");
		}
		break;

	case 'C':  /* remove from stack after subroutine call */
#ifdef notyet
		if (p->n_left->n_flags & FSTDCALL)
			break;
#endif
		pr = p->n_qual;
		if (p->n_op == STCALL || p->n_op == USTCALL)
			pr += 4;
		if (p->n_flags & FFPPOP)
			printf("	fstp	%%st(0)\n");
		if (p->n_op == UCALL)
			return; /* XXX remove ZC from UCALL */
		if (pr)
			printf("	addl $%d, %s\n", pr, rnames[ESP]);
		break;

	case 'D': /* Long long comparision */
		twollcomp(p);
		break;

	case 'E': /* Perform bitfield sign-extension */
		bfext(p);
		break;

	case 'F': /* Structure argument */
		if (p->n_stalign != 0) /* already on stack */
			starg(p);
		break;

	case 'G': /* Floating point compare */
		fcomp(p);
		break;

	case 'H': /* assign of longlong between regs */
		rmove(DECRA(p->n_right->n_reg, 0),
		    DECRA(p->n_left->n_reg, 0), LONGLONG);
		break;

	case 'I': /* float casts */
		fcast(p);
		break;

	case 'J': /* convert unsigned long long to floating point */
		ulltofp(p);
		break;

	case 'K': /* Load longlong reg into another reg */
		rmove(regno(p), DECRA(p->n_reg, 0), LONGLONG);
		break;

	case 'L': /* long long bitfield assign */
		llbf(p);
		break;

	case 'M': /* Output sconv move, if needed */
		l = getlr(p, 'L');
		/* XXX fixneed: regnum */
		pr = DECRA(p->n_reg, 0);
		lr = DECRA(l->n_reg, 0);
		if ((pr == AL && lr == EAX) || (pr == BL && lr == EBX) ||
		    (pr == CL && lr == ECX) || (pr == DL && lr == EDX))
			;
		else
			printf("	movb %%%cl,%s\n",
			    rnames[lr][2], rnames[pr]);
		l->n_rval = l->n_reg = p->n_reg; /* XXX - not pretty */
		break;

	case 'N': /* output extended reg name */
		printf("%s", rnames[getlr(p, '1')->n_rval]);
		break;

	case 'O': /* print out emulated ops */
		pr = 16;
		if (p->n_op == RS || p->n_op == LS) {
			llshft(p);
			break;
		} else if (p->n_op == MUL) {
			printf("\timull %%ecx, %%edx\n");
			printf("\timull %%eax, %%esi\n");
			printf("\taddl %%edx, %%esi\n");
			printf("\tmull %%ecx\n");
			printf("\taddl %%esi, %%edx\n");
			break;
		}
		expand(p, INCREG, "\tpushl UR\n\tpushl AR\n");
		expand(p, INCREG, "\tpushl UL\n\tpushl AL\n");
		if (p->n_op == DIV && p->n_type == ULONGLONG) ch = "udiv";
		else if (p->n_op == DIV) ch = "div";
		else if (p->n_op == MOD && p->n_type == ULONGLONG) ch = "umod";
		else if (p->n_op == MOD) ch = "mod";
		else ch = 0, comperr("ZO");
#ifdef ELFABI
		printf("\tcall " EXPREFIX "__%sdi3%s\n\taddl $%d,%s\n",
			ch, (kflag ? "@PLT" : ""), pr, rnames[ESP]);
#else
		printf("\tcall " EXPREFIX "__%sdi3\n\taddl $%d,%s\n",
			ch, pr, rnames[ESP]);
#endif
                break;

	case 'P': /* push hidden argument on stack */
		printf("\tleal -%d(%%ebp),", stkpos);
		adrput(stdout, getlr(p, '1'));
		printf("\n\tpushl ");
		adrput(stdout, getlr(p, '1'));
		putchar('\n');
		break;

	case 'Q': /* emit struct assign */
		/*
		 * With <= 16 bytes, put out mov's, otherwise use movsb/w/l.
		 * esi/edi/ecx are available.
		 * XXX should not need esi/edi if not rep movsX.
		 * XXX can save one insn if src ptr in reg.
		 */
		switch (p->n_stsize) {
		case 1:
			expand(p, INAREG, "	movb (%esi),%cl\n");
			expand(p, INAREG, "	movb %cl,AL\n");
			break;
		case 2:
			expand(p, INAREG, "	movw (%esi),%cx\n");
			expand(p, INAREG, "	movw %cx,AL\n");
			break;
		case 4:
			expand(p, INAREG, "	movl (%esi),%ecx\n");
			expand(p, INAREG, "	movl %ecx,AL\n");
			break;
		default:
			expand(p, INAREG, "	leal AL,%edi\n");
			if (p->n_stsize <= 16 && (p->n_stsize & 3) == 0) {
				printf("	movl (%%esi),%%ecx\n");
				printf("	movl %%ecx,(%%edi)\n");
				printf("	movl 4(%%esi),%%ecx\n");
				printf("	movl %%ecx,4(%%edi)\n");
				if (p->n_stsize > 8) {
					printf("	movl 8(%%esi),%%ecx\n");
					printf("	movl %%ecx,8(%%edi)\n");
				}
				if (p->n_stsize == 16) {
					printf("\tmovl 12(%%esi),%%ecx\n");
					printf("\tmovl %%ecx,12(%%edi)\n");
				}
			} else {
				if (p->n_stsize > 4) {
					printf("\tmovl $%d,%%ecx\n",
					    p->n_stsize >> 2);
					printf("	rep movsl\n");
				}
				if (p->n_stsize & 2)
					printf("	movsw\n");
				if (p->n_stsize & 1)
					printf("	movsb\n");
			}
			break;
		}
		break;

	case 'S': /* emit eventual move after cast from longlong */
		pr = DECRA(p->n_reg, 0);
		lr = p->n_left->n_rval;
		switch (p->n_type) {
		case CHAR:
		case UCHAR:
			if (rnames[pr][2] == 'l' && rnames[lr][2] == 'x' &&
			    rnames[pr][1] == rnames[lr][1])
				break;
			if (rnames[lr][2] == 'x') {
				printf("\tmovb %%%cl,%s\n",
				    rnames[lr][1], rnames[pr]);
				break;
			}
			/* Must go via stack */
			s = BITOOR(freetemp(1));
			printf("\tmovl %%e%ci,%d(%%ebp)\n", rnames[lr][1], s);
			printf("\tmovb %d(%%ebp),%s\n", s, rnames[pr]);
//			comperr("SCONV1 %s->%s", rnames[lr], rnames[pr]);
			break;

		case SHORT:
		case USHORT:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			printf("\tmovw %%%c%c,%%%s\n",
			    rnames[lr][1], rnames[lr][2], rnames[pr]+2);
			break;
		case INT:
		case UNSIGNED:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			printf("\tmovl %%e%c%c,%s\n",
				    rnames[lr][1], rnames[lr][2], rnames[pr]);
			break;

		default:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			comperr("SCONV2 %s->%s", rnames[lr], rnames[pr]);
			break;
		}
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
	int val = (int)p->n_lval;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%d", val);
		} else
			fprintf(fp, "%d", val);
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

	if (p->n_op == FLD)
		p = p->n_left;

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
	/* output an address, with offsets, from p */

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {

	case NAME:
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (p->n_lval != 0)
				fprintf(io, "+" CONFMT, p->n_lval);
		} else
			fprintf(io, CONFMT, p->n_lval);
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (p->n_lval)
			fprintf(io, "%d", (int)p->n_lval);
		if (R2TEST(r)) {
			fprintf(io, "(%s,%s,4)", rnames[R2UPK1(r)],
			    rnames[R2UPK2(r)]);
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
#ifdef PCC_DEBUG
		/* Sanitycheck for PIC, to catch adressable constants */
		if (kflag && p->n_name[0] && 0) {
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
		case LONGLONG:
		case ULONGLONG:
			fprintf(io, "%%%c%c%c", rnames[p->n_rval][0],
			    rnames[p->n_rval][1], rnames[p->n_rval][2]);
			break;
		case SHORT:
		case USHORT:
			fprintf(io, "%%%s", &rnames[p->n_rval][2]);
			break;
		default:
			fprintf(io, "%s", rnames[p->n_rval]);
		}
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
	case LS:
	case RS:
		if (p->n_type != LONGLONG && p->n_type != ULONGLONG)
			break;
		if (p->n_right->n_op == ICON) /* constants must be char */
			p->n_right->n_type = CHAR;
		break;
	}
}

/*
 * Must store floats in memory if there are two function calls involved.
 */
static int
storefloat(struct interpass *ip, NODE *p)
{
	int l, r;

	switch (optype(p->n_op)) {
	case BITYPE:
		l = storefloat(ip, p->n_left);
		r = storefloat(ip, p->n_right);
		if (p->n_op == CM)
			return 0; /* arguments, don't care */
		if (callop(p->n_op))
			return 1; /* found one */
#define ISF(p) ((p)->n_type == FLOAT || (p)->n_type == DOUBLE || \
	(p)->n_type == LDOUBLE)
		if (ISF(p->n_left) && ISF(p->n_right) && l && r) {
			/* must store one. store left */
			struct interpass *nip;
			TWORD t = p->n_left->n_type;
			NODE *ll;
			int off;

                	off = BITOOR(freetemp(szty(t)));
                	ll = mklnode(OREG, off, FPREG, t);
			nip = ipnode(mkbinode(ASSIGN, ll, p->n_left, t));
			p->n_left = mklnode(OREG, off, FPREG, t);
                	DLIST_INSERT_BEFORE(ip, nip, qelem);
		}
		return l|r;

	case UTYPE:
		l = storefloat(ip, p->n_left);
		if (callop(p->n_op))
			l = 1;
		return l;
	default:
		return 0;
	}
}

static void
outfargs(struct interpass *ip, NODE **ary, int num, int *cwp, int c)
{
	struct interpass *ip2;
	NODE *q, *r;
	int i;

	for (i = 0; i < num; i++)
		if (XASMVAL(cwp[i]) == c && (cwp[i] & (XASMASG|XASMINOUT)))
			break;
	if (i == num)
		return;
	q = ary[i]->n_left;
	r = mklnode(REG, 0, c == 'u' ? 040 : 037, q->n_type);
	ary[i]->n_left = tcopy(r);
	ip2 = ipnode(mkbinode(ASSIGN, q, r, q->n_type));
	DLIST_INSERT_AFTER(ip, ip2, qelem);
}

static void
infargs(struct interpass *ip, NODE **ary, int num, int *cwp, int c)
{
	struct interpass *ip2;
	NODE *q, *r;
	int i;

	for (i = 0; i < num; i++)
		if (XASMVAL(cwp[i]) == c && (cwp[i] & XASMASG) == 0)
			break;
	if (i == num)
		return;
	q = ary[i]->n_left;
	q = (cwp[i] & XASMINOUT) ? tcopy(q) : q;
	r = mklnode(REG, 0, c == 'u' ? 040 : 037, q->n_type);
	if ((cwp[i] & XASMINOUT) == 0)
		ary[i]->n_left = tcopy(r);
	ip2 = ipnode(mkbinode(ASSIGN, r, q, q->n_type));
	DLIST_INSERT_BEFORE(ip, ip2, qelem);
}

/*
 * Extract float args to XASM and ensure that they are put on the stack
 * in correct order.
 * This should be done sow other way.
 */
static void
fixxfloat(struct interpass *ip, NODE *p)
{
	NODE *w, **ary;
	int nn, i, c, *cwp;

	nn = 1;
	w = p->n_left;
	if (w->n_op == ICON && w->n_type == STRTY)
		return;
	/* index all xasm args first */
	for (; w->n_op == CM; w = w->n_left)
		nn++;
	ary = tmpcalloc(nn * sizeof(NODE *));
	cwp = tmpcalloc(nn * sizeof(int));
	for (i = 0, w = p->n_left; w->n_op == CM; w = w->n_left) {
		ary[i] = w->n_right;
		cwp[i] = xasmcode(ary[i]->n_name);
		i++;
	}
	ary[i] = w;
	cwp[i] = xasmcode(ary[i]->n_name);
	for (i = 0; i < nn; i++)
		if (XASMVAL(cwp[i]) == 't' || XASMVAL(cwp[i]) == 'u')
			break;
	if (i == nn)
		return;

	for (i = 0; i < nn; i++) {
		c = XASMVAL(cwp[i]);
		if (c >= '0' && c <= '9')
			cwp[i] = (cwp[i] & ~0377) | XASMVAL(cwp[c-'0']);
	}
	infargs(ip, ary, nn, cwp, 'u');
	infargs(ip, ary, nn, cwp, 't');
	outfargs(ip, ary, nn, cwp, 't');
	outfargs(ip, ary, nn, cwp, 'u');
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
		storefloat(ip, ip->ip_node);
		if (ip->ip_node->n_op == XASM)
			fixxfloat(ip, ip->ip_node);
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

static char rl[] =
  { EAX, EAX, EAX, EAX, EAX, EDX, EDX, EDX, EDX, ECX, ECX, ECX, EBX, EBX, ESI };
static char rh[] =
  { EDX, ECX, EBX, ESI, EDI, ECX, EBX, ESI, EDI, EBX, ESI, EDI, ESI, EDI, EDI };

void
rmove(int s, int d, TWORD t)
{
	int sl, sh, dl, dh;

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
#if 1
		sl = rl[s-EAXEDX];
		sh = rh[s-EAXEDX];
		dl = rl[d-EAXEDX];
		dh = rh[d-EAXEDX];

		/* sanity checks, remove when satisfied */
		if (memcmp(rnames[s], rnames[sl]+1, 3) != 0 ||
		    memcmp(rnames[s]+3, rnames[sh]+1, 3) != 0)
			comperr("rmove source error");
		if (memcmp(rnames[d], rnames[dl]+1, 3) != 0 ||
		    memcmp(rnames[d]+3, rnames[dh]+1, 3) != 0)
			comperr("rmove dest error");
#define	SW(x,y) { int i = x; x = y; y = i; }
		if (sh == dl) {
			/* Swap if overwriting */
			SW(sl, sh);
			SW(dl, dh);
		}
		if (sl != dl)
			printf("	movl %s,%s\n", rnames[sl], rnames[dl]);
		if (sh != dh)
			printf("	movl %s,%s\n", rnames[sh], rnames[dh]);
#else
		if (memcmp(rnames[s], rnames[d], 3) != 0)
			printf("	movl %%%c%c%c,%%%c%c%c\n",
			    rnames[s][0],rnames[s][1],rnames[s][2],
			    rnames[d][0],rnames[d][1],rnames[d][2]);
		if (memcmp(&rnames[s][3], &rnames[d][3], 3) != 0)
			printf("	movl %%%c%c%c,%%%c%c%c\n",
			    rnames[s][3],rnames[s][4],rnames[s][5],
			    rnames[d][3],rnames[d][4],rnames[d][5]);
#endif
		break;
	case CHAR:
	case UCHAR:
		printf("	movb %s,%s\n", rnames[s], rnames[d]);
		break;
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
#ifdef notdef
		/* a=b()*c(); will generate this */
		comperr("bad float rmove: %d %d", s, d);
#endif
		break;
	default:
		printf("	movl %s,%s\n", rnames[s], rnames[d]);
	}
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	int num;

	switch (c) {
	case CLASSA:
		num = r[CLASSB] > 4 ? 4 : r[CLASSB];
		num += 2*r[CLASSC];
		num += r[CLASSA];
		return num < 6;
	case CLASSB:
		num = r[CLASSA];
		num += 2*r[CLASSC];
		num += r[CLASSB];
		return num < 4;
	case CLASSC:
		num = r[CLASSA];
		num += r[CLASSB] > 4 ? 4 : r[CLASSB];
		num += 2*r[CLASSC];
		return num < 5;
	case CLASSD:
		return r[CLASSD] < DREGCNT;
	}
	return 0; /* XXX gcc */
}

char *rnames[] = {
	"%eax", "%edx", "%ecx", "%ebx", "%esi", "%edi", "%ebp", "%esp",
	"%al", "%ah", "%dl", "%dh", "%cl", "%ch", "%bl", "%bh",
	"eaxedx", "eaxecx", "eaxebx", "eaxesi", "eaxedi", "edxecx",
	"edxebx", "edxesi", "edxedi", "ecxebx", "ecxesi", "ecxedi",
	"ebxesi", "ebxedi", "esiedi",
	"%st0", "%st1", "%st2", "%st3", "%st4", "%st5", "%st6", "%st7",
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t == CHAR || t == UCHAR)
		return CLASSB;
	if (t == LONGLONG || t == ULONGLONG)
		return CLASSC;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSD;
	return CLASSA;
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
#if defined(ELFABI)
	if (kflag)
		size -= 4;
#endif

	
#if defined(MACHOABI)
	int newsize = (size + 15) & ~15;	/* stack alignment */
	int align = newsize-size;

	if (align != 0)
		printf("	subl $%d,%%esp\n", align);

	size=newsize;
#endif
	
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
	switch (c) {
	case 'D': reg = EDI; break;
	case 'S': reg = ESI; break;
	case 'a': reg = EAX; break;
	case 'b': reg = EBX; break;
	case 'c': reg = ECX; break;
	case 'd': reg = EDX; break;

	case 't':
	case 'u':
		p->n_name = tmpstrdup(p->n_name);
		w = strchr(p->n_name, XASMVAL(cw));
		*w = 'r'; /* now reg */
		return 1;

	case 'A': reg = EAXEDX; break;
	case 'q': {
		/* Set edges in MYSETXARG */
		if (p->n_left->n_op == REG || p->n_left->n_op == TEMP)
			return 1;
		t = p->n_left->n_type;
		if (in && ut)
			in = tcopy(in);
		p->n_left = mklnode(TEMP, 0, p2env.epp->ip_tmpnum++, t);
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

	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
		if (p->n_left->n_op != ICON)
			uerror("xasm arg not constant");
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
	if (reg == EAXEDX) {
		p->n_label = CLASSC;
	} else {
		p->n_label = CLASSA;
		if (t == CHAR || t == UCHAR) {
			p->n_label = CLASSB;
			reg = reg * 2 + 8;
		}
	}
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE) {
		p->n_label = CLASSD;
		reg += 037;
	}

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
targarg(char *w, void *arg)
{
	NODE **ary = arg;
	NODE *p, *q;

	if (ary[(int)w[1]-'0'] == 0)
		p = ary[(int)w[1]-'0'-1]->n_left; /* XXX */
	else
		p = ary[(int)w[1]-'0']->n_left;
	if (optype(p->n_op) != LTYPE)
		comperr("bad xarg op %d", p->n_op);
	q = tcopy(p);
	if (q->n_op == REG) {
		if (*w == 'k') {
			q->n_type = INT;
		} else if (*w != 'w') {
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
		p->n_name[0] = (char)XASMVAL(cw);
		return 1;
	default:
		return 0;
	}
}

static struct {
	char *name; int num;
} xcr[] = {
	{ "eax", EAX },
	{ "ebx", EBX },
	{ "ecx", ECX },
	{ "edx", EDX },
	{ "esi", ESI },
	{ "edi", EDI },
	{ "ax", EAX },
	{ "bx", EBX },
	{ "cx", ECX },
	{ "dx", EDX },
	{ NULL, 0 },
};

/*
 * Check for other names of the xasm constraints registers.
 */

/*
 * Check for other names of the xasm constraints registers.
 */
int xasmconstregs(char *s)
{
	int i;

	if (strncmp(s, "st", 2) == 0) {
		int off =0;
		if (s[2] == '(' && s[4] == ')')
			off = s[3] - '0';
		return ESIEDI + 1 + off;
	}

	for (i = 0; xcr[i].name; i++)
		if (strcmp(xcr[i].name, s) == 0)
			return xcr[i].num;
	return -1;
}

