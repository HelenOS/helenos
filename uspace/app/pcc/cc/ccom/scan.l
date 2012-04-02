%{
/*	$Id: scan.l,v 1.102 2011/02/16 18:50:42 ragge Exp $	*/

/*
 * Copyright (c) 2002 Anders Magnusson. All rights reserved.
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
%}


D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
P			[Pp][+-]?{D}+
FS			(f|F|l|L)?i?
IS			(u|U|l|L)*
UL			({L}|\\u{H}{H}{H}{H}|\\U{H}{H}{H}{H}{H}{H}{H}{H})

%{
#include <stdlib.h>
#include <errno.h>  
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "pass1.h"
#include "cgram.h"

static NODE *cvtdig(int radix);
static NODE *charcon(void);
static NODE *wcharcon(void);
static void control(int);
static void pragma(void);
int notype, parbal, inattr, parlvl;
static int resw(TWORD, int);

#define	CPP_IDENT 	2
#define	CPP_LINE 	3
#define	CPP_HASH	4

#ifdef STABS
#define	STABS_LINE(x) if (gflag && cftnsp) stabs_line(x)
#else
#define STABS_LINE(x)
#endif
#if defined(FLEX_SCANNER) && YY_FLEX_SUBMINOR_VERSION == 31
/* Hack to avoid unnecessary warnings */
FILE *yyget_in  (void);
FILE *yyget_out  (void);
int yyget_leng  (void);
char *yyget_text  (void);
void yyset_in (FILE *);
void yyset_out (FILE *);
int yyget_debug  (void);
void yyset_debug (int);
int yylex_destroy  (void);
extern int yyget_lineno (void);
extern void yyset_lineno (int);
#endif

%}

%%

"__func__"		{
				if (cftnsp == NULL)
					uerror("__func__ outside function");
				yylval.strp = cftnsp->sname; /* XXX - not C99 */
				return(C_STRING);
			}
"asm"			{ return(C_ASM); }
"auto"			{ 	return resw(AUTO, C_CLASS); }
"_Bool"			{ 	return resw(BOOL, C_TYPE); }
"break"			{ return(C_BREAK); }
"case"			{ return(C_CASE); }
"char"			{ 	return resw(CHAR, C_TYPE); }
"_Complex"		{ 	return resw(COMPLEX, C_TYPE); }
"const"			{ 	return resw(CON, C_QUALIFIER); }
"continue"		{ return(C_CONTINUE); }
"default"		{ return(C_DEFAULT); }
"do"			{ return(C_DO); }
"double"		{ 	return resw(DOUBLE, C_TYPE); }
"else"			{ return(C_ELSE); }
"enum"			{ notype=1; return(C_ENUM); }
"extern"		{ 	return resw(EXTERN, C_CLASS); }
"float"			{ 	return resw(FLOAT, C_TYPE); }
"for"			{ return(C_FOR); }
"goto"			{ notype=1; return(C_GOTO); }
"if"			{ return(C_IF); }
"_Imaginary"		{ 	return resw(IMAG, C_TYPE); }
"inline"		{ return(C_FUNSPEC); }
"int"			{ 	return resw(INT, C_TYPE); }
"long"			{ 	return resw(LONG, C_TYPE); }
"register"		{ 	return resw(REGISTER, C_CLASS); }
"restrict"		{ ; /* just ignore */ }
"return"		{ return(C_RETURN); }
"short"			{ 	return resw(SHORT, C_TYPE); }
"signed"		{ 	return resw(SIGNED, C_TYPE); }
"sizeof"		{ return(C_SIZEOF); }
"static"		{ 	return resw(STATIC, C_CLASS); }
"struct"		{ yylval.intval = STNAME; notype=1; return(C_STRUCT); }
"switch"		{ return(C_SWITCH); }
"typedef"		{ 	return resw(TYPEDEF, C_CLASS); }
"union"			{ yylval.intval = UNAME; notype=1; return(C_STRUCT); }
"unsigned"		{ 	return resw(UNSIGNED, C_TYPE); }
"void"			{ 	return resw(VOID, C_TYPE); }
"volatile"		{	return resw(VOL, C_QUALIFIER); }
"while"			{ return(C_WHILE); }

{UL}({UL}|{D})*	{ 	struct symtab *s;
			int i = 0;

			yylval.strp = addname(yytext);
#ifdef GCC_COMPAT
			if ((i = gcc_keyword(yylval.strp, &yylval.nodep)) > 0)
				return i;
#endif
			if (i == 0) {
				if (notype)
					return(C_NAME);
				s = lookup(yylval.strp, SNOCREAT);
				return s && s->sclass == TYPEDEF ?
				    notype=1, C_TYPENAME : C_NAME;
			}
		}

0[xX]{H}+{IS}?		{ yylval.nodep = cvtdig(16); return(C_ICON); }
0{D}+{IS}?		{ yylval.nodep = cvtdig(8); return(C_ICON); }
{D}+{IS}?		{ yylval.nodep = cvtdig(10); return(C_ICON); }
L'(\\.|[^\\'])+'	{ yylval.nodep = wcharcon(); return(C_ICON); }
'(\\.|[^\\'])+'		{ yylval.nodep = charcon(); return(C_ICON); }

{D}+{E}{FS}?		{ yylval.nodep = floatcon(yytext); return(C_FCON); }
{D}*"."{D}+({E})?{FS}?	{ yylval.nodep = floatcon(yytext); return(C_FCON); }
{D}+"."{D}*({E})?{FS}?	{ yylval.nodep = floatcon(yytext); return(C_FCON); }
0[xX]{H}*"."{H}+{P}{FS}? { yylval.nodep = fhexcon(yytext); return(C_FCON); }
0[xX]{H}+"."{P}{FS}?	{ yylval.nodep = fhexcon(yytext); return(C_FCON); }
0[xX]{H}+{P}{FS}?	{ yylval.nodep = fhexcon(yytext); return(C_FCON); }

L?\"(\\.|[^\\"])*\"	{ yylval.strp = yytext; return C_STRING; }

"..."			{ return(C_ELLIPSIS); }
">>="			{ yylval.intval = RSEQ; return(C_ASOP); }
"<<="			{ yylval.intval = LSEQ; return(C_ASOP); }
"+="			{ yylval.intval = PLUSEQ; return(C_ASOP); }
"-="			{ yylval.intval = MINUSEQ; return(C_ASOP); }
"*="			{ yylval.intval = MULEQ; return(C_ASOP); }
"/="			{ yylval.intval = DIVEQ; return(C_ASOP); }
"%="			{ yylval.intval = MODEQ; return(C_ASOP); }
"&="			{ yylval.intval = ANDEQ; return(C_ASOP); }
"^="			{ yylval.intval = EREQ; return(C_ASOP); }
"|="			{ yylval.intval = OREQ; return(C_ASOP); }
">>"			{ yylval.intval = RS; return(C_SHIFTOP); }
"<<"			{ yylval.intval = LS; return(C_SHIFTOP); }
"++"			{ yylval.intval = INCR; return(C_INCOP); }
"--"			{ yylval.intval = DECR; return(C_INCOP); }
"->"			{ yylval.intval = STREF; return(C_STROP); }
"&&"			{ yylval.intval = ANDAND; return(C_ANDAND); }
"||"			{ yylval.intval = OROR; return(C_OROR); }
"<="			{ yylval.intval = LE; return(C_RELOP); }
">="			{ yylval.intval = GE; return(C_RELOP); }
"=="			{ yylval.intval = EQ; return(C_EQUOP); }
"!="			{ yylval.intval = NE; return(C_EQUOP); }
";"			{ notype = 0; return(';'); }
("{"|"<%")		{ notype = 0; return('{'); }
("}"|"%>")		{ if (rpole) notype = 1; return('}'); }
","			{ if (parbal) notype = 0; return(','); }
":"			{ return(':'); }
"="			{ return('='); }
"("			{ parbal++; notype = 0; return('('); }
")"			{	parbal--;
				if (parbal==0) { notype = 0; }
				if (inattr && parlvl == parbal)
					inattr = 0;
				return(')'); }
("["|"<:")		{ return('['); }
("]"|":>")		{ return(']'); }
"."			{ yylval.intval = DOT; return(C_STROP); }
"&"			{ return('&'); }
"!"			{ yylval.intval = NOT; return(C_UNOP); }
"~"			{ yylval.intval = COMPL; return(C_UNOP); }
"-"			{ return('-'); }
"+"			{ return('+'); }
"*"			{ if (parbal && notype == 0) notype = 1; return('*'); }
"/"			{ yylval.intval = DIV; return(C_DIVOP); }
"%"			{ yylval.intval = MOD; return(C_DIVOP); }
"<"			{ yylval.intval = LT; return(C_RELOP); }
">"			{ yylval.intval = GT; return(C_RELOP); }
"^"			{ return('^'); }
"|"			{ return('|'); }
"?"			{ return('?'); }
^#pragma[ \t].*		{ pragma(); }
^#ident[ \t].*		{ control(CPP_IDENT); }
^#line[ \t].*		{ control(CPP_LINE); }
^#.*			{ control(CPP_HASH); }

[ \t\v\f]		{ }
"\n"			{ ++lineno; STABS_LINE(lineno); }
.			{ /* ignore bad characters */ }

%%

int lineno;
char *ftitle = "<stdin>";

int
yywrap(void)
{
	if (0) unput(0); /* quiet gcc */
	return(1);
}

int
resw(TWORD t, int rv)
{
	if (inattr) {
		yylval.strp = addname(yytext);
		return C_NAME;
	}

	switch (rv) {
	case C_CLASS:
		yylval.nodep = block(CLASS, NIL, NIL, t, 0, 0);
		return rv;

	case C_QUALIFIER:
		yylval.nodep = block(QUALIFIER, NIL, NIL, 0, 0, 0);
		yylval.nodep->n_qual = t;
		return rv;

	case C_TYPE:
		yylval.nodep = mkty(t, 0, MKAP(t));
		notype=1;
		return(rv);

	default:
		cerror("resw");
	}
	return 0;
}

#ifndef SOFTFLOAT

static long double
typround(long double dc, char *e, TWORD *tw)
{
	int im = 0;

	*tw = DOUBLE;
	for (; *e; e++) {
		switch (*e) {
		case 'f':
		case 'F':
			*tw = FLOAT;
			dc = (float)dc;
			break;
		case 'l':
		case 'L':
			*tw = LDOUBLE;
			break;
		case 'i':
		case 'I':
			im = 1;
			break;
		}
	}
	if (*tw == DOUBLE)
		dc = (double)dc;
#ifndef NO_COMPLEX
	if (im)
		*tw += (FIMAG-FLOAT);
#endif
	return dc;
}

/*
 * XXX floatcon() and fhexcon() should be in support libraries for
 * the target floating point.
 */
static NODE *
f2(char *str)
{
	TWORD tw;
	NODE *p;
	long double dc;
	char *eptr;

#ifdef HAVE_STRTOLD
	dc = strtold(str, &eptr); /* XXX - avoid strtod() */
#else
	dc = strtod(str, &eptr); /* XXX - avoid strtod() */
#endif
	dc = typround(dc, eptr, &tw);
	p = block(FCON, NIL, NIL, tw, 0, MKAP(tw));
	p->n_dcon = dc;
	return p;
}

NODE *
floatcon(char *s)
{
	return f2(s);
}

static int
h2n(int ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return ch - 'A' + 10;
	
}

NODE *
fhexcon(char *c)
{
	TWORD tw;
	char *ep;
	long double d;
	int i, ed;
	NODE *p;

	d = 0.0;
	ed = 0;
	c+= 2; /* skip 0x */
#define FSET(n) { d *= 2; if (i & n) d += 1.0; }
	for (; *c != '.' && *c != 'p' && *c != 'P'; c++) {
		i = h2n(*c);
		FSET(8); FSET(4); FSET(2); FSET(1);
	}
	if (*c != '.' && *c != 'p' && *c != 'P')
		cerror("fhexcon");
	if (*c == '.') {
		c++;
		for (; *c != 'p' && *c != 'P'; c++) {
			i = h2n(*c);
			FSET(8); FSET(4); FSET(2); FSET(1);
			ed -= 4;
		}
	}
	if (*c != 'P' && *c != 'p')
		cerror("fhexcon2");
	c++;
	ed += strtol(c, &ep, 10);

	/* avoid looping in vain. Idea from Fred J. Tydeman */
	if (ed > 32769) ed = 32769;
	if (ed < -32769) ed = -32769;

	while (ed > 0)
		d *= 2, ed--;
	while (ed < 0)
		d /= 2, ed++;
	d = typround(d, ep, &tw);
	p = block(FCON, NIL, NIL, tw, 0, MKAP(tw));
	p->n_dcon = d;
	return p;
}
#endif

unsigned int
esccon(char **sptr)
{
	char *wr = *sptr;
	char *owr;
	char c;
	unsigned int val;
	int wsz = 4, esccon_warn = 1;

	switch (*wr++) {
	case 'a': val = '\a'; break;
	case 'b': val = '\b'; break;
	case 'f': val = '\f'; break;
	case 'n': val = '\n'; break;
	case 'r': val = '\r'; break;
	case 't': val = '\t'; break;
	case 'v': val = '\v'; break;
	case '\"': val = '\"'; break;
	case 'x': val = strtoul(wr, &wr, 16); break;
	/* ISO/IEC 9099:1999 (E) 6.4.3 */
	case 'U'|(char)0x80:
		esccon_warn = 0;
		/* FALLTHROUGH */
	case 'U':
		wsz = 8;
		/* FALLTHROUGH */
	case 'u':
		owr = wr;
		while (wr < (owr + wsz))
			if (*wr == '\0')
				break;
			else
				++wr;
		if (wr != (owr + wsz)) {
			/* incomplete */
			val = strtoul(owr, &wr, 16);
		} else {
			c = owr[wsz];
			owr[wsz] = '\0'; /* prevent it from reading too much */
			val = strtoul(owr, &wr, 16);
			owr[wsz] = c;
		}
		if (wr != (owr + wsz))
			werror("incomplete universal character name");
		if (wsz == 4)
			val &= 0xFFFF;
		if (esccon_warn && ((val >= 0xD800 && val <= 0xDFFF) ||
		    (val < 0xA0 && val != 0x24 && val != 0x40 && val != 0x60)))
			werror("invalid universal character name %04X", val);
		break;
	case '0': case '1': case '2': case '3': case '4': 
	case '5': case '6': case '7':
		val = wr[-1] - '0';
		if (*wr >= '0' && *wr <= '7') {
			val = (val << 3) + (*wr++ - '0');
			if (*wr >= '0' && *wr <= '7')
				val = (val << 3) + (*wr++ - '0');
		}
		break;
	default: val = wr[-1];
	}
	*sptr = wr;
	return val;
}

NODE *
cvtdig(int radix)
{
	NODE *p;
	TWORD ntype;
	unsigned long long v;
	char *ch = yytext;
	int n, numl, numu;

	if (radix == 16)
		ch += 2; /* Skip 0x */
	
	v = 0;
	while ((*ch >= '0' && *ch <= '9') || (*ch >= 'a' && *ch <= 'f') ||
	    (*ch >= 'A' && *ch <= 'F')) {
		v *= radix;
		n = *ch;
		n = (n <= '9' ? n - '0' : (n > 'F' ? n - 'a' : n - 'A') + 10);
		ch++;
		v += n;
	}
	/* Parse trailing chars */
	ntype = INT;
	numl = numu = 0;
	for (n = 0; n < 3; n++) {
		if (*ch == 0)
			break;
		if ((*ch == 'l' || *ch == 'L') && numl < 2)
			ntype+=2, numl++;
		else if ((*ch == 'u' || *ch == 'U') && numu < 1)
			ntype = ENUNSIGN(ntype), numu++;
		else
			break;
		ch++;
	}
	if (*ch)
		uerror("constant has too many '%c'", *ch);

	if (ntype == INT) {
		/* v contains a number. Get type correct */
		if (v > MAX_LONGLONG && radix != 10)
			ntype = ULONGLONG;
		else if (v > MAX_ULONG)
			ntype = LONGLONG;
		else if (v > MAX_LONG && radix != 10)
			ntype = ULONG;
		else if (v > MAX_UNSIGNED)
			ntype = LONG;
		else if (v > MAX_INT && radix != 10)
			ntype = UNSIGNED;
	}
	ntype = ctype(ntype);
	p = xbcon(v, NULL, ntype);
	ASGLVAL(p->n_slval, v);

	return p;
}

/*
 * Convert a character constant to an integer.
 */
NODE *
charcon(void)
{
	int lastcon = 0;
	int val, i = 0;
	char *pp = yytext;

	if (*pp == 'L')
		pp++;
	pp++;
	while (*pp != '\'') {
		if (*pp++ == '\\') {
			val = esccon(&pp);
		} else
			val = pp[-1];
		makecc(val, i);
		i++;
	}

	if (i == 0)
		uerror("empty character constant");
	if (i > (SZINT/SZCHAR) || (i>1))
		werror("too many characters in character constant");
	return bcon(lastcon);
}

NODE *
wcharcon(void)
{
	unsigned int lastcon = 0;
	unsigned int val, i = 0;
	char *pp = yytext;

	if (*pp == 'L')
		pp++;
	pp++;
	while (*pp != '\'') {
		if (*pp++ == '\\') {
			val = esccon(&pp);
		} else
			val = pp[-1];
#if WCHAR_SIZE == 2
		lastcon = (lastcon << 16) | (val & 0xFFFF);
#else
		lastcon = val;
#endif
		i++;
	}

	if (i == 0)
		uerror("empty wide character constant");
	if (i > 1)
		werror("too many characters in wide character constant");
	return xbcon(lastcon, NULL, ctype(UNSIGNED));
}

void
control(int t)
{
	char *wr = yytext;
	char *eptr;
	int val;

	wr++;	/* Skip initial '#' */
	switch (t) {
	case CPP_IDENT:
		return;	/* Just skip these for now. */

	case CPP_LINE:
		wr += 4;
		/* FALLTHROUGH */
	case CPP_HASH:
		val = strtol(wr, &eptr, 10);
		if (wr == eptr)	/* Illegal string */
			goto bad;
		wr = eptr;
		lineno = val - 1;
		while (*wr && *wr != '\"')
			wr++;
		if (*wr == 0)
			return;
		if (*wr++ != '\"')
			goto bad;
		eptr = wr;
		while (*wr && *wr != '\"')
			wr++;
		if (*wr != '\"')
			goto bad;
		*wr = 0;
		ftitle = addstring(eptr);
#ifdef STABS
		if (gflag)
			stabs_file(ftitle);
#endif
	}
	return;
bad:
	werror("%s: illegal control", yytext);
}

int pragma_allpacked;
int pragma_packed, pragma_aligned;
char *pragma_renamed;

static int
pragmas_weak(char *str)
{
	struct symtab *sp;
	char *s1, *s2;

	if ((s1 = pragtok(NULL)) == NULL)
		return 1;
	if ((s2 = pragtok(NULL)) == NULL) {
		sp = lookup(addname(s1), SNORMAL);
		sp->sap = attr_add(sp->sap, gcc_attr_parse(bdty(NAME, "weak")));
	} else if (*s2 == '=') {
		if ((s2 = pragtok(NULL)) == NULL)
			return 1;
		sp = lookup(addname(s2), SNORMAL);
		sp->sap = attr_add(sp->sap, gcc_attr_parse(bdty(CALL,
		    bdty(NAME, "aliasweak"), bdty(STRING, s1, 0))));
	} else
		return 1;
	return 0;
}

char *pragstore;

/* trivial tokenizer for pragmas */
#define ps pragstore
char *
pragtok(char *sin)
{
	static char ss[2];
	char *rv;

	if (sin)
		ps = sin;

	for (; isspace((int)*ps); ps++)
		;
	if (*ps == 0)
		return NULL;
	for (rv = ps; isalpha((int)*ps) || isdigit((int)*ps) || *ps == '_'; ps++)
		;
	ss[0] = *ps;
	if (rv == ps) {
		rv = ss, ps++;
	} else {
		*ps = 0;
		rv = tmpstrdup(rv);
		*ps = ss[0];
	}
	return rv;
}

/* return 1 on error */
int
eat(int ch)
{
	char *s = pragtok(0);
	return (s == 0 || *s != ch);
}

static int
pragmas_alpack(char *t)
{
	char *s;
	int ap;

	ap = (s = pragtok(0)) ? atoi(s) : 1;
	if (strcmp(t, "packed") == 0)
		pragma_packed = ap;
	else
		pragma_aligned = ap;
	return 0;
}


/*
 * Packing control.
 * still missing push/pop.
 */
static int
pragmas_pack(char *t)
{
	char *s;

	if (eat('('))
		return 1;
	s = pragtok(0);
	if (*s == ')')
		return pragma_allpacked = 0;

	if (*s < '0' || *s > '9') /* no number */
		return 1;
	pragma_allpacked = atoi(s);
	return eat(')');
}

static int
pragmas_renamed(char *t)
{
	char *f = pragtok(0);

	if (f == 0)
		return 1;
	pragma_renamed = newstring(f, strlen(f));
	return 0;
}

static int
pragmas_stdc(char *t)
{
	return 0; /* Just ignore */
}

struct pragmas {
	char *name;
	int (*fun)(char *);
} pragmas[] = {
	{ "pack", pragmas_pack },
	{ "packed", pragmas_alpack },
	{ "aligned", pragmas_alpack },
	{ "rename", pragmas_renamed },
#ifdef GCC_COMPAT
	{ "GCC", pragmas_gcc },
#endif
	{ "STDC", pragmas_stdc },
	{ "weak", pragmas_weak },
	{ "ident", NULL },
	{ 0 },
};
/*
 * got a full pragma line.  Split it up here.
 */
static void
pragma()
{
	struct pragmas *p;
	char *t, *pt;

	if ((t = pragtok(&yytext[7])) != NULL) {
		pt = ps;
		for (p = pragmas; p->name; p++) {
			if (strcmp(t, p->name) == 0) {
				if (p->fun && (*p->fun)(t))
					uerror("bad argument to #pragma");
				return;
			}
		}
		ps = pt;
		if (mypragma(t))
			return;
	}
	warner(Wunknown_pragmas, t, ps);
}

void
cunput(char c)
{
	unput(c);
}
