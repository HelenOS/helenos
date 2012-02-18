/*	$Id: ccconfig.h,v 1.6.2.1 2011/02/26 07:35:32 ragge Exp $	*/

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

#include <sys/param.h>

/* common cpp predefines */
#define	CPPADD	{ "-D__DragonFly__", "-D__ELF__", NULL, }
#define	DYNLINKER { "-dynamic-linker", "/usr/libexec/ld-elf.so.2", NULL }

#if __DragonFly_version < 200202
#define CRT0FILE "/usr/lib/gcc34/crt1.o"
#define CRT0FILE_PROFILE "/usr/lib/gcc34/gcrt1.o"
#define STARTFILES { "/usr/lib/gcc34/crti.o", "/usr/lib/gcc34/crtbegin.o", NULL }
#define LIBCLIBS { "-lc", "-L/usr/lib/gcc34", "-lgcc", NULL }
#define	ENDFILES { "/usr/lib/gcc34/crtend.o", "/usr/lib/gcc34/crtn.o", NULL }
#else
#define	CRT0FILE "/usr/lib/crt1.o"
#define	CRT0FILE_PROFILE "/usr/lib/gcrt1.o"
#define	STARTFILES { "/usr/lib/crti.o", "/usr/lib/gcc41/crtbegin.o", NULL }
#define	LIBCLIBS { "-lc", "-L/usr/lib/gcc41", "-lgcc", NULL }
#define	ENDFILES { "/usr/lib/gcc41/crtend.o", "/usr/lib/crtn.o", NULL }
#endif

#define STARTLABEL "_start"

#if defined(mach_i386)
#define	CPPMDADD { "-D__i386__", NULL, }
#elif defined(mach_amd64)
#define CPPMDADD \
	{ "-D__x86_64__", "-D__x86_64", "-D__amd64__", "-D__amd64", \
	  "-D__LP64__=1", "-D_LP64=1", NULL, }
#else
#error defines for arch missing
#endif

#define	STABS
