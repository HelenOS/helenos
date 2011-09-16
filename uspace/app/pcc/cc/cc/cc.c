/*	$Id: cc.c,v 1.170.2.1 2011/03/15 19:25:36 ragge Exp $	*/
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

/*
 * Front-end to the C compiler.
 *
 * Brief description of its syntax:
 * - Files that end with .c are passed via cpp->ccom->as->ld
 * - Files that end with .i are passed via ccom->as->ld
 * - Files that end with .s are passed as->ld
 * - Files that end with .o are passed directly to ld
 * - Multiple files may be given on the command line.
 * - Unrecognized options are all sent directly to ld.
 * -c or -S cannot be combined with -o if multiple files are given.
 *
 * This file should be rewritten readable.
 */
#include "config.h"

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <io.h>
#endif

#include "compat.h"

#include "ccconfig.h"
#include "macdefs.h"
/* C command */

#define	MKS(x) _MKS(x)
#define _MKS(x) #x

/*
 * Many specific definitions, should be declared elsewhere.
 */

#ifndef STDINC
#define	STDINC	  	"/usr/include/"
#endif

#ifndef LIBDIR
#define LIBDIR		"/usr/lib/"
#endif

#ifndef PREPROCESSOR
#define PREPROCESSOR	"cpp"
#endif

#ifndef COMPILER
#define COMPILER	"ccom"
#endif

#ifndef ASSEMBLER
#define ASSEMBLER	"as"
#endif

#ifndef LINKER
#define LINKER		"ld"
#endif

#define OS MKS(TARGOS)
#define MACH MKS(TARGMACH)
#ifndef PCCINCDIR
#define PCCINCDIR	LIBDIR "pcc/" MACH "-" OS "/" PACKAGE_VERSION "/include"
#endif
#ifndef PCCLIBDIR
#define PCCLIBDIR	LIBDIR "pcc/" MACH "-" OS "/" PACKAGE_VERSION "/lib"
#endif

#define MAXFIL 10000
#define MAXLIB 10000
#define MAXAV  10000
#define MAXOPT 200
char	*tmp3;
char	*tmp4;
char	*outfile, *ermfile;
char *Bprefix(char *);
char *copy(char *, int);
char *setsuf(char *, char);
int getsuf(char *);
int main(int, char *[]);
void error(char *, ...);
void errorx(int, char *, ...);
int callsys(char [], char *[]);
int cunlink(char *);
void dexit(int);
void idexit(int);
char *gettmp(void);
void *ccmalloc(int size);
#ifdef WIN32
char *win32pathsubst(char *);
char *win32commandline(char *, char *[]);
#endif
char	*av[MAXAV];
char	*clist[MAXFIL];
char    *olist[MAXFIL];
char	*llist[MAXLIB];
char	*aslist[MAXAV];
char	*cpplist[MAXAV];
char	alist[20];
char	*xlist[100];
int	xnum;
char	*mlist[100];
char	*flist[100];
char	*wlist[100];
char	*idirafter;
int	nm;
int	nf;
int	nw;
int	sspflag;
int	dflag;
int	pflag;
int	sflag;
int	cflag;
int	eflag;
int	gflag;
int	rflag;
int	vflag;
int	tflag;
int	Eflag;
int	Oflag;
int	kflag;	/* generate PIC/pic code */
#define F_PIC	1
#define F_pic	2
int	Mflag;	/* dependencies only */
int	pgflag;
int	exfail;
int	Xflag;
int	Wallflag;
int	Wflag;
int	nostartfiles, Bstatic, shared;
int	nostdinc, nostdlib;
int	onlyas;
int	pthreads;
int	xcflag;
int 	ascpp;

char	*passp = LIBEXECDIR PREPROCESSOR;
char	*pass0 = LIBEXECDIR COMPILER;
char	*as = ASSEMBLER;
char	*ld = LINKER;
char	*Bflag;
char *cppadd[] = CPPADD;
#ifdef DYNLINKER
char *dynlinker[] = DYNLINKER;
#endif
#ifdef CRT0FILE
char *crt0file = CRT0FILE;
#endif
#ifdef CRT0FILE_PROFILE
char *crt0file_profile = CRT0FILE_PROFILE;
#endif
#ifdef STARTFILES
char *startfiles[] = STARTFILES;
char *endfiles[] = ENDFILES;
#endif
#ifdef STARTFILES_T
char *startfiles_T[] = STARTFILES_T;
char *endfiles_T[] = ENDFILES_T;
#endif
#ifdef STARTFILES_S
char *startfiles_S[] = STARTFILES_S;
char *endfiles_S[] = ENDFILES_S;
#endif
#ifdef MULTITARGET
char *mach = DEFMACH;
struct cppmd {
	char *mach;
	char *cppmdadd[MAXCPPMDARGS];
};

struct cppmd cppmds[] = CPPMDADDS;
#else
char *cppmdadd[] = CPPMDADD;
#endif
#ifdef LIBCLIBS
char *libclibs[] = LIBCLIBS;
#else
char *libclibs[] = { "-lc", NULL };
#endif
#ifdef LIBCLIBS_PROFILE
char *libclibs_profile[] = LIBCLIBS_PROFILE;
#else
char *libclibs_profile[] = { "-lc_p", NULL };
#endif
#ifndef STARTLABEL
#define STARTLABEL "__start"
#endif
char *incdir = STDINC;
char *altincdir = INCLUDEDIR "pcc/";
char *libdir = LIBDIR;
char *pccincdir = PCCINCDIR;
char *pcclibdir = PCCLIBDIR;
#ifdef mach_amd64
int amd64_i386;
#endif

/* handle gcc warning emulations */
struct Wflags {
	char *name;
	int flags;
#define	INWALL		1
#define	NEGATIVE	2
} Wflags[] = {
	{ "-Wtruncate", 0 },
	{ "-Wno-truncate", NEGATIVE },
	{ "-Werror", 0 },
	{ "-Wshadow", 0 },
	{ "-Wno-shadow", NEGATIVE },
	{ "-Wpointer-sign", INWALL },
	{ "-Wno-pointer-sign", NEGATIVE },
	{ "-Wsign-compare", 0 },
	{ "-Wno-sign-compare", NEGATIVE },
	{ "-Wunknown-pragmas", INWALL },
	{ "-Wno-unknown-pragmas", NEGATIVE },
	{ "-Wunreachable-code", 0 },
	{ "-Wno-unreachable-code", NEGATIVE },
	{ 0, 0 },
};

#define	SZWFL	(sizeof(Wflags)/sizeof(Wflags[0]))

#ifndef USHORT
/* copied from mip/manifest.h */
#define	USHORT		5
#define	INT		6
#define	UNSIGNED	7
#endif

/*
 * Wide char defines.
 */
 #if 0
#if WCHAR_TYPE == USHORT
#define	WCT "short unsigned int"
#define WCM "65535U"
#if WCHAR_SIZE != 2
#error WCHAR_TYPE vs. WCHAR_SIZE mismatch
#endif
#elif WCHAR_TYPE == INT
#define WCT "int"
#define WCM "2147483647"
#if WCHAR_SIZE != 4
#error WCHAR_TYPE vs. WCHAR_SIZE mismatch
#endif
#elif WCHAR_TYPE == UNSIGNED
#define WCT "unsigned int"
#define WCM "4294967295U"
#if WCHAR_SIZE != 4
#error WCHAR_TYPE vs. WCHAR_SIZE mismatch
#endif
#else
#error WCHAR_TYPE not defined or invalid
#endif
 #endif

#define WCT "int"
#define WCM "2147483647"


#ifdef GCC_COMPAT
#ifndef REGISTER_PREFIX
#define REGISTER_PREFIX ""
#endif
#ifndef USER_LABEL_PREFIX
#define USER_LABEL_PREFIX ""
#endif
#endif

#ifndef PCC_PTRDIFF_TYPE
#define PCC_PTRDIFF_TYPE "long int"
#endif

int
main(int argc, char *argv[])
{
	struct Wflags *Wf;
	char *t, *u;
	char *assource;
	char **pv, *ptemp[MAXOPT], **pvt;
	int nc, nl, nas, ncpp, i, j, c, nxo, na;
#ifdef MULTITARGET
	int k;
#endif

#ifdef WIN32
	/* have to prefix path early.  -B may override */
	incdir = win32pathsubst(incdir);
	altincdir = win32pathsubst(altincdir);
	libdir = win32pathsubst(libdir);
	pccincdir = win32pathsubst(pccincdir);
	pcclibdir = win32pathsubst(pcclibdir);
	passp = win32pathsubst(passp);
	pass0 = win32pathsubst(pass0);
#endif

	i = nc = nl = nas = ncpp = nxo = 0;
	pv = ptemp;
	while(++i < argc) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			default:
				goto passa;
#ifdef notyet
	/* must add library options first (-L/-l/...) */
				error("unrecognized option `-%c'", argv[i][1]);
				break;
#endif

			case '-': /* double -'s */
				if (strcmp(argv[i], "--version") == 0) {
					printf("%s\n", VERSSTR);
					return 0;
				} else if (strcmp(argv[i], "--param") == 0) {
					/* NOTHING YET */;
					i++; /* ignore arg */
				} else
					goto passa;
				break;

			case 'B': /* other search paths for binaries */
				Bflag = &argv[i][2];
				break;

#ifdef MULTITARGET
			case 'b':
				t = &argv[i][2];
				if (*t == '\0' && i + 1 < argc) {
					t = argv[i+1];
					i++;
				}
				if (strncmp(t, "?", 1) == 0) {
					/* show machine targets */
					printf("Available machine targets:");
					for (j=0; cppmds[j].mach; j++)
						printf(" %s",cppmds[j].mach);
					printf("\n");
					exit(0);
				}
				for (j=0; cppmds[j].mach; j++)
					if (strcmp(t, cppmds[j].mach) == 0) {
						mach = cppmds[j].mach;
						break;
					}
				if (cppmds[j].mach == NULL)
					errorx(1, "unknown target arch %s", t);
				break;
#endif

			case 'X':
				Xflag++;
				break;
			case 'W': /* Ignore (most of) W-flags */
				if (strncmp(argv[i], "-Wl,", 4) == 0) {
					/* options to the linker */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						llist[nl++] = t;
						t = u;
					}
					llist[nl++] = t;
				} else if (strncmp(argv[i], "-Wa,", 4) == 0) {
					/* options to the assembler */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						aslist[nas++] = t;
						t = u;
					}
					aslist[nas++] = t;
				} else if (strncmp(argv[i], "-Wc,", 4) == 0) {
					/* options to ccom */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						wlist[nw++] = t;
						t = u;
					}
					wlist[nw++] = t;
				} else if (strncmp(argv[i], "-Wp,", 4) == 0) {
					/* preprocessor */
					t = &argv[i][4];
					while ((u = strchr(t, ','))) {
						*u++ = 0;
						cpplist[ncpp++] = t;
						t = u;
					}
					cpplist[ncpp++] = t;
				} else if (strcmp(argv[i], "-Wall") == 0) {
					Wallflag = 1;
				} else if (strcmp(argv[i], "-WW") == 0) {
					Wflag = 1;
				} else {
					/* check and set if available */
					for (Wf = Wflags; Wf->name; Wf++) {
						if (strcmp(argv[i], Wf->name))
							continue;
						wlist[nw++] = Wf->name;
					}
				}
				break;

			case 'f': /* GCC compatibility flags */
				if (strcmp(argv[i], "-fPIC") == 0)
					kflag = F_PIC;
				else if (strcmp(argv[i], "-fpic") == 0)
					kflag = F_pic;
				else if (strcmp(argv[i],
				    "-fsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fno-signed-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-funsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fno-unsigned-char") == 0)
					flist[nf++] = argv[i];
				else if (strcmp(argv[i],
				    "-fstack-protector") == 0) {
					flist[nf++] = argv[i];
					sspflag++;
				} else if (strcmp(argv[i],
				    "-fstack-protector-all") == 0) {
					flist[nf++] = argv[i];
					sspflag++;
				} else if (strcmp(argv[i],
				    "-fno-stack-protector") == 0) {
					flist[nf++] = argv[i];
					sspflag = 0;
				} else if (strcmp(argv[i],
				    "-fno-stack-protector-all") == 0) {
					flist[nf++] = argv[i];
					sspflag = 0;
				}
				/* silently ignore the rest */
				break;

			case 'g': /* create debug output */
				gflag++;
				break;

			case 'i':
				if (strcmp(argv[i], "-isystem") == 0) {
					*pv++ = "-S";
					*pv++ = argv[++i];
				} else if (strcmp(argv[i], "-include") == 0) {
					*pv++ = "-i";
					*pv++ = argv[++i];
				} else if (strcmp(argv[i], "-idirafter") == 0) {
					idirafter = argv[++i];
				} else
					goto passa;
				break;

			case 'k': /* generate PIC code */
				kflag = F_pic;
				break;

			case 'm': /* target-dependent options */
#ifdef mach_amd64
				/* need to call i386 ccom for this */
				if (strcmp(argv[i], "-m32") == 0) {
					pass0 = LIBEXECDIR "/ccom_i386";
					amd64_i386 = 1;
					break;
				}
#endif
				mlist[nm++] = argv[i];
				if (argv[i][2] == 0) {
					/* separate second arg */
					/* give also to linker */
					llist[nl++] = argv[i++];
					mlist[nm++] = argv[i];
					llist[nl++] = argv[i];
				}
				break;

			case 'n': /* handle -n flags */
				if (strcmp(argv[i], "-nostdinc") == 0)
					nostdinc++;
				else if (strcmp(argv[i], "-nostdlib") == 0) {
					nostdlib++;
					nostartfiles++;
				} else if (strcmp(argv[i], "-nostartfiles") == 0)
					nostartfiles = 1;
				else
					goto passa;
				break;

			case 'p':
				if (strcmp(argv[i], "-pg") == 0 ||
				    strcmp(argv[i], "-p") == 0)
					pgflag++;
				else if (strcmp(argv[i], "-pthread") == 0)
					pthreads++;
				else if (strcmp(argv[i], "-pipe") == 0)
					/* NOTHING YET */;
				else if (strcmp(argv[i], "-pedantic") == 0)
					/* NOTHING YET */;
				else if (strcmp(argv[i],
				    "-print-prog-name=ld") == 0) {
					printf("%s\n", LINKER);
					return 0;
				} else
					errorx(1, "unknown option %s", argv[i]);
				break;

			case 'r':
				rflag = 1;
				break;

			case 'x':
				t = &argv[i][2];
				if (*t == 0)
					t = argv[++i];
				if (strcmp(t, "c") == 0)
					xcflag = 1; /* default */
				else if (strcmp(t, "assembler-with-cpp") == 0)
					ascpp = 1;
#ifdef notyet
				else if (strcmp(t, "c++") == 0)
					cxxflag++;
#endif
				else
					xlist[xnum++] = argv[i];
				break;
			case 't':
				tflag++;
				break;
			case 'S':
				sflag++;
				cflag++;
				break;
			case 'o':
				if (outfile)
					errorx(8, "too many -o");
				outfile = argv[++i];
				break;
			case 'O':
				if (argv[i][2] == '0')
					Oflag = 0;
				else
					Oflag++;
				break;
			case 'E':
				Eflag++;
				break;
			case 'P':
				pflag++;
				*pv++ = argv[i];
			case 'c':
				cflag++;
				break;

#if 0
			case '2':
				if(argv[i][2] == '\0')
					pref = "/lib/crt2.o";
				else {
					pref = "/lib/crt20.o";
				}
				break;
#endif
			case 'C':
				cpplist[ncpp++] = argv[i];
				break;
			case 'D':
			case 'I':
			case 'U':
				*pv++ = argv[i];
				if (argv[i][2] == 0)
					*pv++ = argv[++i];
				if (pv >= ptemp+MAXOPT) {
					error("Too many DIU options");
					--pv;
				}
				break;

			case 'M':
				Mflag++;
				break;

			case 'd':
#ifdef os_darwin
				if (strcmp(argv[i], "-dynamiclib") == 0) {
					shared = 1;
				} else
#endif
				if (strcmp(argv[i], "-d") == 0) {
					dflag++;
					strlcpy(alist, argv[i], sizeof (alist));
				}
				break;
			case 'v':
				printf("%s\n", VERSSTR);
				vflag++;
				break;

			case 's':
#ifndef os_darwin
				if (strcmp(argv[i], "-shared") == 0) {
					shared = 1;
#ifndef os_win32
					nostdlib = 1;
#endif
				} else
#endif
				if (strcmp(argv[i], "-static") == 0) {
					Bstatic = 1;
				} else if (strncmp(argv[i], "-std", 4) == 0) {
					/* ignore gcc -std= */;
				} else
					goto passa;
				break;
			}
		} else {
		passa:
			t = argv[i];
			if (*argv[i] == '-' && argv[i][1] == 'L')
				;
			else if((c=getsuf(t))=='c' || c=='S' || c=='i' ||
			    c=='s'|| Eflag || xcflag) {
				clist[nc++] = t;
				if (nc>=MAXFIL) {
					error("Too many source files");
					exit(1);
				}
			}

			/* Check for duplicate .o files. */
			for (j = getsuf(t) == 'o' ? 0 : nl; j < nl; j++) {
				if (strcmp(llist[j], t) == 0)
					break;
			}
			if ((c=getsuf(t))!='c' && c!='S' &&
			    c!='s' && c!='i' && j==nl) {
				llist[nl++] = t;
				if (nl >= MAXLIB) {
					error("Too many object/library files");
					exit(1);
				}
				if (getsuf(t)=='o')
					nxo++;
			}
		}
	}
	/* Sanity checking */
	if (nc == 0 && nl == 0)
		errorx(8, "no input files");
	if (outfile && (cflag || sflag || Eflag) && nc > 1)
		errorx(8, "-o given with -c || -E || -S and more than one file");
	if (outfile && clist[0] && strcmp(outfile, clist[0]) == 0)
		errorx(8, "output file will be clobbered");
	if (nc==0)
		goto nocom;
	if (pflag==0) {
		if (!sflag)
			tmp3 = gettmp();
		tmp4 = gettmp();
	}
	if (Bflag) {
		altincdir = Bflag;
		pccincdir = Bflag;
		pcclibdir = Bflag;
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)	/* interrupt */
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)	/* terminate */
		signal(SIGTERM, idexit);
#ifdef MULTITARGET
	pass0 = copy(LIBEXECDIR "/ccom_", k = strlen(mach));
	strlcat(pass0, mach, sizeof(LIBEXECDIR "/ccom_") + k);
#endif
	pvt = pv;
	for (i=0; i<nc; i++) {
		/*
		 * C preprocessor
		 */
		if (nc>1 && !Eflag)
			printf("%s:\n", clist[i]);
		onlyas = 0;
		assource = tmp3;
		if (getsuf(clist[i])=='S')
			ascpp = 1;
		if (getsuf(clist[i])=='i') {
			if(Eflag)
				continue;
			goto com;
		} else if (ascpp) {
			onlyas = 1;
		} else if (getsuf(clist[i])=='s') {
			assource = clist[i];
			goto assemble;
		}
		if (pflag)
			tmp4 = setsuf(clist[i], 'i');
		na = 0;
		av[na++] = "cpp";
		if (vflag)
			av[na++] = "-v";
		av[na++] = "-D__PCC__=" MKS(PCC_MAJOR);
		av[na++] = "-D__PCC_MINOR__=" MKS(PCC_MINOR);
		av[na++] = "-D__PCC_MINORMINOR__=" MKS(PCC_MINORMINOR);
#ifndef os_win32
#ifdef GCC_COMPAT
		av[na++] = "-D__GNUC__=4";
		av[na++] = "-D__GNUC_MINOR__=3";
		av[na++] = "-D__GNUC_PATCHLEVEL__=1";
		av[na++] = "-D__GNUC_STDC_INLINE__=1";
#endif
#endif
		av[na++] = "-D__VERSION__=" MKS(VERSSTR);
		av[na++] = "-D__SCHAR_MAX__=" MKS(MAX_CHAR);
		av[na++] = "-D__SHRT_MAX__=" MKS(MAX_SHORT);
		av[na++] = "-D__INT_MAX__=" MKS(MAX_INT);
		av[na++] = "-D__LONG_MAX__=" MKS(MAX_LONG);
		av[na++] = "-D__LONG_LONG_MAX__=" MKS(MAX_LONGLONG);
#ifdef CHAR_UNSIGNED
		av[na++] = "-D__CHAR_UNSIGNED__";
#endif
		if (ascpp)
			av[na++] = "-D__ASSEMBLER__";
		if (sspflag)
			av[na++] = "-D__SSP__=1";
		if (pthreads)
			av[na++] = "-D_PTHREADS";
		if (Mflag)
			av[na++] = "-M";
		if (Oflag)
			av[na++] = "-D__OPTIMIZE__";
#ifdef GCC_COMPAT
		av[na++] = "-D__REGISTER_PREFIX__=" REGISTER_PREFIX;
		av[na++] = "-D__USER_LABEL_PREFIX__=" USER_LABEL_PREFIX;
		if (Oflag)
			av[na++] = "-D__OPTIMIZE__";
#endif
		if (dflag)
			av[na++] = alist;
		for (j = 0; cppadd[j]; j++)
			av[na++] = cppadd[j];
		for (j = 0; j < ncpp; j++)
			av[na++] = cpplist[j];
		av[na++] = "-D__STDC_ISO_10646__=200009L";
		av[na++] = "-D__WCHAR_TYPE__=" WCT;
		av[na++] = "-D__SIZEOF_WCHAR_T__=" MKS(WCHAR_SIZE);
		av[na++] = "-D__WCHAR_MAX__=" WCM;
		av[na++] = "-D__WINT_TYPE__=unsigned int";
		av[na++] = "-D__SIZE_TYPE__=unsigned long";
		av[na++] = "-D__PTRDIFF_TYPE__=" PCC_PTRDIFF_TYPE;
		av[na++] = "-D__SIZEOF_WINT_T__=4";
#ifdef MULTITARGET
		for (k = 0; cppmds[k].mach; k++) {
			if (strcmp(cppmds[k].mach, mach) != 0)
				continue;
			for (j = 0; cppmds[k].cppmdadd[j]; j++)
				av[na++] = cppmds[k].cppmdadd[j];
			break;
		}
#else
		for (j = 0; cppmdadd[j]; j++)
			av[na++] = cppmdadd[j];
#endif
		if (tflag)
			av[na++] = "-t";
		for(pv=ptemp; pv <pvt; pv++)
			av[na++] = *pv;
		if (!nostdinc) {
			av[na++] = "-S", av[na++] = altincdir;
			av[na++] = "-S", av[na++] = incdir;
			av[na++] = "-S", av[na++] = pccincdir;
		}
		if (idirafter) {
			av[na++] = "-I";
			av[na++] = idirafter;
		}
		av[na++] = clist[i];
		if (!Eflag && !Mflag)
			av[na++] = tmp4;
		if ((Eflag || Mflag) && outfile)
			 ermfile = av[na++] = outfile;
		av[na++]=0;
		if (callsys(passp, av)) {
			exfail++;
			eflag++;
		}
		if (Eflag || Mflag)
			continue;
		if (onlyas) {
			assource = tmp4;
			goto assemble;
		}

		/*
		 * C compiler
		 */
	com:
		na = 0;
		av[na++]= "ccom";
		if (Wallflag) {
			/* Set only the same flags as gcc */
			for (Wf = Wflags; Wf->name; Wf++) {
				if (Wf->flags != INWALL)
					continue;
				av[na++] = Wf->name;
			}
		}
		if (Wflag) {
			/* set all positive flags */
			for (Wf = Wflags; Wf->name; Wf++) {
				if (Wf->flags == NEGATIVE)
					continue;
				av[na++] = Wf->name;
			}
		}
		for (j = 0; j < nw; j++)
			av[na++] = wlist[j];
		for (j = 0; j < nf; j++)
			av[na++] = flist[j];
#if !defined(os_sunos) && !defined(mach_i386)
		if (vflag)
			av[na++] = "-v";
#endif
		if (pgflag)
			av[na++] = "-p";
		if (gflag)
			av[na++] = "-g";
#ifdef os_darwin
		/* darwin always wants PIC compilation */
		if (!Bstatic)
			av[na++] = "-k";
#elif defined(os_sunos) && defined(mach_i386)
		if (kflag) {
			av[na++] = "-K";
			av[na++] = "pic";
		}
#else
		if (kflag)
			av[na++] = "-k";
#endif
		if (Oflag) {
			av[na++] = "-xtemps";
			av[na++] = "-xdeljumps";
			av[na++] = "-xinline";
		}
		for (j = 0; j < xnum; j++)
			av[na++] = xlist[j];
		for (j = 0; j < nm; j++)
			av[na++] = mlist[j];
		if (getsuf(clist[i])=='i')
			av[na++] = clist[i];
		else
			av[na++] = tmp4; /* created by cpp */
		if (pflag || exfail)
			{
			cflag++;
			continue;
			}
		if(sflag) {
			if (outfile)
				tmp3 = outfile;
			else
				tmp3 = setsuf(clist[i], 's');
		}
		ermfile = av[na++] = tmp3;
#if 0
		if (proflag) {
			av[3] = "-XP";
			av[4] = 0;
		} else
			av[3] = 0;
#endif
		av[na++] = NULL;
		if (callsys(pass0, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (sflag)
			continue;

		/*
		 * Assembler
		 */
	assemble:
		na = 0;
		av[na++] = as;
		for (j = 0; j < nas; j++)
			av[na++] = aslist[j];
#if defined(os_win32) && defined(USE_YASM)
		av[na++] = "-p";
		av[na++] = "gnu";
		av[na++] = "-f";
		av[na++] = "win32";
#endif
#if defined(os_sunos) && defined(mach_sparc64)
		av[na++] = "-m64";
#endif
#if defined(os_darwin)
		if (Bstatic)
			av[na++] = "-static";
#endif
		if (vflag)
			av[na++] = "-v";
		if (kflag)
			av[na++] = "-k";
#ifdef mach_amd64
		if (amd64_i386)
			av[na++] = "--32";
#endif
		av[na++] = "-o";
		if (outfile && cflag)
			ermfile = av[na++] = outfile;
		else if (cflag)
			ermfile = av[na++] = olist[i] = setsuf(clist[i], 'o');
		else
			ermfile = av[na++] = olist[i] = gettmp();
		av[na++] = assource;
		if (dflag)
			av[na++] = alist;
		av[na++] = 0;
		if (callsys(as, av)) {
			cflag++;
			eflag++;
			cunlink(tmp4);
			continue;
		}
		cunlink(tmp4);
	}

	if (Eflag || Mflag)
		dexit(eflag);

	/*
	 * Linker
	 */
nocom:
	if (cflag==0 && nc+nl != 0) {
		j = 0;
		av[j++] = ld;
#ifndef MSLINKER
		if (vflag)
			av[j++] = "-v";
#endif
#if !defined(os_sunos) && !defined(os_win32) && !defined(os_darwin)
		av[j++] = "-X";
#endif
		if (shared) {
#ifdef os_darwin
			av[j++] = "-dylib";
#else
			av[j++] = "-shared";
#endif
#ifdef os_win32
			av[j++] = "-Bdynamic";
#endif
#ifndef os_sunos
		} else {
#ifndef os_win32
#ifndef os_darwin
			av[j++] = "-d";
#endif
			if (rflag) {
				av[j++] = "-r";
			} else {
				av[j++] = "-e";
				av[j++] = STARTLABEL;
			}
#endif
#endif
			if (Bstatic == 0) { /* Dynamic linkage */
#ifdef DYNLINKER
				for (i = 0; dynlinker[i]; i++)
					av[j++] = dynlinker[i];
#endif
			} else {
#ifdef os_darwin
				av[j++] = "-static";
#else
				av[j++] = "-Bstatic";
#endif
			}
		}
		if (outfile) {
#ifdef MSLINKER
#define	OUTSTR	"/OUT:"
			char *s = copy(OUTSTR, i = strlen(outfile));
			strlcat(s, outfile, sizeof(OUTSTR) + i);
			av[j++] = s;
#else
			av[j++] = "-o";
			av[j++] = outfile;
#endif
		}
#ifdef STARTFILES_S
		if (shared) {
			if (!nostartfiles) {
				for (i = 0; startfiles_S[i]; i++)
					av[j++] = Bprefix(startfiles_S[i]);
			}
		} else
#endif
		{
			if (!nostartfiles) {
#ifdef CRT0FILE_PROFILE
				if (pgflag) {
					av[j++] = Bprefix(crt0file_profile);
				} else
#endif
				{
#ifdef CRT0FILE
					av[j++] = Bprefix(crt0file);
#endif
				}
#ifdef STARTFILES_T
				if (Bstatic) {
					for (i = 0; startfiles_T[i]; i++)
						av[j++] = Bprefix(startfiles_T[i]);
				} else
#endif
				{
#ifdef STARTFILES
					for (i = 0; startfiles[i]; i++)
						av[j++] = Bprefix(startfiles[i]);
#endif
				}
			}
		}
		i = 0;
		while (i<nc) {
			av[j++] = olist[i++];
			if (j >= MAXAV)
				error("Too many ld options");
		}
		i = 0;
		while(i<nl) {
			av[j++] = llist[i++];
			if (j >= MAXAV)
				error("Too many ld options");
		}
#if !defined(os_darwin) && !defined(os_sunos)
		/* darwin assembler doesn't want -g */
		if (gflag)
			av[j++] = "-g";
#endif
#if 0
		if (gflag)
			av[j++] = "-lg";
#endif
		if (pthreads)
			av[j++] = "-lpthread";
		if (!nostdlib) {
#ifdef MSLINKER
#define	LFLAG	"/LIBPATH:"
#else
#define	LFLAG	"-L"
#endif
			char *s = copy(LFLAG, i = strlen(pcclibdir));
			strlcat(s, pcclibdir, sizeof(LFLAG) + i);
			av[j++] = s;
#ifdef os_win32
			s = copy(LFLAG, i = strlen(libdir));
			strlcat(s, libdir, sizeof(LFLAG) + i);
			av[j++] = s;
#endif
			if (pgflag) {
				for (i = 0; libclibs_profile[i]; i++)
					av[j++] = Bprefix(libclibs_profile[i]);
			} else {
				for (i = 0; libclibs[i]; i++)
					av[j++] = Bprefix(libclibs[i]);
			}
		}
		if (!nostartfiles) {
#ifdef STARTFILES_S
			if (shared) {
				for (i = 0; endfiles_S[i]; i++)
					av[j++] = Bprefix(endfiles_S[i]);
			} else 
#endif
			{
#ifdef STARTFILES_T
				if (Bstatic) {
					for (i = 0; endfiles_T[i]; i++)
						av[j++] = Bprefix(endfiles_T[i]);
				} else
#endif
				{
#ifdef STARTFILES
					for (i = 0; endfiles[i]; i++)
						av[j++] = Bprefix(endfiles[i]);
#endif
				}
			}
		}
#ifdef __helenos__
		av[j++] = "-T";
		av[j++] = "/inc/_link.ld";
#endif
		
		av[j++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			cunlink(olist[0]);
		else if (nc > 0 && eflag == 0) {
			/* remove .o files XXX ugly */
			for (i = 0; i < nc; i++)
				cunlink(olist[i]);
		}
	}
	dexit(eflag);
	return 0;
}

/*
 * exit and cleanup after interrupt.
 */
void
idexit(int arg)
{
	dexit(100);
}

/*
 * exit and cleanup.
 */
void
dexit(int eval)
{
	if (!pflag && !Xflag) {
		if (sflag==0)
			cunlink(tmp3);
		cunlink(tmp4);
	}
	if (exfail || eflag)
		cunlink(ermfile);
	if (eval == 100)
		_exit(eval);
	exit(eval);
}

static void
ccerror(char *s, va_list ap)
{
	vfprintf(Eflag ? stderr : stdout, s, ap);
	putc('\n', Eflag? stderr : stdout);
	exfail++;
	cflag++;
	eflag++;
}

/*
 * complain a bit.
 */
void
error(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	ccerror(s, ap);
	va_end(ap);
}

/*
 * complain a bit and then exit.
 */
void
errorx(int eval, char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	ccerror(s, ap);
	va_end(ap);
	dexit(eval);
}

char *
Bprefix(char *s)
{
	char *suffix;
	char *str;
	int i;

#ifdef WIN32

	/*  put here to save sprinkling it ~everywhere  */
	s =  win32pathsubst(s);

	if (Bflag == NULL)
		return s;
	suffix = strrchr(s, '/');
	if (suffix == NULL)
		suffix = strrchr(s, '\\');

#else

	if (Bflag == NULL || s[0] != '/')
		return s;
	suffix = strrchr(s, '/');

#endif

	if (suffix == NULL)
		suffix = s;

	str = copy(Bflag, i = strlen(suffix));
	strlcat(str, suffix, strlen(Bflag) + i + 1);
	return str;
}

int
getsuf(char *s)
{
	register char *p;

	if ((p = strrchr(s, '.')) && p[1] != '\0' && p[2] == '\0')
		return p[1];
	return(0);
}

/*
 * Get basename of string s and change its suffix to ch.
 */
char *
setsuf(char *s, char ch)
{
	char *p;

	s = copy(basename(s), 2);
	if ((p = strrchr(s, '.')) == NULL) {
		p = s + strlen(s);
		p[0] = '.';
	}
	p[1] = ch;
	p[2] = '\0';
	return(s);
}

#ifdef WIN32
#define MAX_CMDLINE_LENGTH 32768
int
callsys(char *f, char *v[])
{
	char *cmd;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD exitCode;
	BOOL ok;

	cmd = win32commandline(f, v);
	if (vflag)
		printf("%s\n", cmd);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	ok = CreateProcess(NULL,  // the executable program
		cmd,   // the command line arguments
		NULL,       // ignored
		NULL,       // ignored
		TRUE,       // inherit handles
		HIGH_PRIORITY_CLASS,
		NULL,       // ignored
		NULL,       // ignored
		&si,
		&pi);

	if (!ok) {
		fprintf(stderr, "Can't find %s\n", f);
		return 100;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (exitCode != 0);
}

#elif defined(__helenos__)

#include <task.h>

int callsys(char *f, char *v[])
{
	size_t len;
	char *path = NULL;
	const char *s;
	int t;
	task_exit_t texit;
	int retval;
	
	task_id_t tid;
	
	if (vflag) {
		fprintf(stderr, "%s ", f);
		for (t = 1; v[t]; t++)
			fprintf(stderr, "%s ", v[t]);
		fprintf(stderr, "\n");
	}
	
	if (Bflag) {
		len = strlen (Bflag) + 8;
		path = malloc (len);
		if (path == NULL) {
			error("callsys: malloc failed");
			exit(1);
		}
		if ((s = strrchr(f, '/'))) {
			strlcpy(path, Bflag, len);
			strlcat(path, s, len);
			if (task_spawnv(&tid, path, v) == 0)
				goto _wait;
		}
	}
	
	if (task_spawnv(&tid, f, v) == 0)
		goto _wait;
	
	if ((s = strrchr(f, '/')) && task_spawnv (&tid, s + 1, v) == 0) 
		goto _wait;
	
	errorx(8, "Can't find %s\n", f);
	return 0;
	
_wait:
	
	
	if (task_wait (tid, &texit, &retval) != 0) {
		error("Couldn't wait on task");
		return 0;
	}
	
	return retval;
}

#else

int
callsys(char *f, char *v[])
{
	int t, status = 0;
	pid_t p;
	char *s;
	char * volatile a = NULL;
	volatile size_t len;

	if (vflag) {
		fprintf(stderr, "%s ", f);
		for (t = 1; v[t]; t++)
			fprintf(stderr, "%s ", v[t]);
		fprintf(stderr, "\n");
	}

	if (Bflag) {
		len = strlen(Bflag) + 8;
		a = malloc(len);
	}
#ifdef HAVE_VFORK
	if ((p = vfork()) == 0) {
#else
	if ((p = fork()) == 0) {
#endif
		if (Bflag) {
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
		execvp(f, v);
		if ((s = strrchr(f, '/')))
			execvp(s+1, v);
		fprintf(stderr, "Can't find %s\n", f);
		_exit(100);
	}
	if (p == -1) {
		fprintf(stderr, "fork() failed, try again\n");
		return(100);
	}
	if (Bflag) {
		free(a);
	}
	while (waitpid(p, &status, 0) == -1 && errno == EINTR)
		;
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		dexit(eflag ? eflag : 1);
	errorx(8, "Fatal error in %s", f);

	return 0;
}
#endif

/*
 * Make a copy of string as, mallocing extra bytes in the string.
 */
char *
copy(char *s, int extra)
{
	int len = strlen(s)+1;
	char *rv;

	rv = ccmalloc(len+extra);
	strlcpy(rv, s, len);
	return rv;
}

int
cunlink(char *f)
{
	if (f==0 || Xflag)
		return(0);
	return (unlink(f));
}

#ifdef WIN32
char *
gettmp(void)
{
#define BUFFSIZE 1000
	DWORD pathSize;
	char pathBuffer[BUFFSIZE];
	char tempFilename[MAX_PATH];
	UINT uniqueNum;

	pathSize = GetTempPath(BUFFSIZE, pathBuffer);
	if (pathSize < BUFFSIZE)
		pathBuffer[pathSize] = 0;
	else
		pathBuffer[0] = 0;
	uniqueNum = GetTempFileName(pathBuffer, "ctm", 0, tempFilename);
	if (uniqueNum == 0) {
		fprintf(stderr, "%s:\n", pathBuffer);
		exit(8);
	}
	return copy(tempFilename, 0);
}

#else

char *
gettmp(void)
{
	char *sfn = copy("/tmp/ctm.XXXXXX", 0);
	int fd = -1;

	if ((fd = mkstemp(sfn)) == -1) {
		fprintf(stderr, "%s: %s\n", sfn, strerror(errno));
		exit(8);
	}
	close(fd);
	return sfn;
}
#endif

void *
ccmalloc(int size)
{
	void *rv;

	if ((rv = malloc(size)) == NULL)
		error("malloc failed");
	return rv;
}

#ifdef WIN32

char *
win32pathsubst(char *s)
{
	char env[1024];
	char *rv;
	int len;

	len = ExpandEnvironmentStrings(s, env, sizeof(env));
	if (len <= 0)
		return s;

	while (env[len-1] == '/' || env[len-1] == '\\' || env[len-1] == '\0')
		env[--len] = 0;

	rv = ccmalloc(len+1);
	strlcpy(rv, env, len+1);

	return rv;
}

char *
win32commandline(char *f, char *args[])
{
	char *cmd;
	char *p;
	int len;
	int i, j, k;

	len = strlen(f) + 3;

	for (i = 1; args[i] != NULL; i++) {
		for (j = 0; args[i][j] != '\0'; j++) {
			len++;
			if (args[i][j] == '\"') {
				for (k = j-1; k >= 0 && args[i][k] == '\\'; k--)
					len++;
			}
		}
		for (k = j-1; k >= 0 && args[i][k] == '\\'; k--)
			len++;
		len += j + 3;
	}

	p = cmd = ccmalloc(len);
	*p++ = '\"';
	p += strlcpy(p, f, len-1);
	*p++ = '\"';
	*p++ = ' ';

	for (i = 1; args[i] != NULL; i++) {
		*p++ = '\"';
		for (j = 0; args[i][j] != '\0'; j++) {
			if (args[i][j] == '\"') {
				for (k = j-1; k >= 0 && args[i][k] == '\\'; k--)
					*p++ = '\\';
				*p++ = '\\';
			}
			*p++ = args[i][j];
		}
		for (k = j-1; k >= 0 && args[i][k] == '\\'; k--)
			*p++ = '\\';
		*p++ = '\"';
		*p++ = ' ';
	}
	p[-1] = '\0';

	return cmd;
}

#endif
