#!/bin/sh

#
# Copyright (c) 2009 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

BINUTILS_GDB_GIT="https://github.com/HelenOS/binutils-gdb.git"

BINUTILS_BRANCH="binutils-2_41-helenos"
BINUTILS_VERSION="2.41"

GDB_BRANCH="gdb-13.2-helenos"
GDB_VERSION="13.2"

GCC_GIT="https://github.com/HelenOS/gcc.git"
GCC_BRANCH="13_2_0-helenos"
GCC_VERSION="13.2"

BASEDIR="$PWD"
SRCDIR="$(readlink -f $(dirname "$0"))"

# If we install into a temporary directory and copy from there,
# we don't have to trust the upstream makefiles respect the prefix for
# all files, and we also have a ready-made package for distribution.
INSTALL_DIR="${BASEDIR}/PKG"

SYSTEM_INSTALL=false

BUILD_GDB=false
BUILD_BINUTILS=true
BUILD_GCC=true

if [ -z "$JOBS" ] ; then
	JOBS="`nproc`"
fi

check_error() {
	if [ "$1" -ne "0" ] ; then
		echo
		echo "Script failed: $2"

		exit 1
	fi
}

show_usage() {
	echo "HelenOS cross-compiler toolchain build script"
	echo
	echo "Syntax:"
	echo " $0 [--system-wide] [--with-gdb|--only-gdb] <platform>"
	echo " $0 [--system-wide] --test-version [<platform>]"
	echo
	echo "Possible target platforms are:"
	echo " amd64      AMD64 (x86-64, x64)"
	echo " arm32      ARM 32b"
	echo " arm64      AArch64"
	echo " ia32       IA-32 (x86, i386)"
	echo " ia64       IA-64 (Itanium)"
	echo " mips32     MIPS little-endian 32b"
	echo " mips32eb   MIPS big-endian 32b"
	echo " ppc32      PowerPC 32b"
	echo " riscv64    RISC-V 64b"
	echo " sparc64    SPARC V9"
	echo " all        build all targets"
	echo " parallel   same as 'all', but all in parallel"
	echo " 2-way      same as 'all', but 2-way parallel"
	echo
	echo "The toolchain target installation directory is determined by matching"
	echo "the first of the following conditions:"
	echo
	echo " (1) If the \$CROSS_PREFIX environment variable is set, then it is"
	echo "     used as the target installation directory."
	echo " (2) If the --system-wide option is used, then /opt/HelenOS/cross"
	echo "     is used as the target installation directory. This usually"
	echo "     requires running this script with super user privileges."
	echo " (3) In other cases, \$XDG_DATA_HOME/HelenOS/cross is used as the"
	echo "     target installation directory. If the \$XDG_DATA_HOME environment"
	echo "     variable is not set, then the default value of \$HOME/.local/share"
	echo "     is assumed."
	echo
	echo "The --non-helenos-target option will build non-HelenOS-specific"
	echo "toolchain (i.e. it will use *-linux-* triplet instead of *-helenos)."
	echo "Using this toolchain for building HelenOS is not supported."
	echo
	echo "The --test-version mode tests the currently installed version of the"
	echo "toolchain."

	exit 3
}

set_cross_prefix() {
	if [ -z "$CROSS_PREFIX" ] ; then
		if $SYSTEM_INSTALL ; then
			CROSS_PREFIX="/opt/HelenOS/cross"
		else
			if [ -z "$XDG_DATA_HOME" ] ; then
				XDG_DATA_HOME="$HOME/.local/share"
			fi
			CROSS_PREFIX="$XDG_DATA_HOME/HelenOS/cross"
		fi
	fi
}

test_version() {
	set_cross_prefix

	echo "HelenOS cross-compiler toolchain build script"
	echo
	echo "Testing the version of the installed software in $CROSS_PREFIX"
	echo

	if [ -z "$1" ] || [ "$1" = "all" ] ; then
		PLATFORMS='amd64 arm32 arm64 ia32 ia64 mips32 mips32eb ppc32 riscv64 sparc64'
	else
		PLATFORMS="$1"
	fi

	for PLATFORM in $PLATFORMS ; do
		set_target_from_platform "$PLATFORM"
		PREFIX="${CROSS_PREFIX}/bin/${TARGET}"

		echo "== $PLATFORM =="
		test_app_version "Binutils" "ld" "GNU ld (.*) \([.0-9]*\)" "$BINUTILS_VERSION"
		test_app_version "GCC" "gcc" "gcc version \([.0-9]*\)" "$GCC_VERSION"
		test_app_version "GDB" "gdb" "GNU gdb (.*)[[:space:]]\+\([.0-9]*\)" "$GDB_VERSION"
	done
}

test_app_version() {
	PKGNAME="$1"
	APPNAME="$2"
	REGEX="$3"
	INS_VERSION="$4"

	APP="${PREFIX}-${APPNAME}"
	if [ ! -e $APP ]; then
		echo "- $PKGNAME is missing"
	else
		VERSION=`${APP} -v 2>&1 | sed -n "s:^${REGEX}.*:\1:p"`

		if [ -z "$VERSION" ]; then
			echo "- $PKGNAME Unexpected output"
			return 1
		fi

		if [ "$INS_VERSION" = "$VERSION" ]; then
			echo "+ $PKGNAME is up-to-date ($INS_VERSION)"
		else
			echo "- $PKGNAME ($VERSION) is outdated ($INS_VERSION)"
		fi
	fi
}

change_title() {
	printf "\e]0;$1\a"
}

ring_bell() {
	printf '\a'
	sleep 0.1
	printf '\a'
	sleep 0.1
	printf '\a'
	sleep 0.1
	printf '\a'
	sleep 0.1
	printf '\a'
}

show_countdown() {
	TM="$1"

	if [ "${TM}" -eq 0 ] ; then
		echo
		return 0
	fi

	printf "${TM} "
	change_title "${TM}"
	sleep 1

	TM="`expr "${TM}" - 1`"
	show_countdown "${TM}"
}

show_dependencies() {
	set_cross_prefix

	echo "HelenOS cross-compiler toolchain build script"
	echo
	echo "Installing software to $CROSS_PREFIX"
	echo
	echo
	echo "IMPORTANT NOTICE:"
	echo
	echo "For a successful compilation and use of the cross-compiler"
	echo "toolchain you need at least the following dependencies."
	echo
	echo "Please make sure that the dependencies are present in your"
	echo "system. Otherwise the compilation process might fail after"
	echo "a few seconds or minutes."
	echo
	echo " - SED, AWK, Flex, Bison, gzip, bzip2, Bourne Shell"
	echo " - gettext, zlib, Texinfo, libelf, libgomp"
	echo " - GNU Make, Coreutils, Sharutils, tar"
	echo " - native C and C++ compiler, assembler and linker"
	echo " - native C and C++ standard library with headers"
	echo
}

cleanup_dir() {
	DIR="$1"

	if [ -d "${DIR}" ] ; then
		change_title "Removing ${DIR}"
		echo " >>> Removing ${DIR}"
		rm -fr "${DIR}"
	fi
}

create_dir() {
	DIR="$1"
	DESC="$2"

	change_title "Creating ${DESC}"
	echo ">>> Creating ${DESC}"

	mkdir -p "${DIR}"
	test -d "${DIR}"
	check_error $? "Unable to create ${DIR}."
}

check_dirs() {
	cd "${BASEDIR}"
	check_error $? "Unable to change directory to ${BASEDIR}."
	ABS_BASE="$PWD"

	if $SYSTEM_INSTALL && [ ! -d "${CROSS_PREFIX}" ]; then
		ring_bell
		( set -x ; sudo -k mkdir -p "${CROSS_PREFIX}" )
	else
		mkdir -p "${CROSS_PREFIX}"
	fi

	cd "${CROSS_PREFIX}"
	check_error $? "Unable to change directory to ${CROSS_PREFIX}."

	while [ "${#PWD}" -gt "${#ABS_BASE}" ]; do
		cd ..
	done

	if [ "$PWD" = "$ABS_BASE" ]; then
		echo
		echo "CROSS_PREFIX cannot reside within the working directory."

		exit 5
	fi

	cd "${BASEDIR}"
}

prepare() {
	show_dependencies
	show_countdown 10

	mkdir -p "${BASEDIR}/downloads"
	cd "${BASEDIR}/downloads"
	check_error $? "Change directory failed."

	change_title "Downloading sources"
	echo ">>> Downloading sources"

	if $BUILD_BINUTILS ; then
		git clone --depth 1 -b "$BINUTILS_BRANCH" "$BINUTILS_GDB_GIT" "binutils-$BINUTILS_VERSION"
		# If the directory already existed, pull upstream changes.
		git -C "binutils-$BINUTILS_VERSION" pull
	fi

	if $BUILD_GCC ; then
		git clone --depth 1 -b "$GCC_BRANCH" "$GCC_GIT" "gcc-$GCC_VERSION"
		git -C "gcc-$GCC_VERSION" pull

		change_title "Downloading GCC prerequisites"
		echo ">>> Downloading GCC prerequisites"
		cd "gcc-${GCC_VERSION}"
		./contrib/download_prerequisites
		cd ..
	fi

	if $BUILD_GDB ; then
		git clone --depth 1 -b "$GDB_BRANCH" "$BINUTILS_GDB_GIT" "gdb-$GDB_VERSION"
		git -C "gdb-$GDB_VERSION" pull
	fi

	# This sets the CROSS_PREFIX variable
	set_cross_prefix

	DESTDIR_SPEC="DESTDIR=${INSTALL_DIR}"

	check_dirs
}

set_target_from_platform() {
	case "$1" in
		"arm32")
			GNU_ARCH="arm"
			;;
		"arm64")
			GNU_ARCH="aarch64"
			;;
		"ia32")
			GNU_ARCH="i686"
			;;
		"mips32")
			GNU_ARCH="mipsel"
			;;
		"mips32eb")
			GNU_ARCH="mips"
			;;
		"ppc32")
			GNU_ARCH="ppc"
			;;
		*)
			GNU_ARCH="$1"
			;;
	esac

	TARGET="${GNU_ARCH}-helenos"
}

build_binutils() {
	# This sets the TARGET variable
	set_target_from_platform "$1"

	WORKDIR="${BASEDIR}/${TARGET}"
	BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"

	echo ">>> Removing previous content"
	cleanup_dir "${WORKDIR}"
	mkdir -p "${WORKDIR}"

	echo ">>> Processing binutils (${TARGET})"
	mkdir -p "${BINUTILSDIR}"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."

	change_title "binutils: configure (${TARGET})"
	CFLAGS="-Wno-error -fcommon" "${BASEDIR}/downloads/binutils-${BINUTILS_VERSION}/configure" \
		"--target=${TARGET}" \
		"--prefix=${CROSS_PREFIX}" \
		"--program-prefix=${TARGET}-" \
		--disable-nls \
		--disable-werror \
		--enable-gold \
		--enable-deterministic-archives \
		--disable-gdb \
		--with-sysroot
	check_error $? "Error configuring binutils."

	change_title "binutils: make (${TARGET})"
	make all -j$JOBS
	check_error $? "Error compiling binutils."

	change_title "binutils: install (${TARGET})"
	make install $DESTDIR_SPEC
	check_error $? "Error installing binutils."
}

build_gcc() {
	# This sets the TARGET variable
	set_target_from_platform "$1"

	WORKDIR="${BASEDIR}/${TARGET}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"

	echo ">>> Removing previous content"
	cleanup_dir "${WORKDIR}"
	mkdir -p "${WORKDIR}"

	echo ">>> Processing GCC (${TARGET})"
	mkdir -p "${GCCDIR}"
	cd "${GCCDIR}"
	check_error $? "Change directory failed."

	BUILDPATH="${CROSS_PREFIX}/bin:${PATH}"

	change_title "GCC: configure (${TARGET})"
	PATH="${BUILDPATH}" "${BASEDIR}/downloads/gcc-${GCC_VERSION}/configure" \
		"--target=${TARGET}" \
		"--prefix=${CROSS_PREFIX}" \
		"--program-prefix=${TARGET}-" \
		--with-gnu-as \
		--with-gnu-ld \
		--disable-nls \
		--enable-languages=c,c++,go \
		--enable-lto \
		--disable-shared \
		--disable-werror \
		--without-headers  # TODO: Replace with proper sysroot so we can build more libs
	check_error $? "Error configuring GCC."

	change_title "GCC: make (${TARGET})"
	PATH="${BUILDPATH}" make all-gcc -j$JOBS
	check_error $? "Error compiling GCC."

	change_title "GCC: install (${TARGET})"
	PATH="${BUILDPATH}" make install-gcc $DESTDIR_SPEC
	check_error $? "Error installing GCC."
}

build_libgcc() {
	# This sets the TARGET variable
	set_target_from_platform "$1"

	WORKDIR="${BASEDIR}/${TARGET}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"

	# No removing previous content here, we need the previous GCC build

	cd "${GCCDIR}"
	check_error $? "Change directory failed."

	BUILDPATH="${CROSS_PREFIX}/bin:${PATH}"

	change_title "libgcc: make (${TARGET})"

	PATH="${BUILDPATH}" make all-target-libgcc -j$JOBS
	check_error $? "Error compiling libgcc."
	# TODO: libatomic and libstdc++ need some extra care
	#    PATH="${BUILDPATH}" make all-target-libatomic -j$JOBS
	#    check_error $? "Error compiling libatomic."
	#    PATH="${BUILDPATH}" make all-target-libstdc++-v3 -j$JOBS
	#    check_error $? "Error compiling libstdc++."

	change_title "libgcc: install (${TARGET})"

	PATH="${BUILDPATH}" make install-target-libgcc $DESTDIR_SPEC
	#    PATH="${BUILDPATH}" make install-target-libatomic $DESTDIR_SPEC
	#    PATH="${BUILDPATH}" make install-target-libstdc++-v3 $DESTDIR_SPEC
	check_error $? "Error installing libgcc."
}

build_gdb() {
	# This sets the TARGET variable
	set_target_from_platform "$1"

	WORKDIR="${BASEDIR}/${TARGET}"
	GDBDIR="${WORKDIR}/gdb-${GDB_VERSION}"

	echo ">>> Removing previous content"
	cleanup_dir "${WORKDIR}"
	mkdir -p "${WORKDIR}"

	echo ">>> Processing GDB (${TARGET})"
	mkdir -p "${GDBDIR}"
	cd "${GDBDIR}"
	check_error $? "Change directory failed."

	change_title "GDB: configure (${TARGET})"
	CFLAGS="-fcommon" "${BASEDIR}/downloads/gdb-${GDB_VERSION}/configure" \
		"--target=${TARGET}" \
		"--prefix=${CROSS_PREFIX}" \
		"--program-prefix=${TARGET}-" \
		--enable-werror=no
	check_error $? "Error configuring GDB."

	change_title "GDB: make (${TARGET})"
	make all-gdb -j$JOBS
	check_error $? "Error compiling GDB."

	change_title "GDB: install (${TARGET})"
	make install-gdb $DESTDIR_SPEC
	check_error $? "Error installing GDB."
}

install_pkg() {
	echo ">>> Moving to the destination directory."
	if $SYSTEM_INSTALL ; then
		ring_bell
		( set -x ; sudo -k cp -r -t "${CROSS_PREFIX}" "${INSTALL_DIR}${CROSS_PREFIX}/"* )
	else
		( set -x ; cp -r -t "${CROSS_PREFIX}" "${INSTALL_DIR}${CROSS_PREFIX}/"* )
	fi
}

link_clang() {
	# Symlink clang and lld to the install path.
	CLANG="`which clang 2> /dev/null || echo "/usr/bin/clang"`"
	LLD="`which ld.lld 2> /dev/null || echo "/usr/bin/ld.lld"`"

	ln -s $CLANG "${INSTALL_DIR}${CROSS_PREFIX}/bin/${TARGET}-clang"
	ln -s $LLD "${INSTALL_DIR}${CROSS_PREFIX}/bin/${TARGET}-ld.lld"
}

while [ "$#" -gt 1 ] ; do
	case "$1" in
		--system-wide)
			SYSTEM_INSTALL=true
			shift
			;;
		--test-version)
			test_version "$2"
			exit
			;;
		--with-gdb)
			BUILD_GDB=true
			shift
			;;
		--only-gdb)
			BUILD_GDB=true
			BUILD_BINUTILS=false
			BUILD_GCC=false
			shift
			;;
		*)
			show_usage
			;;
	esac
done

if [ "$#" -lt "1" ] ; then
	show_usage
fi

PLATFORMS="amd64 arm32 arm64 ia32 ia64 mips32 mips32eb ppc32 riscv64 sparc64"

run_one() {
	$1 $PLATFORM
}

run_all() {
	for x in $PLATFORMS ; do
		$1 $x
	done
}

run_parallel() {
	for x in $PLATFORMS ; do
		$1 $x &
	done
	wait
}

run_2way() {
	$1 amd64 &
	$1 arm32 &
	wait

	$1 arm64 &
	$1 ia32 &
	wait

	$1 ia64 &
	$1 mips32 &
	wait

	$1 mips32eb &
	$1 ppc32 &
	wait

	$1 riscv64 &
	$1 sparc64 &
	wait
}

everything() {
	RUNNER="$1"

	prepare

	if $BUILD_BINUTILS ; then
		$RUNNER build_binutils

		if $BUILD_GCC ; then
			# gcc/libgcc may fail to build correctly if binutils is not installed first
			echo ">>> Installing binutils"
			install_pkg
		fi
	fi

	if $BUILD_GCC ; then
		$RUNNER build_gcc

		# libgcc may fail to build correctly if gcc is not installed first
		echo ">>> Installing GCC"
		install_pkg

		$RUNNER build_libgcc
	fi

	if $BUILD_GDB ; then
		$RUNNER build_gdb
	fi

	echo ">>> Installing all files"
	install_pkg

	link_clang
}

case "$1" in
	--test-version)
		test_version
		exit
		;;
	amd64|arm32|arm64|ia32|ia64|mips32|mips32eb|ppc32|riscv64|sparc64)
		PLATFORM="$1"
		everything run_one
		;;
	"all")
		everything run_all
		;;
	"parallel")
		everything run_parallel
		;;
	"2-way")
		everything run_2way
		;;
	*)
		show_usage
		;;
esac
