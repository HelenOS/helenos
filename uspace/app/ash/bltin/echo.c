/*	$NetBSD: echo.c,v 1.8 1996/11/02 18:26:06 christos Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)echo.c	8.1 (Berkeley) 5/31/93
 */

/*
 * Echo command.
 */

#define main echocmd

#ifdef _GNU_SOURCE
#include <stdio.h>

#include "../mystring.h"
#else
#include "bltin.h"
#endif

/* #define eflag 1 */

int
main(argc, argv)  char **argv; {
	register char **ap;
	register char *p;
	register char c;
	int nflag = 0;
#ifndef eflag
	int eflag = 0;
#endif

	ap = argv;
	if (argc)
		ap++;
	while ((p = *ap) != NULL && *p == '-') {
		if (equal(p, "-n")) {
			nflag = 1;
		} else if (equal(p, "-e")) {
#ifndef eflag
			eflag = 1;
#endif
		} else if (equal(p, "-E")) {
#ifndef eflag
			eflag = 0;
#endif
		}
		else break;
		ap++;
	}
	while ((p = *ap++) != NULL) {
		while ((c = *p++) != '\0') {
			if (c == '\\' && eflag) {
				switch (c = *p++) {
				case 'a':  c = '\007'; break;
				case 'b':  c = '\b';  break;
				case 'c':  return 0;		/* exit */
				case 'e':  c = '\033';  break;
				case 'f':  c = '\f';  break;
				case 'n':  c = '\n';  break;
				case 'r':  c = '\r';  break;
				case 't':  c = '\t';  break;
				case 'v':  c = '\v';  break;
				case '\\':  break;		/* c = '\\' */
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					c -= '0';
					if (*p >= '0' && *p <= '7')
						c = c * 8 + (*p++ - '0');
					if (*p >= '0' && *p <= '7')
					c = c * 8 + (*p++ - '0');
					break;
				default:
					p--;
					break;
				}
			}
			putchar(c);
		}
		if (*ap)
			putchar(' ');
	}
	if (! nflag)
		putchar('\n');
#ifdef _GNU_SOURCE
	fflush(stdout);
	if (ferror(stdout)) {
		clearerr(stdout);
		return 1;
	}
#endif
	return 0;
}
