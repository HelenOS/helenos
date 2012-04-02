/*	$Id: ccconfig.h,v 1.17 2010/11/09 08:50:40 ragge Exp $	*/

/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
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

/*
 * Various settings that controls how the C compiler works.
 */

#ifndef LIBDIR
#define LIBDIR "/usr/lib/"
#endif

/* common cpp predefines */
#define	CPPADD	{ "-D__NetBSD__", "-D__ELF__", NULL, }

/* host-dependent */
#define CRT0FILE LIBDIR "crt0.o"
#define CRT0FILE_PROFILE LIBDIR "gcrt0.o"

#if TARGOSVER == 1
#define STARTFILES { LIBDIR "crtbegin.o", NULL }
#define	ENDFILES { LIBDIR "crtend.o", NULL }
#else
#define STARTFILES { LIBDIR "crti.o", LIBDIR "crtbegin.o", NULL }
#define	ENDFILES { LIBDIR "crtend.o", LIBDIR "crtn.o", NULL }
#endif

/* shared libraries linker files */
#if TARGOSVER == 1
#define STARTFILES_S { LIBDIR "crtbeginS.o", NULL }
#define	ENDFILES_S { LIBDIR "crtendS.o", NULL }
#else
#define STARTFILES_S { LIBDIR "crti.o", LIBDIR "crtbeginS.o", NULL }
#define	ENDFILES_S { LIBDIR "crtendS.o", LIBDIR "crtn.o", NULL }
#endif

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/local/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

/* host-independent */
#define	DYNLINKER { "-dynamic-linker", "/usr/libexec/ld.elf_so", NULL }

#if defined(mach_amd64)
#define CPPMDADD \
	{ "-D__x86_64__", "-D__x86_64", "-D__amd64__", "-D__amd64", NULL, }
#elif defined(mach_arm)
#define	CPPMDADD { "-D__arm__", NULL, }
#elif defined(mach_i386)
#define	CPPMDADD { "-D__i386__", NULL, }
#elif defined(mach_mips)
#define	CPPMDADD { "-D__mips__", NULL, }
#elif defined(mach_pdp10)
#define CPPMDADD { "-D__pdp10__", NULL, }
#elif defined(mach_powerpc)
#define	CPPMDADD { "-D__ppc__", NULL, }
#define STARTLABEL "_start"
#elif defined(mach_vax)
#define CPPMDADD { "-D__vax__", NULL, }
#elif defined(mach_sparc64)
#define CPPMDADD { "-D__sparc64__", NULL, }
#else
#error defines for arch missing
#endif

#define	STABS
