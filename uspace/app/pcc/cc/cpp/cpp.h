/*	$Id: cpp.h,v 1.47.2.1 2011/02/26 06:36:40 ragge Exp $	*/

/*
 * Copyright (c) 2004,2010 Anders Magnusson (ragge@ludd.luth.se).
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

#include <stdio.h> /* for obuf */
#include <stdlib.h>

#include "config.h"

typedef unsigned char usch;
extern usch yytext[];
extern usch *stringbuf;

extern	int	trulvl;
extern	int	flslvl;
extern	int	elflvl;
extern	int	elslvl;
extern	int	tflag, Cflag, Pflag;
extern	int	Mflag, dMflag;
extern	usch	*Mfile;
extern	int	ofd;

/* args for lookup() */
#define FIND    0
#define ENTER   1

/* buffer used internally */
#ifndef CPPBUF
#if defined(__pdp11__)
#define CPPBUF  BUFSIZ
#define	BUF_STACK
#elif defined(WIN32)
/* winxp seems to fail > 26608 bytes */
#define CPPBUF	16384
#else
#define CPPBUF	(65536*2)
#endif
#endif

#define	MAXARGS	128	/* Max # of args to a macro. Should be enouth */

#define	NAMEMAX	CPPBUF	/* currently pushbackbuffer */
#define	BBUFSZ	(NAMEMAX+CPPBUF+1)

#define GCCARG	0xfd	/* has gcc varargs that may be replaced with 0 */
#define VARG	0xfe	/* has varargs */
#define OBJCT	0xff
#define WARN	1	/* SOH, not legal char */
#define CONC	2	/* STX, not legal char */
#define SNUFF	3	/* ETX, not legal char */
#define	EBLOCK	4	/* EOT, not legal char */

/* Used in macro expansion */
#define RECMAX	10000			/* max # of recursive macros */
extern struct symtab *norep[RECMAX];
extern int norepptr;
extern unsigned short bptr[RECMAX];
extern int bidx;
#define	MKB(l,h)	(l+((h)<<8))

/* quick checks for some characters */
#define C_SPEC	001
#define C_EP	002
#define C_ID	004
#define C_I	(C_SPEC|C_ID)		
#define C_2	010		/* for yylex() tokenizing */
#define	C_WSNL	020		/* ' ','\t','\r','\n' */
#define	iswsnl(x) (spechr[x] & C_WSNL)
extern char spechr[];

/* definition for include file info */
struct includ {
	struct includ *next;
	const usch *fname;	/* current fn, changed if #line found */
	const usch *orgfn;	/* current fn, not changed */
	int lineno;
	int infil;
	usch *curptr;
	usch *maxread;
	usch *ostr;
	usch *buffer;
	int idx;
	void *incs;
	const usch *fn;
#ifdef BUF_STACK
	usch bbuf[BBUFSZ];
#else
	usch *bbuf;
#endif
} *ifiles;

/* Symbol table entry  */
struct symtab {
	const usch *namep;    
	const usch *value;    
	const usch *file;
	int line;
};

struct initar {
	struct initar *next;
	int type;
	char *str;
};

/*
 * Struct used in parse tree evaluation.
 * op is one of:
 *	- number type (NUMBER, UNUMBER)
 *	- zero (0) if divided by zero.
 */
struct nd {
	int op;
	union {
		long long val;
		unsigned long long uval;
	} n;
};

#define nd_val n.val
#define nd_uval n.uval

struct symtab *lookup(const usch *namep, int enterf);
usch *gotident(struct symtab *nl);
int slow;	/* scan slowly for new tokens */
int submac(struct symtab *nl, int);
int kfind(struct symtab *nl);
int doexp(void);
int donex(void);

int pushfile(const usch *fname, const usch *fn, int idx, void *incs);
void popfile(void);
void prtline(void);
int yylex(void);
int sloscan(void);
void cunput(int);
int curline(void);
char *curfile(void);
void setline(int);
void setfile(char *);
int yyparse(void);
void yyerror(const char *);
void unpstr(const usch *);
usch *savstr(const usch *str);
void savch(int c);
void mainscan(void);
void putch(int);
void putstr(const usch *s);
void line(void);
usch *sheap(const char *fmt, ...);
void xwarning(usch *);
void xerror(usch *);
#ifdef HAVE_CPP_VARARG_MACRO_GCC
#define warning(...) xwarning(sheap(__VA_ARGS__))
#define error(...) xerror(sheap(__VA_ARGS__))
#else
#define warning printf
#define error printf
#endif
int cinput(void);
void getcmnt(void);
