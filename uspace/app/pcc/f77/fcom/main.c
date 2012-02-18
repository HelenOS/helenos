/*	$Id: main.c,v 1.14 2009/02/09 15:59:48 ragge Exp $	*/
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
char xxxvers[] = "\nFORTRAN 77 PASS 1, VERSION 1.16,  3 NOVEMBER 1978\n";

#include <unistd.h>

#include "defines.h"
#include "defs.h"

void mkdope(void);

int f2debug, e2debug, odebug, rdebug, b2debug, c2debug, t2debug;
int s2debug, udebug, x2debug, nflag, kflag, g2debug;
int xdeljumps, xtemps, xssaflag, xdce;

int mflag, tflag;

#if 1 /* RAGGE */
FILE *initfile, *sortfile;
int dodata(char *file);
LOCAL int nch   = 0;
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: fcom [qw:UuOdpC1I:Z:]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch;
	int k, retcode;

	infile = stdin;
	diagfile = stderr;
#if 1 /* RAGGE */
	char file[] = "/tmp/initfile.XXXXXX";
	char buf[100];
	close(mkstemp(file));
	sprintf(buf, "sort > %s", file);
	initfile = popen(buf, "w");
#endif


#define DONE(c)	{ retcode = c; goto finis; }

	while ((ch = getopt(argc, argv, "qw:UuOdpC1I:Z:X:")) != -1)
		switch (ch) {
		case 'q':
			quietflag = YES;
			break;

		case 'w':
			if(optarg[0]=='6' && optarg[1]=='6') {
				ftn66flag = YES;
			} else
				nowarnflag = YES;
			break;

		case 'U':
			shiftcase = NO;
			break;

		case 'u':
			undeftype = YES;
			break;

		case 'O':
			optimflag = YES;
#ifdef notyet
			xdeljumps = 1;
			xtemps = 1;
#endif
			break;

		case 'd':
			debugflag = YES;
			break;

		case 'p':
			profileflag = YES;
			break;

		case 'C':
			checksubs = YES;
			break;

		case '1':
			onetripflag = YES;
			break;

		case 'I':
			if(*optarg == '2')
				tyint = TYSHORT;
			else if(*optarg == '4') {
				shortsubs = NO;
				tyint = TYLONG;
			} else if(*optarg == 's')
				shortsubs = YES;
			else
				fatal1("invalid flag -I%c\n", *optarg);
			tylogical = tyint;
			break;

		case 'Z':
			while (*optarg)
				switch (*optarg++) {
				case 'f': /* instruction matching */
					++f2debug;
					break;
				case 'e': /* print tree upon pass2 enter */
					++e2debug;
					break;
				case 'o': ++odebug; break;
				case 'r': /* register alloc/graph coloring */
					++rdebug;
					break;
				case 'b': /* basic block and SSA building */
					++b2debug;
					break;
				case 'c': /* code printout */
					++c2debug;
					break;
				case 't': ++t2debug; break;
				case 's': /* shape matching */
					++s2debug;
					break;
				case 'u': /* Sethi-Ullman debugging */
					++udebug;
					break;
				case 'x': ++x2debug; break;
				case 'g': ++g2debug; break;
				case 'n': ++nflag; break;
				default:
					fprintf(stderr, "unknown Z flag '%c'\n",
					    optarg[-1]);
					exit(1);
				}
			break;

		case 'X':
			while (*optarg)
				switch (*optarg++) {
				case 't': /* tree debugging */
					tflag++;
					break;
				case 'm': /* memory allocation */
					++mflag;
					break;
				default:
					usage();
				}
			break;

		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	mkdope();
	initkey();
	if (argc > 0) {
		if (inilex(copys(argv[0])))
			DONE(1);
		if (!quietflag)
			fprintf(diagfile, "%s:\n", argv[0]);
		if (argc != 1)
			if (freopen(argv[1], "w", stdout) == NULL) {
				fprintf(stderr, "open output file '%s':",
				    argv[1]);
				perror(NULL);
				exit(1);
			}
	} else {
		inilex(copys(""));
	}
	fileinit();
	procinit();
	if((k = yyparse())) {
		fprintf(diagfile, "Bad parse, return code %d\n", k);
		DONE(1);
	}
	if(nerr > 0)
		DONE(1);
	if(parstate != OUTSIDE) {
		warn("missing END statement");
		endproc();
	}
	doext();
	preven(ALIDOUBLE);
	prtail();
	puteof();
	DONE(0);


finis:
	pclose(initfile);
	retcode |= dodata(file);
	unlink(file);
	done(retcode);
	return(retcode);
}

#define USEINIT ".data\t2"
#define LABELFMT "%s:\n"

static void
prcha(FILEP fp, int *s)
{

fprintf(fp, ".byte 0%o,0%o\n", s[0], s[1]);
}

static void
prskip(FILEP fp, ftnint k)
{
fprintf(fp, "\t.space\t%ld\n", k);
}


static void
prch(int c)
{
static int buff[SZSHORT];

buff[nch++] = c;
if(nch == SZSHORT)
        {
        prcha(stdout, buff);
        nch = 0;
        }
}


static int
rdname(int *vargroupp, char *name)
{
register int i, c;

if( (c = getc(sortfile)) == EOF)
        return(NO);
*vargroupp = c - '0';

for(i = 0 ; i<XL ; ++i)
        {
        if( (c = getc(sortfile)) == EOF)
                return(NO);
        if(c != ' ')
                *name++ = c;
        }
*name = '\0';
return(YES);
}

static int
rdlong(ftnint *n)
{
register int c;

for(c = getc(sortfile) ; c!=EOF && isspace(c) ; c = getc(sortfile) );
        ;
if(c == EOF)
        return(NO);

for(*n = 0 ; isdigit(c) ; c = getc(sortfile) )
        *n = 10* (*n) + c - '0';
return(YES);
}

static void
prspace(ftnint n)
{
register ftnint m;

while(nch>0 && n>0)
        {
        --n;
        prch(0);
        }
m = SZSHORT * (n/SZSHORT);
if(m > 0)
        prskip(stdout, m);
for(n -= m ; n>0 ; --n)
        prch(0);
}

static ftnint
doeven(ftnint tot, int align)
{
ftnint new;
new = roundup(tot, align);
prspace(new - tot);
return(new);
}


int
dodata(char *file)
{
	char varname[XL+1], ovarname[XL+1];
	flag erred;
	ftnint offset, vlen, type;
	register ftnint ooffset, ovlen;
	ftnint vchar;
	int size, align;
	int vargroup;
	ftnint totlen;

	erred = NO;
	ovarname[0] = '\0';
	ooffset = 0;
	ovlen = 0;
	totlen = 0;
	nch = 0;

	if( (sortfile = fopen(file, "r")) == NULL)
		fatal1(file);
#if 0
	pruse(asmfile, USEINIT);
#else
	printf("\t%s\n", USEINIT);
#endif
	while (rdname(&vargroup, varname) && rdlong(&offset) &&
	    rdlong(&vlen) && rdlong(&type) ) {
		size = typesize[type];
		if( strcmp(varname, ovarname) ) {
			prspace(ovlen-ooffset);
			strcpy(ovarname, varname);
			ooffset = 0;
			totlen += ovlen;
			ovlen = vlen;
			if(vargroup == 0)
				align = (type==TYCHAR ? SZLONG :
				    typealign[type]);
			else
				align = ALIDOUBLE;
			totlen = doeven(totlen, align);
			if(vargroup == 2) {
#if 0
				prcomblock(asmfile, varname);
#else
				printf(LABELFMT, varname);
#endif
			} else {
#if 0
				fprintf(asmfile, LABELFMT, varname);
#else
				printf(LABELFMT, varname);
#endif
			}
		}
		if(offset < ooffset) {
			erred = YES;
			err("overlapping initializations");
		}
		if(offset > ooffset) {
			prspace(offset-ooffset);
			ooffset = offset;
		}
		if(type == TYCHAR) {
			if( ! rdlong(&vchar) )
				fatal("bad intermediate file format");
			prch( (int) vchar );
		} else {
			putc('\t', stdout);
			while	( putc( getc(sortfile), stdout)  != '\n')
				;
		}
		if( (ooffset += size) > ovlen) {
			erred = YES;
			err("initialization out of bounds");
		}
	}

	prspace(ovlen-ooffset);
	totlen = doeven(totlen+ovlen, (ALIDOUBLE>SZLONG ? ALIDOUBLE : SZLONG) );
	return(erred);
}

void
done(k)
int k;
{
static int recurs	= NO;

if(recurs == NO)
	{
	recurs = YES;
	}
exit(k);
}
