                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([pcc], [1.0.0.RELEASE], <pcc-list@ludd.ltu.se>)
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_TARGET

abi=unknown
endian=little
targosver=0
tls=no
gcccompat=yes
# allowed: UNSIGNED (4-char u_int), INT (4-char int), SHORT (2-char u_short)
wchar_type=INT

case "$target_os" in

    apple)
	targos=apple
	abi=classic68k
	case "$target_cpu" in
	    m68k) targmach=m68k endian=big ;;
	esac
	;;

    bsd)
	targos=bsd
	abi=aout
	targmach=pdp11
	wchar_type=USHORT
	;;

    darwin*)
	targos=darwin
	abi=macho
	case "$target_os" in
	    *10.*) targosver=10 ;;
	    *9.*) targosver=9 ;;
	    *8.*) targosver=8 ;;
	    *7.*) targosver=7 ;;
	esac
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc) targmach=powerpc endian=big ;;
	    x86_64) targmach=amd64 ;;
	esac
        ;;

    dragonfly*)
	targos=dragonfly
	abi=elf
	tls=yes
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    freebsd*)
	targos=freebsd
	abi=elf
	case "$target_os" in
	    *9.*) targosver=9 ;;
	    *8.*) targosver=8 ;;
	    *7.*) targosver=7 ;;
	    *6.*) targosver=6 ;;
	    *5.*) targosver=5 ;;
	    *4.*) targosver=4 ;;
	esac
	case "$target_cpu" in
	    i386) targmach=i386 ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    linux*)
	targos=linux
	abi=elf
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    powerpc*) targmach=powerpc endian=big ;;
	    x86_64) targmach=amd64 ;;
	    mipseb) targmach=mips endian=big ;;
	    mips*) targmach=mips ;;
	esac
	;;

    midnightbsd*)
	targos=midnightbsd
	abi=elf
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    sparc64) targmach=sparc64 endian=big ;;
	esac
	;;

    mingw*)
	targos=win32
	abi=pecoff
	wchar_type=USHORT
	targmach=i386
	altincdir="c:/mingw/include"
	altlibdir="c:/mingw/lib"
	;;

    minix)
	targos=minix
	abi=coff
	case "$target_cpu" in
	    m68k) targmach=m68k endian=big ;;
	esac
	;;

    mirbsd*)
	targos=mirbsd
	abi=elf
	wchar_type=USHORT
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	esac
	;;

    netbsd*)
	targos=netbsd
	abi=elf
	case "$target_os" in
	    *5.*) targosver=5 ;;
	    *4.*) targosver=4 ;;
	    *3.*) targosver=3 ;;
	    *2.*) targosver=2 ;;
	    *1.*) targosver=1 ;;
	esac
	case "$target_cpu" in
	    armeb) targmach=arm endian=big ;;
	    arm*) targmach=arm ;;
	    i?86) targmach=i386 ;;
	    m68k*) targmach=m68k endian=big ;;
	    mipseb) targmach=mips endian=big ;;
	    mips*) targmach=mips ;;
	    pdp10) targmach=pdp10 ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    vax) targmach=vax ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    nextstep*)
	targos=nextstep
	abi=macho
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    sparc) targmach=sparc endian=big ;;
	    hppa) targmach=hppa endian=big ;;
	esac
	;;

    openbsd*)
	targos=openbsd
	abi=elf
	case "$target_cpu" in
	    i?86) targmach=i386 ;;
	    vax) targmach=vax ;;
	    powerpc) targmach=powerpc endian=big ;;
	    sparc64) targmach=sparc64 endian=big ;;
	    x86_64) targmach=amd64 ;;
	esac
	;;

    sunos*|solaris*)
        targos=sunos
        abi=elf
        case "$target_cpu" in
            i?86) targmach=i386 ;;
            sparc*) targmach=sparc64 endian=big ;;
        esac
        ;;

    windows*|pe*)
	target_alias=i386-pe
	targos=win32
	abi=pecoff
	wchar_type=USHORT
	targmach=i386
	;;

    *)
        targos="$target_os"
	case "$target_cpu" in
	    m16c) targmach=m16c ;;
	    nova) targmach=nova ;;
	esac
	;;
esac

if test "X$targos" = X -o "X$targmach" = X ; then
	AC_MSG_ERROR(['$target' is not (yet) supported by pcc.])
fi

case "$host_os" in

    pe*)
	# quick hack for cross-build to win32 host
	if "$prefix" = NONE; then
		prefix="c:/pcc"
		assembler="yasm.exe -p gnu -f win32"
		linker="link.exe /nologo"
		ADD_CFLAGS=-DMSLINKER
	fi
	;;

    sunos*|solaris*)
	ADD_CPPFLAGS="$ADD_CPPFLAGS -D_XOPEN_SOURCE=600"
	;;

    darwin*)
	if test "$targosver" -ge 10 ; then
		targmach=amd64
	fi
	;;

esac

if test "X$endian" = "Xbig" ; then
	AC_DEFINE(TARGET_BIG_ENDIAN, 1,
		[Define if target defaults to BIG endian])
else
	AC_DEFINE(TARGET_LITTLE_ENDIAN, 1,
		[Define if target defaults to LITTLE endian])
fi

case "$abi" in
	elf*)		AC_DEFINE(ELFABI, [], [Using ELF ABI]) ;;
	aout)		AC_DEFINE(AOUTABI, [], [Using a.out ABI]) ;;
	macho)		AC_DEFINE(MACHOABI, [], [Using Mach-O ABI]) ;;
	coff)		AC_DEFINE(COFFABI, [], [Using COFF ABI]) ;;
	ecoff)		AC_DEFINE(ECOFFABI, [], [Using ECOFF ABI]) ;;
	pecoff)		AC_DEFINE(PECOFFABI, [], [Using PE/COFF ABI]) ;;
	classic68k)	AC_DEFINE(CLASSIC68K, [], [Using Classic 68k ABI]) ;;
esac

# Specify alternate assembler, linker, include and lib paths
AC_ARG_WITH(incdir,
	AC_HELP_STRING([--with-incdir=<path>],
		[Specify the default include path.]),
	altincdir=$withval,
	[])
AC_ARG_WITH(libdir,
	AC_HELP_STRING([--with-libdir=<path>],
		[Specify the default library path.]),
	altlibdir=$withval,
	[])
AC_ARG_WITH(assembler,
	AC_HELP_STRING([--with-assembler=<path>],
		[Specify alternate assember.]),
	assembler=$withval,
	[])
AC_ARG_WITH(linker,
	AC_HELP_STRING([--with-linker=<path>],
		[Specify alternate linker.]),
	linker=$withval,
	[])
AC_ARG_ENABLE(tls,
	AC_HELP_STRING([--enable-tls],
		[Enable Thread-local storage (TLS).]),
	[tls=$enableval], [])
if test "$tls" = "yes"; then
	AC_DEFINE(TLS, 1, [Enable thread-local storage (TLS).])
fi
AC_ARG_ENABLE(gcc-compat,
	AC_HELP_STRING([--disable-gcc-compat],
		[Disable GCC compatibility]),
	[gcccompat=$enableval], [])
if test "$gcccompat" = "yes"; then
	ADD_CPPFLAGS="$ADD_CPPFLAGS -DGCC_COMPAT";
fi

# setup for building a cross-compiler
if test "X$target_alias" = "X$host_alias" -o "X$target_alias" = "X"; then
	BINPREFIX=""
else
	BINPREFIX="${target_alias}-"
	test "X$prefix" = XNONE && prefix="$ac_default_prefix"
	test "X$exec_prefix" = XNONE && exec_prefix="${prefix}"
	if test -z "$altincdir"; then
		altincdir=${exec_prefix}/${target_alias}/include
	fi
	if test -z "$altlibdir"; then
		altlibdir=${exec_prefix}/${target_alias}/lib
	fi
	if test -z "$assembler"; then
		assembler=${BINPREFIX}as
	fi
	if test -z  "$linker"; then
		linker=${BINPREFIX}ld
	fi
	preprocessor="${BINPREFIX}cpp"
	compiler="${BINPREFIX}ccom"
fi
AC_SUBST(BINPREFIX)

if test -n "$altincdir"; then
	AC_DEFINE_UNQUOTED(STDINC, "$altincdir",
		[Define alternate standard include directory])
fi
if test -n "$altlibdir"; then
	AC_DEFINE_UNQUOTED(LIBDIR, "${altlibdir}/",
		[Define alternate standard lib directory])
fi
if test -n "$assembler"; then
	AC_DEFINE_UNQUOTED(ASSEMBLER, "$assembler",
		[Define path to alternate assembler])
fi
if test -n "$linker"; then
	AC_DEFINE_UNQUOTED(LINKER, "$linker",
		[Define path to alternate linker])
fi
if test -n "$preprocessor"; then
	AC_DEFINE_UNQUOTED(PREPROCESSOR, "${BINPREFIX}cpp",
		[Define path to alternate preprocessor])
fi
if test -n "$compiler"; then
	AC_DEFINE_UNQUOTED(COMPILER, "${BINPREFIX}ccom",
		[Define path to alternate compiler])
fi

case $wchar_type in
USHORT) wchar_size=2 ;;
UNSIGNED|INT) wchar_size=4 ;;
*) AC_MSG_ERROR([Unknown wchar_t '$wchar_type'.]) ;;
esac

AC_DEFINE_UNQUOTED(WCHAR_TYPE, $wchar_type, [Type to use for wide characters])
AC_DEFINE_UNQUOTED(WCHAR_SIZE, $wchar_size, [Size of wide character type in chars])

# check for additional compiler flags
AC_PROG_CC
DESIRED_FLAGS="-Wall -Wmissing-prototypes -Wstrict-prototypes -Wshadow -Wsign-compare -Wtruncate"
for flag in $DESIRED_FLAGS
do
	AC_MSG_CHECKING([whether $CC accepts $flag])
	AC_LANG_CONFTEST([int main() { return 0; }])
	$CC $CFLAGS $flag -c conftest.c > /dev/null 2> /dev/null
	if test $? = 0 ; then
		ADD_CFLAGS="$ADD_CFLAGS $flag"
		AC_MSG_RESULT([yes]);
	else
		AC_MSG_RESULT([no]);
	fi
	rm -f conftest.o
done

# setup for cross-compiling mkext
AC_MSG_CHECKING([for a C compiler for mkext])
if test $cross_compiling = yes; then
        AC_MSG_RESULT([cross compiling])
        AC_CHECK_PROGS(CC_FOR_BUILD, [pcc gcc cc])
else
        AC_MSG_RESULT([not cross compiling])
        CC_FOR_BUILD=${CC-cc}
        AC_SUBST(CC_FOR_BUILD)
fi

AC_CACHE_CHECK([for C99 printf size specifiers], ac_have_c99_format, [
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT], [[
      char buf[64];
      if (sprintf(buf, "%lld%hhd%jd%zd%td", (long long int)1, (char)2, (intmax_t)3, (size_t)4, (ptrdiff_t)5) != 5)
        exit(1);
      else if (strcmp(buf, "12345"))
        exit(2);
      ]])],
    [ ac_have_c99_format=yes ],
    [ ac_have_c99_format=no ],
    [ ac_have_c99_format=yes ])
])
if test $ac_have_c99_format = yes; then
  AC_DEFINE([HAVE_C99_FORMAT], 1,
    [Define to 1 if printf supports C99 size specifiers])
fi

AC_CACHE_CHECK([for C99 variadic macro support], ac_have_variadic, [
  AC_COMPILE_IFELSE([
	#define macro(...) func(__VA_ARGS__)
	int func(int a, ...);
	int test() { return macro(1, 2, 3); }
	], [ ac_have_variadic=yes ], [ ac_have_variadic=no ])])
if test $ac_have_variadic = yes ; then
  AC_DEFINE([HAVE_CPP_VARARG_MACRO_GCC], 1, 
    [Define to 1 if your compiler supports C99 variadic macros])
fi

# Byteorder of host
AC_C_BIGENDIAN([AC_DEFINE(HOST_BIG_ENDIAN,[],[Define if host is BIG endian])],
	[AC_DEFINE(HOST_LITTLE_ENDIAN,[],[Define if host is LITTLE endian])],
	[],[])


# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_YACC
AC_CHECK_PROG(strip,strip,yes,no)

AC_PROG_LEX
if test "$LEX" = flex ; then
	AC_DEFINE_UNQUOTED(ISFLEX, 1, [lex is flex])
fi

# Checks for libraries.

# Checks for header files.
# AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([string.h malloc.h libgen.h])
AC_HEADER_SYS_WAIT

# Checks for library functions.
##  AC_FUNC_STRTOD
# AC_FUNC_VPRINTF
# AC_CHECK_FUNCS([memset strchr strdup strrchr strtol])
AC_CHECK_FUNCS([strtold vsnprintf snprintf mkstemp strlcat strlcpy basename getopt ffs vfork])

AC_EXEEXT

AC_SUBST(targos)
AC_SUBST(targosver)
AC_SUBST(targmach)
AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(libexecdir)
AC_SUBST(includedir)
AC_SUBST(strip)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(ADD_CFLAGS)
AC_SUBST(ADD_CPPFLAGS)

pcc_major=`echo $PACKAGE_VERSION | awk -F. '{print $1}'`
pcc_minor=`echo $PACKAGE_VERSION | awk -F. '{print $2}'`
pcc_minorminor=`echo $PACKAGE_VERSION | awk -F. '{print $3}'`
versstr="\"$PACKAGE_STRING `cat $srcdir/DATESTAMP` for $target, $USER@`hostname`\""

AC_DEFINE_UNQUOTED(TARGOS, $targos, [Target OS])
AC_DEFINE_UNQUOTED(TARGOSVER, $targosver, [Target OS version])
AC_DEFINE_UNQUOTED(PCC_MAJOR, $pcc_major, [Major version no])
AC_DEFINE_UNQUOTED(PCC_MINOR, $pcc_minor, [Minor version no])
AC_DEFINE_UNQUOTED(PCC_MINORMINOR, $pcc_minorminor, [Minor minor version no])
AC_DEFINE_UNQUOTED(VERSSTR, $versstr, [Version string])

AC_CONFIG_FILES([Makefile
		cc/Makefile
		cc/cc/Makefile
		cc/cpp/Makefile
		cc/ccom/Makefile
		f77/Makefile
		f77/f77/Makefile
		f77/fcom/Makefile
])
AC_OUTPUT

eval "exec_prefix=$exec_prefix"
eval "bindir=$bindir"
eval "libexecdir=$libexecdir"

echo
echo "Target CPU is .................... ${targmach}"
echo "Target ABI is .................... ${abi}"
echo "Target OS is ..................... ${targos}"
echo "Compiler is called ............... ${BINPREFIX}pcc${EXEEXT}"
echo "Installing compiler into ......... ${bindir}"
echo "Installing pre-processor into .... ${libexecdir}"
echo "Using assembler .................. ${assembler-<default>}"
echo "Using linker ..................... ${linker-<default>}"
echo "Using include path ............... ${altincdir-<default>}"
echo "Using library path ............... ${altlibdir-<default>}"
echo "Has TLS support .................. $tls"
echo "Has GCC compatibility ............ $gcccompat"
echo "Type of wchar_t is ............... ${wchar_type} (${wchar_size} chars)"
echo
echo "Configure finished.  Do 'make && make install' to compile and install.
"
