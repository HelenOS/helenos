/* $Id: ccconfig.h,v 1.7 2009/01/24 21:43:49 gmcgarry Exp $ */
/*-
 * Copyright (c) 2007, 2008
 *	Thorsten Glaser <tg@mirbsd.de>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un-
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 */

/**
 * Configuration for pcc on a MirOS BSD (i386 or sparc) target
 */

/* === mi part === */

#ifndef LIBDIR
#define LIBDIR			"/usr/lib/"
#endif

/* cpp MI defines */
#define CPPADD			{		\
	"-D__MirBSD__",				\
	"-D__OpenBSD__",			\
	"-D__unix__",				\
	"-D__ELF__",				\
	NULL					\
}

/* for dynamically linked binaries */
#define DYNLINKER		{		\
	"-dynamic-linker",			\
	"/usr/libexec/ld.so",			\
	NULL					\
}
#define STARTFILES		{		\
	LIBDIR "crti.o",			\
	LIBDIR "crtbegin.o",			\
	NULL					\
}
#define ENDFILES		{		\
	LIBDIR "crtend.o",			\
	LIBDIR "crtn.o",			\
	NULL					\
}

/* for shared libraries */
#define STARTFILES_S		{		\
	LIBDIR "crti.o",			\
	LIBDIR "crtbeginS.o",			\
	NULL					\
}
#define ENDFILES_S		{		\
	LIBDIR "crtendS.o",			\
	LIBDIR "crtn.o",			\
	NULL					\
}

/* for statically linked binaries */
#define STARTFILES_T		{		\
	LIBDIR "crti.o",			\
	LIBDIR "crtbeginT.o",			\
	NULL					\
}
#define ENDFILES_T		{		\
	LIBDIR "crtend.o",			\
	LIBDIR "crtn.o",			\
	NULL					\
}

/* libc contains helper functions, so -lpcc is not needed */
#define LIBCLIBS		{		\
	"-lc",					\
	NULL					\
}

/* C run-time startup */
#define CRT0FILE		LIBDIR "crt0.o"
#define STARTLABEL		"__start"

/* debugging info */
#define STABS

/* === md part === */

#if defined(mach_i386)
#define CPPMDADD		{		\
	"-D__i386__",				\
	"-D__i386",				\
	"-Di386",				\
	NULL,					\
}
#elif defined(mach_sparc)
#error pcc does not support sparc yet
#else
#error this architecture is not supported by MirOS BSD
#endif
