/*	$Id: pass1.h,v 1.215 2011/02/17 13:44:13 ragge Exp $	*/
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
 * notice, this list of conditionsand the following disclaimer in the
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

#include "config.h"

#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifndef MKEXT
#include "external.h"
#else
typedef unsigned int bittype; /* XXX - for basicblock */
#endif
#include "manifest.h"

#include "ccconfig.h"

/*
 * Storage classes
 */
#define SNULL		0
#define AUTO		1
#define EXTERN		2
#define STATIC		3
#define REGISTER	4
#define EXTDEF		5
/* #define LABEL	6*/
/* #define ULABEL	7*/
#define MOS		8
#define PARAM		9
#define STNAME		10
#define MOU		11
#define UNAME		12
#define TYPEDEF		13
/* #define FORTRAN		14 */
#define ENAME		15
#define MOE		16
/* #define UFORTRAN 	17 */
#define USTATIC		18

	/* field size is ORed in */
#define FIELD		0200
#define FLDSIZ		0177
extern	char *scnames(int);

/*
 * Symbol table flags
 */
#define	SNORMAL		0
#define	STAGNAME	01
#define	SLBLNAME	02
#define	SMOSNAME	03
#define	SSTRING		04
#define	NSTYPES		05
#define	SMASK		07

/* #define SSET		00010 */
/* #define SREF		00020 */
#define SNOCREAT	00040	/* don't create a symbol in lookup() */
#define STEMP		00100	/* Allocate symtab from temp or perm mem */
#define	SDYNARRAY	00200	/* symbol is dynamic array on stack */
#define	SINLINE		00400	/* function is of type inline */
#define	STNODE		01000	/* symbol shall be a temporary node */
#define	SASG		04000	/* symbol is assigned to already */
#define	SLOCAL1		010000
#define	SLOCAL2		020000
#define	SLOCAL3		040000

	/* alignment of initialized quantities */
#ifndef AL_INIT
#define	AL_INIT ALINT
#endif

struct rstack;
struct symtab;
union arglist;
#ifdef GCC_COMPAT
struct gcc_attr_pack;
#endif

/*
 * Dimension/prototype information.
 * 	ddim > 0 holds the dimension of an array.
 *	ddim < 0 is a dynamic array and refers to a tempnode.
 *	...unless:
 *		ddim == NOOFFSET, an array without dimenston, "[]"
 *		ddim == -1, dynamic array while building before defid.
 */
union dimfun {
	int	ddim;		/* Dimension of an array */
	union arglist *dfun;	/* Prototype index */
};

/*
 * Argument list member info when storing prototypes.
 */
union arglist {
	TWORD type;
	union dimfun *df;
	struct attr *sap;
};
#define TNULL		INCREF(FARG) /* pointer to FARG -- impossible type */
#define TELLIPSIS 	INCREF(INCREF(FARG))

/*
 * Symbol table definition.
 */
struct	symtab {
	struct	symtab *snext;	/* link to other symbols in the same scope */
	int	soffset;	/* offset or value */
	char	sclass;		/* storage class */
	char	slevel;		/* scope level */
	short	sflags;		/* flags, see below */
	char	*sname;		/* Symbol name */
	char	*soname;	/* Written-out name */
	TWORD	stype;		/* type word */
	TWORD	squal;		/* qualifier word */
	union	dimfun *sdf;	/* ptr to the dimension/prototype array */
	struct	attr *sap;	/* the base type attribute list */
};

struct attr2 {
	struct attr *next;
	int atype;
	union aarg aa[2];
};

#define	ISSOU(ty)   ((ty) == STRTY || (ty) == UNIONTY)
#define	MKAP(type)  ((struct attr *)&btattr[type])
extern const struct attr2 btattr[];

/*
 * External definitions
 */
struct swents {			/* switch table */
	struct swents *next;	/* Next struct in linked list */
	CONSZ	sval;		/* case value */
	int	slab;		/* associated label */
};
int mygenswitch(int, TWORD, struct swents **, int);

extern	int blevel;
extern	int instruct, got_type;
extern	int oldstyle;
extern	int oflag;

extern	int lineno, nerrors;

extern	char *ftitle;
extern	struct symtab *cftnsp;
extern	int autooff, maxautooff, argoff, strucoff;
extern	int brkflag;

extern	OFFSZ inoff;

extern	int reached;
extern	int isinlining;
extern	int xinline;

extern	int sdebug, idebug, pdebug;

/* various labels */
extern	int brklab;
extern	int contlab;
extern	int flostat;
extern	int retlab;

/* pragma globals */
extern int pragma_allpacked, pragma_packed, pragma_aligned;
extern char *pragma_renamed;

/*
 * Flags used in the (elementary) flow analysis ...
 */
#define FBRK		02
#define FCONT		04
#define FDEF		010
#define FLOOP		020

/*	mark an offset which is undefined */

#define NOOFFSET	(-10201)

/* declarations of various functions */
extern	NODE
	*buildtree(int, NODE *, NODE *r),
	*mkty(unsigned, union dimfun *, struct attr *),
	*rstruct(char *, int),
	*dclstruct(struct rstack *),
	*strend(int gtype, char *),
	*tymerge(NODE *, NODE *),
	*stref(NODE *),
	*offcon(OFFSZ, TWORD, union dimfun *, struct attr *),
	*bcon(int),
	*xbcon(CONSZ, struct symtab *, TWORD),
	*bpsize(NODE *),
	*convert(NODE *, int),
	*pconvert(NODE *),
	*oconvert(NODE *),
	*ptmatch(NODE *),
	*tymatch(NODE *),
	*makety(NODE *, TWORD, TWORD, union dimfun *, struct attr *),
	*block(int, NODE *, NODE *, TWORD, union dimfun *, struct attr *),
	*doszof(NODE *),
	*talloc(void),
	*optim(NODE *),
	*clocal(NODE *),
	*ccopy(NODE *),
	*tempnode(int, TWORD, union dimfun *, struct attr *),
	*eve(NODE *),
	*doacall(struct symtab *, NODE *, NODE *);
NODE	*intprom(NODE *);
OFFSZ	tsize(TWORD, union dimfun *, struct attr *),
	psize(NODE *);
NODE *	typenode(NODE *new);
void	spalloc(NODE *, NODE *, OFFSZ);
char	*exname(char *);
NODE	*floatcon(char *);
NODE	*fhexcon(char *);
NODE	*bdty(int op, ...);
extern struct rstack *rpole;

int oalloc(struct symtab *, int *);
void deflabel(char *, NODE *);
void gotolabel(char *);
unsigned int esccon(char **);
void inline_start(struct symtab *);
void inline_end(void);
void inline_addarg(struct interpass *);
void inline_ref(struct symtab *);
void inline_prtout(void);
void inline_args(struct symtab **, int);
NODE *inlinetree(struct symtab *, NODE *, NODE *);
void ftnarg(NODE *);
struct rstack *bstruct(char *, int, NODE *);
void moedef(char *);
void beginit(struct symtab *);
void simpleinit(struct symtab *, NODE *);
struct symtab *lookup(char *, int);
struct symtab *getsymtab(char *, int);
char *addstring(char *);
char *addname(char *);
void symclear(int);
struct symtab *hide(struct symtab *);
void soumemb(NODE *, char *, int);
int talign(unsigned int, struct attr *);
void bfcode(struct symtab **, int);
int chkftn(union arglist *, union arglist *);
void branch(int);
void cbranch(NODE *, NODE *);
void extdec(struct symtab *);
void defzero(struct symtab *);
int falloc(struct symtab *, int, NODE *);
TWORD ctype(TWORD);  
void ninval(CONSZ, int, NODE *);
void infld(CONSZ, int, CONSZ);
void zbits(CONSZ, int);
void instring(struct symtab *);
void inwstring(struct symtab *);
void plabel(int);
void bjobcode(void);
void ejobcode(int);
void calldec(NODE *, NODE *);
int cisreg(TWORD);
char *tmpsprintf(char *, ...);
char *tmpvsprintf(char *, va_list);
void asginit(NODE *);
void desinit(NODE *);
void endinit(void);
void endictx(void);
void sspinit(void);
void sspstart(void);
void sspend(void);
void ilbrace(void);
void irbrace(void);
CONSZ scalinit(NODE *);
void p1print(char *, ...);
char *copst(int);
int cdope(int);
void myp2tree(NODE *);
void lcommprint(void);
void lcommdel(struct symtab *);
NODE *funcode(NODE *);
struct symtab *enumhd(char *);
NODE *enumdcl(struct symtab *);
NODE *enumref(char *);
CONSZ icons(NODE *);
CONSZ valcast(CONSZ v, TWORD t);
int mypragma(char *);
char *pragtok(char *);
int eat(int);
void fixdef(struct symtab *);
int cqual(TWORD, TWORD);
void defloc(struct symtab *);
int fldchk(int);
int nncon(NODE *);
void cunput(char);
NODE *nametree(struct symtab *sp);
void *inlalloc(int size);
void *blkalloc(int size);
void pass1_lastchance(struct interpass *);
void fldty(struct symtab *p);
int getlab(void);
struct suedef *sueget(struct suedef *p);
void complinit(void);
NODE *structref(NODE *p, int f, char *name);
NODE *cxop(int op, NODE *l, NODE *r);
NODE *imop(int op, NODE *l, NODE *r);
NODE *cxelem(int op, NODE *p);
NODE *cxconj(NODE *p);
NODE *cxret(NODE *p, NODE *q);
NODE *cast(NODE *p, TWORD t, TWORD q);
NODE *ccast(NODE *p, TWORD t, TWORD u, union dimfun *df, struct attr *sue);
int andable(NODE *);
int conval(NODE *, int, NODE *);
int ispow2(CONSZ);
void defid(NODE *q, int class);
void efcode(void);
void ecomp(NODE *p);
void cendarg(void);
int fldal(unsigned int);
int upoff(int size, int alignment, int *poff);
void nidcl(NODE *p, int class);
void eprint(NODE *, int, int *, int *);
int uclass(int class);
int notlval(NODE *);
void ecode(NODE *p);
void bccode(void);
void ftnend(void);
void dclargs(void);
int suemeq(struct attr *s1, struct attr *s2);
struct symtab *strmemb(struct attr *ap);
int yylex(void);
void yyerror(char *);
int pragmas_gcc(char *t);

NODE *builtin_check(NODE *f, NODE *a);

#ifdef SOFTFLOAT
typedef struct softfloat SF;
SF soft_neg(SF);
SF soft_cast(CONSZ v, TWORD);
SF soft_plus(SF, SF);
SF soft_minus(SF, SF);
SF soft_mul(SF, SF);
SF soft_div(SF, SF);
int soft_cmp_eq(SF, SF);
int soft_cmp_ne(SF, SF);
int soft_cmp_ge(SF, SF);
int soft_cmp_gt(SF, SF);
int soft_cmp_le(SF, SF);
int soft_cmp_lt(SF, SF);
int soft_isz(SF);
CONSZ soft_val(SF);
#define FLOAT_NEG(sf)		soft_neg(sf)
#define	FLOAT_CAST(v,t)		soft_cast(v, t)
#define	FLOAT_PLUS(x1,x2)	soft_plus(x1, x2)
#define	FLOAT_MINUS(x1,x2)	soft_minus(x1, x2)
#define	FLOAT_MUL(x1,x2)	soft_mul(x1, x2)
#define	FLOAT_DIV(x1,x2)	soft_div(x1, x2)
#define	FLOAT_ISZERO(sf)	soft_isz(sf)
#define	FLOAT_VAL(sf)		soft_val(sf)
#define FLOAT_EQ(x1,x2)		soft_cmp_eq(x1, x2)
#define FLOAT_NE(x1,x2)		soft_cmp_ne(x1, x2)
#define FLOAT_GE(x1,x2)		soft_cmp_ge(x1, x2)
#define FLOAT_GT(x1,x2)		soft_cmp_gt(x1, x2)
#define FLOAT_LE(x1,x2)		soft_cmp_le(x1, x2)
#define FLOAT_LT(x1,x2)		soft_cmp_lt(x1, x2)
#else
#define	FLOAT_NEG(p)		-(p)
#define	FLOAT_CAST(p,v)		(ISUNSIGNED(v) ? \
		(long double)(U_CONSZ)(p) : (long double)(CONSZ)(p))
#define	FLOAT_PLUS(x1,x2)	(x1) + (x2)
#define	FLOAT_MINUS(x1,x2)	(x1) - (x2)
#define	FLOAT_MUL(x1,x2)	(x1) * (x2)
#define	FLOAT_DIV(x1,x2)	(x1) / (x2)
#define	FLOAT_ISZERO(p)		(p) == 0.0
#define FLOAT_VAL(p)		(CONSZ)(p)
#define FLOAT_EQ(x1,x2)		(x1) == (x2)
#define FLOAT_NE(x1,x2)		(x1) != (x2)
#define FLOAT_GE(x1,x2)		(x1) >= (x2)
#define FLOAT_GT(x1,x2)		(x1) > (x2)
#define FLOAT_LE(x1,x2)		(x1) <= (x2)
#define FLOAT_LT(x1,x2)		(x1) < (x2)
#endif

enum {	ATTR_NONE,

	/* PCC used attributes */
	ATTR_COMPLEX,	/* Internal definition of complex */
	ATTR_BASETYP,	/* Internal; see below */
	ATTR_QUALTYP,	/* Internal; const/volatile, see below */
	ATTR_STRUCT,	/* Internal; element list */
#define	ATTR_MAX ATTR_STRUCT

#ifdef GCC_COMPAT
	/* type attributes */
	GCC_ATYP_ALIGNED,
	GCC_ATYP_PACKED,
	GCC_ATYP_SECTION,
	GCC_ATYP_TRANSP_UNION,
	GCC_ATYP_UNUSED,
	GCC_ATYP_DEPRECATED,
	GCC_ATYP_MAYALIAS,

	/* variable attributes */
	GCC_ATYP_MODE,

	/* function attributes */
	GCC_ATYP_NORETURN,
	GCC_ATYP_FORMAT,
	GCC_ATYP_NONNULL,
	GCC_ATYP_SENTINEL,
	GCC_ATYP_WEAK,
	GCC_ATYP_FORMATARG,
	GCC_ATYP_GNU_INLINE,
	GCC_ATYP_MALLOC,
	GCC_ATYP_NOTHROW,
	GCC_ATYP_CONST,
	GCC_ATYP_PURE,
	GCC_ATYP_CONSTRUCTOR,
	GCC_ATYP_DESTRUCTOR,
	GCC_ATYP_VISIBILITY,
	GCC_ATYP_STDCALL,
	GCC_ATYP_CDECL,
	GCC_ATYP_WARN_UNUSED_RESULT,
	GCC_ATYP_USED,
	GCC_ATYP_NO_INSTR_FUN,
	GCC_ATYP_NOINLINE,
	GCC_ATYP_ALIAS,
	GCC_ATYP_WEAKREF,
	GCC_ATYP_ALLOCSZ,
	GCC_ATYP_ALW_INL,
	GCC_ATYP_TLSMODEL,
	GCC_ATYP_ALIASWEAK,

	/* other stuff */
	GCC_ATYP_BOUNDED,	/* OpenBSD extra boundary checks */

	GCC_ATYP_MAX
#endif
};


/*
 * ATTR_BASETYP has the following layout:
 * aa[0].iarg has size
 * aa[1].iarg has alignment
 * ATTR_QUALTYP has the following layout:
 * aa[0].iarg has CON/VOL + FUN/ARY/PTR
 * Not defined yet...
 * aa[3].iarg is dimension for arrays (XXX future)
 * aa[3].varg is function defs for functions.
 */
#define	atypsz	aa[0].iarg
#define	aalign	aa[1].iarg

/*
 * ATTR_STRUCT member list.
 */
#define amlist  aa[0].varg
#define	strattr(x)	(attr_find(x, ATTR_STRUCT))

#define	iarg(x)	aa[x].iarg
#define	sarg(x)	aa[x].sarg
#define	varg(x)	aa[x].varg

void gcc_init(void);
int gcc_keyword(char *, NODE **);
struct attr *gcc_attr_parse(NODE *);
void gcc_tcattrfix(NODE *);
struct gcc_attrib *gcc_get_attr(struct suedef *, int);
void dump_attr(struct attr *gap);

struct attr *attr_add(struct attr *orig, struct attr *new);
struct attr *attr_new(int, int);
struct attr *attr_find(struct attr *, int);
struct attr *attr_copy(struct attr *src, struct attr *dst, int nelem);
struct attr *attr_dup(struct attr *ap, int n);

#ifdef STABS
void stabs_init(void);
void stabs_file(char *);
void stabs_efile(char *);
void stabs_line(int);
void stabs_rbrac(int);
void stabs_lbrac(int);
void stabs_func(struct symtab *);
void stabs_newsym(struct symtab *);
void stabs_chgsym(struct symtab *);
void stabs_struct(struct symtab *, struct attr *);
#endif

#ifndef CHARCAST
/* to make character constants into character connstants */
/* this is a macro to defend against cross-compilers, etc. */
#define CHARCAST(x) (char)(x)
#endif

/* sometimes int is smaller than pointers */
#if SZPOINT(CHAR) <= SZINT
#define INTPTR  INT
#elif SZPOINT(CHAR) <= SZLONG
#define INTPTR  LONG
#elif SZPOINT(CHAR) <= SZLONGLONG
#define INTPTR  LONGLONG
#else
#error int size unknown
#endif

/*
 * C compiler first pass extra defines.
 */
#define	QUALIFIER	(MAXOP+1)
#define	CLASS		(MAXOP+2)
#define	RB		(MAXOP+3)
#define	DOT		(MAXOP+4)
#define	ELLIPSIS	(MAXOP+5)
#define	TYPE		(MAXOP+6)
#define	LB		(MAXOP+7)
#define	COMOP		(MAXOP+8)
#define	QUEST		(MAXOP+9)
#define	COLON		(MAXOP+10)
#define	ANDAND		(MAXOP+11)
#define	OROR		(MAXOP+12)
#define	NOT		(MAXOP+13)
#define	CAST		(MAXOP+14)
#define	STRING		(MAXOP+15)

/* The following must be in the same order as their NOASG counterparts */
#define	PLUSEQ		(MAXOP+16)
#define	MINUSEQ		(MAXOP+17)
#define	DIVEQ		(MAXOP+18)
#define	MODEQ		(MAXOP+19)
#define	MULEQ		(MAXOP+20)
#define	ANDEQ		(MAXOP+21)
#define	OREQ		(MAXOP+22)
#define	EREQ		(MAXOP+23)
#define	LSEQ		(MAXOP+24)
#define	RSEQ		(MAXOP+25)

#define	UNASG		(-(PLUSEQ-PLUS))+

#define INCR		(MAXOP+26)
#define DECR		(MAXOP+27)
#define SZOF		(MAXOP+28)
#define CLOP		(MAXOP+29)
#define ATTRIB		(MAXOP+30)
#define XREAL		(MAXOP+31)
#define XIMAG		(MAXOP+32)
#define TYMERGE		(MAXOP+33)


/*
 * The following types are only used in pass1.
 */
#define SIGNED		(MAXTYPES+1)
#define BOOL		(MAXTYPES+2)
#define	FIMAG		(MAXTYPES+3)
#define	IMAG		(MAXTYPES+4)
#define	LIMAG		(MAXTYPES+5)
#define	FCOMPLEX	(MAXTYPES+6)
#define	COMPLEX		(MAXTYPES+7)
#define	LCOMPLEX	(MAXTYPES+8)
#define	ENUMTY		(MAXTYPES+9)

#define	ISFTY(x)	((x) >= FLOAT && (x) <= LDOUBLE)
#define	ISCTY(x)	((x) >= FCOMPLEX && (x) <= LCOMPLEX)
#define	ISITY(x)	((x) >= FIMAG && (x) <= LIMAG)
#define ANYCX(p) (p->n_type == STRTY && attr_find(p->n_ap, ATTR_COMPLEX))

#define coptype(o)	(cdope(o)&TYFLG)
#define clogop(o)	(cdope(o)&LOGFLG)
#define casgop(o)	(cdope(o)&ASGFLG)

