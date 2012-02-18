/*	$Id: ccconfig.h,v 1.19 2010/11/09 08:40:50 ragge Exp $	*/

/*
 * Copyright (c) 2011 Jiri Zarevucky
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

/* common cpp predefines */
#define CPPADD	{ "-D__helenos__", "-D__ELF__", NULL, }

#undef CRT0FILE
#undef CRT0FILE_PROFILE

#define LIBCLIBS { "/lib/libc.a", "/lib/libsoftfloat.a", "/lib/libsoftint.a", NULL }
#define LIBCLIBS_PROFILE LIBCLIBS

#define STARTFILES { NULL }
#define ENDFILES { NULL }

#define STARTFILES_S { NULL }
#define ENDFILES_S { NULL }

#define STARTLABEL "__entry"

#if defined(mach_ia32)
#define CPPMDADD { "-D__i386__", NULL, }
#define DYNLINKER { NULL }
#elif defined(mach_ppc32)
#define CPPMDADD { "-D__ppc__", NULL, }
#define DYNLINKER { NULL }
#elif defined(mach_amd64)
#define CPPMDADD { "-D__x86_64__", NULL, }
#define	DYNLINKER { NULL }
#elif defined(mach_mips32)
#define CPPMDADD { "-D__mips__", NULL, }
#define DYNLINKER { NULL }
#else
#error defines for arch missing
#endif

#ifndef LIBDIR
#define LIBDIR "/lib/"
#endif

#ifndef LIBEXECDIR
#define LIBEXECDIR "/app/"
#endif

#ifndef STDINC
#define STDINC "/inc/c/"
#endif

#ifndef INCLUDEDIR
#define INCLUDEDIR STDINC
#endif

#define STABS

#ifndef ELFABI
#define ELFABI
#endif
