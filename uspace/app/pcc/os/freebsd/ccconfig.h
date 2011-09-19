/*-
 * Copyright (c) 2007 David O'Brien <obrien@FreeBSD.org>
 * Copyright (c) 2007 Ed Schouten <ed@fxq.nl>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LIBDIR
#define LIBDIR "/usr/lib/"
#endif

#define CPPADD { "-D__FreeBSD__=" MKS(TARGOSVER), "-D__ELF__", \
	"-D__unix__=1", "-D__unix=1", NULL, }

/* host-dependent */
#define CRT0FILE LIBDIR "crt1.o"
#define CRT0FILE_PROFILE LIBDIR "gcrt1.o"
#define STARTFILES { LIBDIR "crti.o", LIBDIR "crtbegin.o", NULL }
#define ENDFILES { LIBDIR "crtend.o", LIBDIR "crtn.o", NULL }
#define STARTFILES_S { LIBDIR "crti.o", LIBDIR "crtbeginS.o", NULL }
#define ENDFILES_S { LIBDIR "crtendS.o", LIBDIR "crtn.o", NULL }
#define LIBCLIBS { "-lc", "-lpcc", NULL }
#define STARTLABEL "_start"

/* host-independent */
#define DYNLINKER { "-dynamic-linker", "/libexec/ld-elf.so.1", NULL }

#if defined(mach_i386)
#define CPPMDADD { "-D__i386__", "-D__i386", NULL, }
#elif defined(mach_amd64)
#define CPPMDADD \
	{ "-D__x86_64__", "-D__x86_64", "-D__amd64__", "-D__amd64", \
	  "-D__LP64__=1", "-D_LP64=1", NULL, }
#else
#error defines for arch missing
#endif

#ifdef LANG_F77
#define F77LIBLIST { "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

#define STABS
