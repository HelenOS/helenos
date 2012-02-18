/*	$Id: defs.h,v 1.22 2008/12/24 17:40:41 sgk Exp $	*/
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
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define VL 6
#define XL 8

#define MAXINCLUDES 10
#define MAXLITERALS 20
#define MAXCTL 20
#define MAXHASH 401
#define MAXSTNO 1000
#define MAXEXT 200
#define MAXEQUIV 150
#define MAXLABLIST 100

typedef struct bigblock *bigptr;
typedef union chainedblock *chainp;

extern FILE *infile;
extern FILE *diagfile;
extern long int headoffset;

extern char token [ ];
extern int toklen;
extern int lineno;
extern char *infname;
extern int needkwd;
extern struct labelblock *thislabel;

extern int mflag, tflag;

extern flag profileflag;
extern flag optimflag;
extern flag quietflag;
extern flag nowarnflag;
extern flag ftn66flag;
extern flag shiftcase;
extern flag undeftype;
extern flag shortsubs;
extern flag onetripflag;
extern flag checksubs;
extern flag debugflag;
extern int nerr;
extern int nwarn;
extern int ndata;

extern int parstate;
extern flag headerdone;
extern int blklevel;
extern flag saveall;
extern flag substars;
extern int impltype[ ];
extern int implleng[ ];
extern int implstg[ ];

extern int tyint;
extern int tylogical;
extern ftnint typesize[];
extern int typealign[];
extern int procno;
extern int proctype;
extern char * procname;
extern int rtvlabel[ ];
extern int fudgelabel;	/* to confuse the pdp11 optimizer */
extern struct bigblock *typeaddr;
extern struct bigblock *retslot;
extern int cxslot;
extern int chslot;
extern int chlgslot;
extern int procclass;
extern ftnint procleng;
extern int nentry;
extern flag multitype;
extern int blklevel;
extern int lastlabno;
extern int lastvarno;
extern int lastargslot;
extern int argloc;
extern ftnint autoleng;
extern ftnint bssleng;
extern int retlabel;
extern int ret0label;
extern int dorange;
extern int regnum[ ];
extern bigptr regnamep[ ];
extern int maxregvar;
extern int highregvar;

extern chainp templist;
extern chainp holdtemps;
extern chainp entries;
extern chainp rpllist;
extern chainp curdtp;
extern ftnint curdtelt;
extern flag toomanyinit;

extern flag inioctl;
extern int iostmt;
extern struct bigblock *ioblkp;
extern int nioctl;
extern int nequiv;
extern int nintnames;
extern int nextnames;

struct chain
	{
	chainp nextp;
	bigptr datap;
	};

extern chainp chains;

struct ctlframe
	{
	unsigned ctltype:8;
	unsigned dostepsign:8;
	int ctlabels[4];
	int dolabel;
	struct bigblock *donamep;
	bigptr domax;
	bigptr dostep;
	};
#define endlabel ctlabels[0]
#define elselabel ctlabels[1]
#define dobodylabel ctlabels[1]
#define doposlabel ctlabels[2]
#define doneglabel ctlabels[3]
extern struct ctlframe ctls[ ];
extern struct ctlframe *ctlstack;
extern struct ctlframe *lastctl;

struct extsym
	{
	char extname[XL];
	unsigned extstg:4;
	unsigned extsave:1;
	unsigned extinit:1;
	chainp extp;
	ftnint extleng;
	ftnint maxleng;
	};

extern struct extsym extsymtab[ ];
extern struct extsym *nextext;
extern struct extsym *lastext;

struct labelblock
	{
	int labelno;
	unsigned blklevel:8;
	unsigned labused:1;
	unsigned labinacc:1;
	unsigned labdefined:1;
	unsigned labtype:2;
	ftnint stateno;
	};

extern struct labelblock labeltab[ ];
extern struct labelblock *labtabend;
extern struct labelblock *highlabtab;

struct entrypoint
	{
	chainp nextp;
	struct extsym *entryname;
	chainp arglist;
	int entrylabel;
	int typelabel;
	ptr enamep;
	};

struct primblock
	{
	struct bigblock *namep;
	struct bigblock *argsp;
	bigptr fcharp;
	bigptr lcharp;
	};


struct hashentry
	{
	int hashval;
	struct bigblock *varp;
	};
extern struct hashentry hashtab[ ];
extern struct hashentry *lasthash;

struct intrpacked	/* bits for intrinsic function description */
	{
	unsigned f1:3;
	unsigned f2:4;
	unsigned f3:7;
	};

struct nameblock
	{
	char varname[VL];
	unsigned vdovar:1;
	unsigned vdcldone:1;
	unsigned vadjdim:1;
	unsigned vsave:1;
	unsigned vprocclass:3;
	unsigned vregno:4;
	union	{
		int varno;
		chainp vstfdesc;	/* points to (formals, expr) pair */
		struct intrpacked intrdesc;	/* bits for intrinsic function */
		} vardesc;
	struct dimblock *vdim;
	int voffset;
	};


struct paramblock
	{
	char varname[VL];
	bigptr paramval;
	} ;


struct exprblock
	{
	unsigned opcode:6;
	bigptr leftp;
	bigptr rightp;
	};

struct dcomplex {
	double dreal, dimag;
};

union constant
	{
	char *ccp;
	ftnint ci;
	double cd[2];
	struct dcomplex dc;
	};

struct constblock
	{
	union constant fconst;
	};


struct listblock
	{
	chainp listp;
	};



struct addrblock
	{
	int memno;
	bigptr memoffset;
	unsigned istemp:1;
	unsigned ntempelt:10;
	};



struct errorblock
	{
	int pad;
	};


struct dimblock
	{
	int ndim;
	bigptr nelt;
	bigptr baseoffset;
	bigptr basexpr;
	struct
		{
		bigptr dimsize;
		bigptr dimexpr;
		} dims[1];
	};


struct impldoblock  /* XXXX */
	{
#define	isactive vtype
#define isbusy vclass
	struct bigblock *varnp;
	struct bigblock *varvp;
	bigptr implb;
	bigptr impub;
	bigptr impstep;
	ftnint impdiff;
	ftnint implim;
	chainp datalist;
	};


struct rplblock	/* name replacement block */
	{
	chainp nextp;
	struct bigblock *rplnp;
	ptr rplvp;
	struct bigblock *rplxp;
	int rpltag;
	};



struct equivblock
	{
	ptr equivs;
	unsigned eqvinit:1;
	long int eqvtop;
	long int eqvbottom;
	} ;
#define eqvleng eqvtop

extern struct equivblock eqvclass[ ];


struct eqvchain
	{
	chainp nextp;
	ptr eqvitem;
	long int eqvoffset;
	} ;

union chainedblock
	{
	struct chain chain;
	struct entrypoint entrypoint;
	struct rplblock rplblock;
	struct eqvchain eqvchain;
	};


struct bigblock {
	unsigned tag:4;
	unsigned vtype:4;
	unsigned vclass:4;
	unsigned vstg:4;
	bigptr vleng;
	union {
		struct exprblock _expr;
		struct addrblock _addr;
		struct constblock _const;
		struct errorblock _error;
		struct listblock _list;
		struct primblock _prim;
		struct nameblock _name;
		struct paramblock _param;
		struct impldoblock _impldo;
	} _u;
#define	b_expr		_u._expr
#define	b_addr		_u._addr
#define	b_const		_u._const
#define	b_error		_u._error
#define	b_list		_u._list
#define	b_prim		_u._prim
#define	b_name		_u._name
#define	b_param		_u._param
#define	b_impldo	_u._impldo
};

struct literal
	{
	short littype;
	short litnum;
	union	{
		ftnint litival;
		double litdval;
		struct	{
			char litclen;	/* small integer */
			char litcstr[XL];
			} litcval;
		} litval;
	};

extern struct literal litpool[ ];
extern int nliterals;





/* popular functions with non integer return values */
#define	expptr bigptr
#define	tagptr bigptr

ptr cpblock(int ,void *);

ptr ckalloc(int);
char *varstr(int, char *), *nounder(int, char *), *varunder(int, char *);
char *copyn(int, char *), *copys(char *);
chainp hookup(chainp, chainp), mkchain(bigptr, chainp);
ftnint convci(int, char *), iarrlen(struct bigblock *q);
ftnint lmin(ftnint, ftnint), lmax(ftnint, ftnint);
ftnint simoffset(expptr *);
char *memname(int, int), *convic(ftnint), *setdoto(char *);
double convcd(int, char *);
struct extsym *mkext(char *), 
	*newentry(struct bigblock *),
	*comblock(int, char *s);
struct bigblock *mkname(int, char *);
struct labelblock *mklabel(ftnint);
struct bigblock *addrof(expptr), *call1(int, char *, expptr),
	*call2(int, char *, expptr, expptr),
	*call3(int, char *, expptr, expptr, expptr),
	*call4(int, char *, expptr, expptr, expptr, expptr);
struct bigblock *call0(int, char *), *mkexpr(int, bigptr, bigptr);
struct bigblock *callk(int, char *, bigptr);

struct bigblock *builtin(int, char *), *fmktemp(int, bigptr),
	*mktmpn(int, int, bigptr), *nextdata(ftnint *, ftnint *),
	*autovar(int, int, bigptr), *mklhs(struct bigblock *),
	*mkaddr(struct bigblock *), *putconst(struct bigblock *),
	*memversion(struct bigblock *);
struct bigblock *mkscalar(struct bigblock *np);
struct bigblock *realpart(struct bigblock *p);
struct bigblock *imagpart(struct bigblock *p);

struct bigblock *mkintcon(ftnint), *mkbitcon(int, int, char *),
	*mklogcon(int), *mkaddcon(int), *mkrealcon(int, double),
	*mkstrcon(int, char *), *mkcxcon(bigptr,bigptr);
bigptr mkconst(int t);

bigptr mklist(chainp p);
bigptr mkiodo(chainp, chainp);


bigptr mkconv(int, bigptr),
	mkfunct(struct bigblock *), fixexpr(struct bigblock *),
	fixtype(bigptr);


bigptr cpexpr(bigptr), mkprim(bigptr, struct bigblock *, bigptr, bigptr);
struct bigblock *mkarg(int, int);
struct bigblock *errnode(void);
void initkey(void), prtail(void), puteof(void), done(int);
void fileinit(void), procinit(void), endproc(void), doext(void), preven(int);
int inilex(char *), yyparse(void), newlabel(void), lengtype(int, int);
void err(char *, ...), warn(char *, ...), fatal(char *, ...), enddcl(void);
void p2pass(char *s), frexpr(bigptr), execerr(char *, ...);
void setimpl(int, ftnint, int, int), setlog(void), newproc(void);
void prdbginfo(void), impldcl(struct bigblock *p);
void putbracket(void), enddcl(void), doequiv(void);
void puthead(char *), startproc(struct extsym *, int);
void dclerr(char *s, struct bigblock *v), putforce(int, bigptr);
void entrypt(int, int, ftnint, struct extsym *, chainp);
void settype(struct bigblock *, int, int), putlabel(int);
void putbranch(struct bigblock *p), goret(int), putrbrack(int);
void prolog(struct entrypoint *, struct bigblock *), prendproc(void);
void prlocvar(char *, ftnint), prext(char *, ftnint, int);
void vardcl(struct bigblock *v), frchain(chainp *p); 
void frtemp(struct bigblock *p), incomm(struct extsym *, struct bigblock *);
void setintr(struct bigblock * v), setext(struct bigblock * v);
struct uux { expptr lb, ub; };
void setbound(struct bigblock *, int, struct uux []);
void setfmt(struct labelblock *lp), frdata(chainp), frrpl(void),
	dataval(struct bigblock *, struct bigblock *),
	consnegop(struct bigblock *p), exdo(int, chainp), exelse(void),
	exendif(void), exif(bigptr), exelif(bigptr),
	exequals(struct bigblock *, bigptr),
	exassign(struct bigblock *, struct labelblock *),
	exarif(bigptr, struct labelblock *, struct labelblock *,
	    struct labelblock *);



int intrfunct(char s[VL]), eqn(int, char *, char *);
int fmtstmt(struct labelblock *lp);
int cktype(int, int, int);
int yylex(void), inregister(struct bigblock *);
int inilex(char *), iocname(void);
int maxtype(int, int), flog2(ftnint), hextoi(int);
int cmpstr(char *, char *, ftnint, ftnint);
int enregister(struct bigblock *np);
int conssgn(bigptr p);
int fixargs(int, struct bigblock *);
int addressable(bigptr p);

void prlabel(int);
void prconi(FILE *, int, ftnint);
void prcona(ftnint);
void prconr(FILE *, int, double);
void prarif(bigptr, int, int, int);
void putstr(char *, ftnint);
NODE *putex1(bigptr p);
void puteq(bigptr, bigptr);
void popstack(chainp *p); 
void consconv(int, union constant *, int, union constant *);
void yyerror(char *s);
void enddo(int);
void doinclude(char *);
void flline(void);
void startioctl(void);
void endioctl(void), endio(void), ioclause(int, bigptr), doio(chainp);
void excall(struct bigblock *, struct bigblock *, int, struct labelblock *[]);
void exreturn(expptr p);
void exstop(int, expptr);
void exgoto(struct labelblock *);
void exasgoto(bigptr);
void putcmgo(expptr, int, struct labelblock *[]);
void putexpr(expptr p);
void putif(expptr, int);
void putgoto(int);
void deregister(struct bigblock *np);
NODE *putx(expptr p);
void cpn(int, char *, char *);
void prcmgoto(expptr, int, int, int);
char *lexline(ftnint *n);
bigptr suboffset(struct bigblock *p);
struct bigblock *intraddr(struct bigblock *np);
struct bigblock *intrcall(bigptr, bigptr, int);
void setloc(int);
void prnloc(char *name);
void fprint(bigptr p, int indx);
void ckfree(void *p);

#undef expptr
#undef tagptr

#define	err1 err
#define err2 err
#define	warn1 warn
#define	fatal1 fatal
