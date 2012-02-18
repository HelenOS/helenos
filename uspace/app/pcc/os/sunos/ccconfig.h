/*	$Id: ccconfig.h,v 1.3 2009/05/19 05:15:38 gmcgarry Exp $	*/

/*
 * Copyright (c) 2008 Adam Hoka.
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
#define	CPPADD	{ "-Dunix", "-Dsun", "-D__SVR4", "-D__unix", "-D__sun", "-D__SunOS", "-D__ELF__", NULL }

/* TODO: _ _SunOS_5_6, _ _SunOS_5_7, _ _SunOS_5_8, _ _SunOS_5_9, _ _SunOS_5_10 */

/* host-dependent */
#define CRT0FILE LIBDIR "crt1.o"
#define CRT0FILE_PROFILE LIBDIR "gcrt1.o"
#define STARTFILES { LIBDIR "crti.o", NULL }
#define	ENDFILES { LIBDIR "crtn.o", NULL }
/* shared libraries linker files */
#define STARTFILES_S { LIBDIR "crti.o", NULL }
#define	ENDFILES_S { LIBDIR "crtn.o", NULL }
#define LIBCLIBS { "-lc", "-lpcc", NULL }

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/local/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

/* host-independent */
#define	DYNLINKER { "-Bdynamic", "/usr/lib/ld.so", NULL }

#if defined(mach_i386)
#define	CPPMDADD { "-D__i386__", "-D__i386", NULL, }
/* Let's keep it here in case of Polaris. ;) */
#elif defined(mach_powerpc)
#define	CPPMDADD { "-D__ppc__", NULL, }
#elif defined(mach_sparc64)
#define CPPMDADD { "-D__sparc64__", "-D__sparc", NULL, }
#else
#error defines for arch missing
#endif

#define	STABS
