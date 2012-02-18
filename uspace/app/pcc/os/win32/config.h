/*
 * Config file for visual studio build
 */
#define PREPROCESSOR "%PCCDIR%\\libexec\\cpp.exe"
#define COMPILER "%PCCDIR%\\libexec\\ccom.exe"

#define USE_YASM

#ifdef USE_YASM
#define ASSEMBLER "yasm.exe"
#else
#define ASSEMBLER "gas.exe"
#endif

#ifdef USE_MSLINKER
#define LINKER "link.exe /nologo"
#define MSLINKER
#else
#define LINKER "ld.exe"
#endif


#define PECOFFABI

#define STDINC "%PCCDIR%\\include\\"
#define LIBDIR "%PCCDIR%\\lib\\"
#define INCLUDEDIR STDINC
#define PCCLIBDIR "%PCCDIR%\\lib\\i386-win32\\0.9.9\\lib\\"
#define PCCINCDIR "%PCCDIR%\\lib\\i386-win32\\0.9.9\\include\\"

#if !defined(vsnprintf)
#define vsnprintf _vsnprintf
#endif
/* windows defines (u)uintptr_t in stddef.h, not inttypes.h */
#include <stddef.h>
#if !defined(snprintf)
#define snprintf _snprintf
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#define inline __inline

/* #define HAVE_INTTYPES_H 1 */
#define HAVE_MEMORY_H 1
/* #define HAVE_MKSTEMP 1 */

#ifndef __MSC__
#define HAVE_STDINT_H 1
#endif

#define HAVE_STDLIB_H 1
/* #define HAVE_STRINGS_H 1 */
#define HAVE_STRING_H 1
/* #define HAVE_STRLCAT 1 */
/* #define HAVE_STRLCPY 1 */
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
/* #define HAVE_UNISTD_H 1 */
/* #define HOST_BIG_ENDIAN  */
#define HOST_LITTLE_ENDIAN
#define ISFLEX 1

#define PACKAGE_NAME "pcc"
#define PACKAGE_STRING "pcc 0.9.9"
#define PACKAGE_TARNAME "pcc"
#define PACKAGE_VERSION "0.9.9"
#define PCC_MAJOR 0
#define PCC_MINOR 9
#define PCC_MINORMINOR 9
#define STDC_HEADERS 1
#define TARGET_LITTLE_ENDIAN 1
/* #define TARGOS win32 */
#define VERSSTR "pcc 0.9.9 for win32, gmcgarry@pcc.ludd.ltu.se"
#define WCHAR_SIZE 2
#define WCHAR_TYPE USHORT
#define YYTEXT_POINTER 1
