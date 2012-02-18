/*	$Id: f77.c,v 1.21 2008/12/27 00:36:39 sgk Exp $	*/
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

char xxxvers[] = "FORTRAN 77 DRIVER, VERSION 1.11,   28 JULY 1978\n";

#include <sys/wait.h>

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "ccconfig.h"

typedef FILE *FILEP;
typedef int flag;
#define	YES 1
#define NO 0

FILEP diagfile;

static int pid;
static int sigivalue	= 0;
static int sigqvalue	= 0;

#ifndef FCOM
#define	FCOM		"fcom"
#endif

#ifndef ASSEMBLER
#define ASSEMBLER       "as"
#endif

#ifndef LINKER
#define LINKER          "ld"
#endif

static char *fcom	= LIBEXECDIR "/" FCOM ;
static char *asmname	= ASSEMBLER ;
static char *ldname	= LINKER ;
static char *startfiles[] = STARTFILES;
static char *endfiles[] = ENDFILES;
static char *dynlinker[] = DYNLINKER;
static char *crt0file = CRT0FILE;
static char *macroname	= "m4";
static char *shellname	= "/bin/sh";
static char *aoutname	= "a.out" ;
static char *libdir	= LIBDIR ;
static char *liblist[] = F77LIBLIST;

static char *infname;
static char asmfname[15];
static char prepfname[15];

#define MAXARGS 100
int ffmax;
static char *ffary[MAXARGS];
static char eflags[30]	= "";
static char rflags[30]	= "";
static char lflag[3]	= "-x";
static char *eflagp	= eflags;
static char *rflagp	= rflags;
static char **loadargs;
static char **loadp;
static int oflag;

static flag loadflag	= YES;
static flag saveasmflag	= NO;
static flag profileflag	= NO;
static flag optimflag	= NO;
static flag debugflag	= NO;
static flag verbose	= NO;
static flag fortonly	= NO;
static flag macroflag	= NO;

static char *setdoto(char *), *lastchar(char *), *lastfield(char *);
static void intrupt(int);
static void enbint(void (*)(int));
static void crfnames(void);
static void fatal1(char *, ...);
static void done(int), texec(char *, char **);
static char *copyn(int, char *);
static int dotchar(char *), unreadable(char *), sys(char *), dofort(char *);
static int nodup(char *);
static int await(int);
static void rmf(char *), doload(char *[], char *[]), doasm(char *);
static int callsys(char *, char **);
static void errorx(char *, ...);

static void
addarg(char **ary, int *num, char *arg)
{
	ary[(*num)++] = arg;
	if ((*num) == MAXARGS) {
		fprintf(stderr, "argument array too small\n");
		exit(1);
	}
}

int
main(int argc, char **argv)
{
	int i, c, status;
	char *s;
	char fortfile[20], *t;
	char buff[100];

	diagfile = stderr;

	sigivalue = (int) signal(SIGINT, SIG_IGN) & 01;
	sigqvalue = (int) signal(SIGQUIT, SIG_IGN) & 01;
	enbint(intrupt);

	pid = getpid();
	crfnames();

	loadargs = (char **)calloc(1, (argc + 20) * sizeof(*loadargs));
	if (!loadargs)
		fatal1("out of memory");
	loadp = loadargs;

	--argc;
	++argv;

	while(argc>0 && argv[0][0]=='-' && argv[0][1]!='\0') {
		for(s = argv[0]+1 ; *s ; ++s)
			switch(*s) {
			case 'T':  /* use special passes */
				switch(*++s) {
				case '1':
					fcom = s+1; goto endfor;
				case 'a':
					asmname = s+1; goto endfor;
				case 'l':
					ldname = s+1; goto endfor;
				case 'm':
					macroname = s+1; goto endfor;
				default:
					fatal1("bad option -T%c", *s);
				}
				break;

			case 'w': /* F66 warn or no warn */
				addarg(ffary, &ffmax, s-1);
				break;

			case 'q':
				/*
				 * Suppress printing of procedure names during
				 * compilation.
				 */
				addarg(ffary, &ffmax, s-1);
				break;

			copyfflag:
			case 'u':
			case 'U':
			case 'M':
			case '1':
			case 'C':
				addarg(ffary, &ffmax, s-1);
				break;

			case 'O':
				optimflag = YES;
				addarg(ffary, &ffmax, s-1);
				break;

			case 'm':
				if(s[1] == '4')
					++s;
				macroflag = YES;
				break;

			case 'S':
				saveasmflag = YES;

			case 'c':
				loadflag = NO;
				break;

			case 'v':
				verbose = YES;
				break;

			case 'd':
				debugflag = YES;
				goto copyfflag;

			case 'p':
				profileflag = YES;
				goto copyfflag;

			case 'o':
				if(!strcmp(s, "onetrip")) {
					addarg(ffary, &ffmax, s-1);
					goto endfor;
				}
				oflag = 1;
				aoutname = *++argv;
				--argc;
				break;

			case 'F':
				fortonly = YES;
				loadflag = NO;
				break;

			case 'I':
				if(s[1]=='2' || s[1]=='4' || s[1]=='s')
					goto copyfflag;
				fprintf(diagfile, "invalid flag -I%c\n", s[1]);
				done(1);

			case 'l':	/* letter ell--library */
				s[-1] = '-';
				*loadp++ = s-1;
				goto endfor;

			case 'E':	/* EFL flag argument */
				while(( *eflagp++ = *++s))
					;
				*eflagp++ = ' ';
				goto endfor;
			case 'R':
				while(( *rflagp++ = *++s ))
					;
				*rflagp++ = ' ';
				goto endfor;
			default:
				lflag[1] = *s;
				*loadp++ = copyn(strlen(lflag), lflag);
				break;
			}
endfor:
	--argc;
	++argv;
	}

	if (verbose)
		fprintf(stderr, xxxvers);

	if (argc == 0)
		errorx("No input files");

#ifdef mach_pdp11
	if(nofloating)
		*loadp++ = (profileflag ? NOFLPROF : NOFLFOOT);
	else
#endif

	for(i = 0 ; i<argc ; ++i)
		switch(c =  dotchar(infname = argv[i]) ) {
		case 'r':	/* Ratfor file */
		case 'e':	/* EFL file */
			if( unreadable(argv[i]) )
				break;
			s = fortfile;
			t = lastfield(argv[i]);
			while(( *s++ = *t++))
				;
			s[-2] = 'f';

			if(macroflag) {
				sprintf(buff, "%s %s >%s", macroname, infname, prepfname);
				if(sys(buff)) {
					rmf(prepfname);
					break;
				}
				infname = prepfname;
			}

			if(c == 'e')
				sprintf(buff, "efl %s %s >%s", eflags, infname, fortfile);
			else
				sprintf(buff, "ratfor %s %s >%s", rflags, infname, fortfile);
			status = sys(buff);
			if(macroflag)
				rmf(infname);
			if(status) {
				loadflag = NO;
				rmf(fortfile);
				break;
			}

			if( ! fortonly ) {
				infname = argv[i] = lastfield(argv[i]);
				*lastchar(infname) = 'f';
	
				if( dofort(argv[i]) )
					loadflag = NO;
				else	{
					if( nodup(t = setdoto(argv[i])) )
						*loadp++ = t;
					rmf(fortfile);
				}
			}
			break;

		case 'f':	/* Fortran file */
		case 'F':
			if( unreadable(argv[i]) )
				break;
			if( dofort(argv[i]) )
				loadflag = NO;
			else if( nodup(t=setdoto(argv[i])) )
				*loadp++ = t;
			break;

		case 'c':	/* C file */
		case 's':	/* Assembler file */
			if( unreadable(argv[i]) )
				break;
			fprintf(diagfile, "%s:\n", argv[i]);
			sprintf(buff, "cc -c %s", argv[i] );
			if( sys(buff) )
				loadflag = NO;
			else
				if( nodup(t = setdoto(argv[i])) )
					*loadp++ = t;
			break;

		case 'o':
			if( nodup(argv[i]) )
				*loadp++ = argv[i];
			break;

		default:
			if( ! strcmp(argv[i], "-o") )
				aoutname = argv[++i];
			else
				*loadp++ = argv[i];
			break;
		}

	if(loadflag)
		doload(loadargs, loadp);
	done(0);
	return 0;
}

#define	ADD(x)	addarg(params, &nparms, (x))

static int
dofort(char *s)
{
	int nparms, i;
	char *params[MAXARGS];

	nparms = 0;
	ADD(FCOM);
	for (i = 0; i < ffmax; i++)
		ADD(ffary[i]);
	ADD(s);
	ADD(asmfname);
	ADD(NULL);

	infname = s;
	if (callsys(fcom, params))
		errorx("Error.  No assembly.");
	doasm(s);

	if (saveasmflag == NO)
		rmf(asmfname);
	return(0);
}


static void
doasm(char *s)
{
	char *obj;
	char *params[MAXARGS];
	int nparms;

	if (oflag && loadflag == NO)
		obj = aoutname;
	else
		obj = setdoto(s);

	nparms = 0;
	ADD(asmname);
	ADD("-o");
	ADD(obj);
	ADD(asmfname);
	ADD(NULL);

	if (callsys(asmname, params))
		fatal1("assembler error");
	if(verbose)
		fprintf(diagfile, "\n");
}


static void
doload(char *v0[], char *v[])
{
	int nparms, i;
	char *params[MAXARGS];
	char **p;

	nparms = 0;
	ADD(ldname);
	ADD("-X");
	ADD("-d");
	for (i = 0; dynlinker[i]; i++)
		ADD(dynlinker[i]);
	ADD("-o");
	ADD(aoutname);
	ADD(crt0file);
	for (i = 0; startfiles[i]; i++)
		ADD(startfiles[i]);
	*v = NULL;
	for(p = v0; *p ; p++)
		ADD(*p);
	if (libdir)
		ADD(libdir);
	for(p = liblist ; *p ; p++)
		ADD(*p);
	for (i = 0; endfiles[i]; i++)
		ADD(endfiles[i]);
	ADD(NULL);

	if (callsys(ldname, params))
		fatal1("couldn't load %s", ldname);

	if(verbose)
		fprintf(diagfile, "\n");
}

/* Process control and Shell-simulating routines */

/*
 * Execute f[] with parameter array v[].
 * Copied from cc.
 */
static int
callsys(char f[], char *v[])
{
	int t, status = 0;
	pid_t p;
	char *s;

	if (debugflag || verbose) {
		fprintf(stderr, "%s ", f);
		for (t = 1; v[t]; t++)
			fprintf(stderr, "%s ", v[t]);
		fprintf(stderr, "\n");
	}

	if ((p = fork()) == 0) {
#ifdef notyet
		if (Bflag) {
			size_t len = strlen(Bflag) + 8;
			char *a = malloc(len);
			if (a == NULL) {
				error("callsys: malloc failed");
				exit(1);
			}
			if ((s = strrchr(f, '/'))) {
				strlcpy(a, Bflag, len);
				strlcat(a, s, len);
				execv(a, v);
			}
		}
#endif
		execvp(f, v);
		if ((s = strrchr(f, '/')))
			execvp(s+1, v);
		fprintf(stderr, "Can't find %s\n", f);
		_exit(100);
	} else {
		if (p == -1) {
			printf("Try again\n");
			return(100);
		}
	}
	while (waitpid(p, &status, 0) == -1 && errno == EINTR)
		;
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		done(1);
	fatal1("Fatal error in %s", f);
	return 0; /* XXX */
}


static int
sys(char *str)
{
	char *s, *t;
	char *argv[100], path[100];
	char *inname, *outname;
	int append = 0;
	int wait_pid;
	int argc;


	if(debugflag)
		fprintf(diagfile, "%s\n", str);
	inname  = NULL;
	outname = NULL;
	argv[0] = shellname;
	argc = 1;

	t = str;
	while( isspace((int)*t) )
		++t;
	while(*t) {
		if(*t == '<')
			inname = t+1;
		else if(*t == '>') {
			if(t[1] == '>') {
				append = YES;
				outname = t+2;
			} else	{
				append = NO;
				outname = t+1;
			}
		} else
			argv[argc++] = t;
		while( !isspace((int)*t) && *t!='\0' )
			++t;
		if(*t) {
			*t++ = '\0';
			while( isspace((int)*t) )
				++t;
		}
	}

	if(argc == 1)   /* no command */
		return(-1);
	argv[argc] = 0;

	s = path;
	t = "/usr/bin/";
	while(*t)
		*s++ = *t++;
	for(t = argv[1] ; (*s++ = *t++) ; )
		;
	if((wait_pid = fork()) == 0) {
		if(inname)
			freopen(inname, "r", stdin);
		if(outname)
			freopen(outname, (append ? "a" : "w"), stdout);
		enbint(SIG_DFL);

		texec(path+9, argv);  /* command */
		texec(path+4, argv);  /*  /bin/command */
		texec(path  , argv);  /* /usr/bin/command */

		fatal1("Cannot load %s",path+9);
	}

	return( await(wait_pid) );
}

/* modified version from the Shell */
static void
texec(char *f, char **av)
{

	execv(f, av+1);

	if (errno==ENOEXEC) {
		av[1] = f;
		execv(shellname, av);
		fatal1("No shell!");
	}
	if (errno==ENOMEM)
		fatal1("%s: too large", f);
}

/*
 * Cleanup and exit with value k.
 */
static void
done(int k)
{
	static int recurs	= NO;

	if(recurs == NO) {
		recurs = YES;
		if (saveasmflag == NO)
			rmf(asmfname);
	}
	exit(k);
}


static void
enbint(void (*k)(int))
{
if(sigivalue == 0)
	signal(SIGINT,k);
if(sigqvalue == 0)
	signal(SIGQUIT,k);
}



static void
intrupt(int a)
{
done(2);
}


static int
await(int wait_pid)
{
int w, status;

enbint(SIG_IGN);
while ( (w = wait(&status)) != wait_pid)
	if(w == -1)
		fatal1("bad wait code");
enbint(intrupt);
if(status & 0377)
	{
	if(status != SIGINT)
		fprintf(diagfile, "Termination code %d", status);
	done(3);
	}
return(status>>8);
}

/* File Name and File Manipulation Routines */

static int
unreadable(char *s)
{
	FILE *fp;

	if((fp = fopen(s, "r"))) {
		fclose(fp);
		return(NO);
	} else {
		fprintf(diagfile, "Error: Cannot read file %s\n", s);
		loadflag = NO;
		return(YES);
	}
}


static void
crfnames(void)
{
	sprintf(asmfname,  "fort%d.%s", pid, "s");
	sprintf(prepfname, "fort%d.%s", pid, "p");
}



static void
rmf(char *fn)
{
if(!debugflag && fn!=NULL && *fn!='\0')
	unlink(fn);
}


static int
dotchar(char *s)
{
for( ; *s ; ++s)
	if(s[0]=='.' && s[1]!='\0' && s[2]=='\0')
		return( s[1] );
return(NO);
}


static char *
lastfield(char *s)
{
char *t;
for(t = s; *s ; ++s)
	if(*s == '/')
		t = s+1;
return(t);
}


static char *
lastchar(char *s)
{
while(*s)
	++s;
return(s-1);
}


static char *
setdoto(char *s)
{
*lastchar(s) = 'o';
return( lastfield(s) );
}


static char *
copyn(int n, char *s)
{
	char *p, *q;

	p = q = (char *)calloc(1, (unsigned) n + 1);
	if (!p)
		fatal1("out of memory");

	while(n-- > 0)
		*q++ = *s++;
	return (p);
}


static int
nodup(char *s)
{
char **p;

for(p = loadargs ; p < loadp ; ++p)
	if( !strcmp(*p, s) )
		return(NO);

return(YES);
}


static void
errorx(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(diagfile, fmt, ap);
	fprintf(diagfile, "\n");
	va_end(ap);

	if (debugflag)
		abort();
	done(1);
}


static void
fatal1(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(diagfile, "Compiler error in file %s: ", infname);
	vfprintf(diagfile, fmt, ap);
	fprintf(diagfile, "\n");
	va_end(ap);

	if (debugflag)
		abort();
	done(1);
}
