/* Hard-coded, because wiring up configure script would just not be worth the effort. */

/* Using a.out ABI */
//#undef AOUTABI

/* Define path to alternate assembler */
#define ASSEMBLER "/app/as"

/* Using Classic 68k ABI */
//#undef CLASSIC68K

/* Using COFF ABI */
//#undef COFFABI

/* Define path to alternate compiler */
//#undef COMPILER

/* Using ECOFF ABI */
//#undef ECOFFABI

/* Using ELF ABI */
#define ELFABI 1

/* Define to 1 if you have the `basename' function. */
//#define HAVE_BASENAME 1

/* Define to 1 if printf supports C99 size specifiers */
//#define HAVE_C99_FORMAT 1

/* Define to 1 if your compiler supports C99 variadic macros */
#define HAVE_CPP_VARARG_MACRO_GCC 1

/* Define to 1 if you have the `ffs' function. */
#define HAVE_FFS 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libgen.h> header file. */
//#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
//#define HAVE_MKSTEMP 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
//#define HAVE_STRLCAT 1

/* Define to 1 if you have the `strlcpy' function. */
//#define HAVE_STRLCPY 1

/* Define to 1 if you have the `strtold' function. */
#define HAVE_STRTOLD 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
//#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
//#define HAVE_VFORK 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

#ifdef __BE__
	/* Define if host is BIG endian */
	#define HOST_BIG_ENDIAN
	/* Define if target defaults to BIG endian */
	#undef TARGET_BIG_ENDIAN
#endif

#ifdef __LE__
	/* Define if host is LITTLE endian */
	#define HOST_LITTLE_ENDIAN
	/* Define if target defaults to LITTLE endian */
	#define TARGET_LITTLE_ENDIAN
#endif

/* lex is flex */
#define ISFLEX 1

/* Define alternate standard lib directory */
#define LIBDIR "/lib/"

/* Define path to alternate linker */
#define LINKER "/app/ld"

/* Using Mach-O ABI */
//#undef MACHOABI

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "<zarevucky.jiri@gmail.com>"

/* Define to the full name of this package. */
#define PACKAGE_NAME "pcc"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "pcc 1.0.0.RELEASE"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pcc"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.0.RELEASE"

/* Major version no */
#define PCC_MAJOR 1

/* Minor version no */
#define PCC_MINOR 0

/* Minor minor version no */
#define PCC_MINORMINOR 0

/* Using PE/COFF ABI */
//#undef PECOFFABI

/* Define path to alternate preprocessor */
#undef PREPROCESSOR

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define alternate standard include directory */
#define STDINC "/inc/c"


/* Target OS */
#define TARGOS helenos

/* Target OS version */
#define TARGOSVER 0

/* Enable thread-local storage (TLS). */
#define TLS 1

/* Version string */
#define VERSSTR "pcc 1.0.0.RELEASE 20110221 for HelenOS"

/* Size of wide character type */
#define WCHAR_SIZE 4

/* Type to use for wide characters */
#define WCHAR_TYPE INT

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
//#define YYTEXT_POINTER 1

#undef COMPILER

