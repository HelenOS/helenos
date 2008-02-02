/*	$NetBSD: output.h,v 1.14 1998/01/31 12:37:55 christos Exp $	*/

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
 *	@(#)output.h	8.2 (Berkeley) 5/4/95
 */

#ifndef OUTPUT_INCL

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#if defined(_GNU_SOURCE) && !defined(__UCLIBC__)
#include <stdio.h>
#endif

struct output {
#if defined(_GNU_SOURCE) && !defined(__UCLIBC__)
	FILE *stream;
#endif
	char *nextc;
	int nleft;
	char *buf;
	int bufsize;
	int fd;
	short flags;
};

extern struct output output;
extern struct output errout;
extern struct output memout;
extern struct output *out1;
extern struct output *out2;

void outstr (const char *, struct output *);
#ifndef _GNU_SOURCE
void emptyoutbuf (struct output *);
#endif
void flushall (void);
#ifndef _GNU_SOURCE
void flushout (struct output *);
#endif
void freestdout (void);
void outfmt (struct output *, const char *, ...)
    __attribute__((__format__(__printf__,2,3)));
void out1fmt (const char *, ...)
    __attribute__((__format__(__printf__,1,2)));
#if !defined(__GLIBC__) && !defined(__UCLIBC__)
void dprintf (const char *, ...)
    __attribute__((__format__(__printf__,1,2)));
#endif
void fmtstr (char *, size_t, const char *, ...)
    __attribute__((__format__(__printf__,3,4)));
#ifndef _GNU_SOURCE
void doformat (struct output *, const char *, va_list);
#endif
int xwrite (int, const char *, int);
#if defined(_GNU_SOURCE) && !defined(__UCLIBC__)
void initstreams (void);
void openmemout (void);
void closememout (void);

#define outc(c, o)	putc(c, (o)->stream)
#define flushout(o)	fflush((o)->stream)
#define doformat(d, f, a)	vfprintf((d)->stream, f, a)
#else
#define outc(c, file)	(--(file)->nleft < 0? (emptyoutbuf(file), *(file)->nextc++ = (c)) : (*(file)->nextc++ = (c)))
#endif
#define out1c(c)	outc(c, out1)
#define out2c(c)	outc(c, out2)
#define out1str(s)	outstr(s, out1)
#define out2str(s)	outstr(s, out2)

#define OUTPUT_INCL
#endif
