#! /bin/bash

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

BINUTILS_VERSION="2.28"
BINUTILS_RELEASE=""
## BINUTILS_PATCHES="toolchain-binutils-2.23.1.patch"
GCC_VERSION="7.1.0"
## GCC_PATCHES="toolchain-gcc-4.8.1-targets.patch toolchain-gcc-4.8.1-headers.patch"
GDB_VERSION="7.12.1"
## GDB_PATCHES="toolchain-gdb-7.6.1.patch"

BASEDIR="`pwd`"
SRCDIR="$(readlink -f $(dirname "$0"))"
BINUTILS="binutils-${BINUTILS_VERSION}${BINUTILS_RELEASE}.tar.bz2"
GCC="gcc-${GCC_VERSION}.tar.bz2"
GDB="gdb-${GDB_VERSION}.tar.gz"

REAL_INSTALL=true
USE_HELENOS_TARGET=false

check_error() {
	if [ "$1" -ne "0" ] ; then
		echo
		echo "Script failed: $2"

		exit 1
	fi
}

check_md5() {
	FILE="$1"
	SUM="$2"

	COMPUTED="`md5sum "${FILE}" | cut -d' ' -f1`"
	if [ "${SUM}" != "${COMPUTED}" ] ; then
		echo
		echo "Checksum of ${FILE} does not match."

		exit 2
	fi
}

show_usage() {
	echo "Cross-compiler toolchain build script"
	echo
	echo "Syntax:"
	echo " $0 [--no-install] [--helenos-target] <platform>"
	echo
	echo "Possible target platforms are:"
	echo " amd64      AMD64 (x86-64, x64)"
	echo " arm32      ARM 32b"
	echo " ia32       IA-32 (x86, i386)"
	echo " ia64       IA-64 (Itanium)"
	echo " mips32     MIPS little-endian 32b"
	echo " mips32eb   MIPS big-endian 32b"
	echo " mips64     MIPS little-endian 64b"
	echo " ppc32      PowerPC 32b"
	echo " ppc64      PowerPC 64v"
	echo " riscv64    RISC-V 64b"
	echo " sparc64    SPARC V9"
	echo " all        build all targets"
	echo " essential  build only targets currently needed for HelenOS development"
	echo " parallel   same as 'all', but all in parallel"
	echo " 2-way      same as 'all', but 2-way parallel"
	echo
	echo "The toolchain is installed into directory specified by the"
	echo "CROSS_PREFIX environment variable. If the variable is not"
	echo "defined, /usr/local/cross/ is used as default."
	echo
	echo "If --no-install is present, the toolchain still uses the"
	echo "CROSS_PREFIX as the target directory but the installation"
	echo "copies the files into PKG/ subdirectory without affecting"
	echo "the actual root file system. That is only useful if you do"
	echo "not want to run the script under the super user."
	echo
	echo "The --helenos-target will build HelenOS-specific toolchain"
	echo "(i.e. it will use *-helenos-* triplet instead of *-linux-*)."
	echo "This toolchain is installed into /usr/local/cross-helenos by"
	echo "default. The settings can be changed by setting environment"
	echo "variable CROSS_HELENOS_PREFIX."
	echo "Using the HelenOS-specific toolchain is still an experimental"
	echo "feature that is not fully supported."
	echo

	exit 3
}

change_title() {
	echo -en "\e]0;$1\a"
}

show_countdown() {
	TM="$1"

	if [ "${TM}" -eq 0 ] ; then
		echo
		return 0
	fi

	echo -n "${TM} "
	change_title "${TM}"
	sleep 1

	TM="`expr "${TM}" - 1`"
	show_countdown "${TM}"
}

show_dependencies() {
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

download_fetch() {
	SOURCE="$1"
	FILE="$2"
	CHECKSUM="$3"

	if [ ! -f "${FILE}" ] ; then
		change_title "Downloading ${FILE}"
		wget -c "${SOURCE}${FILE}" -O "${FILE}".part
		check_error $? "Error downloading ${FILE}."

		mv "${FILE}".part "${FILE}"
	fi

	check_md5 "${FILE}" "${CHECKSUM}"
}

source_check() {
	FILE="$1"

	if [ ! -f "${FILE}" ] ; then
		echo
		echo "File ${FILE} not found."

		exit 4
	fi
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
	OUTSIDE="$1"
	BASE="$2"
	ORIGINAL="`pwd`"

	mkdir -p "${OUTSIDE}"

	cd "${OUTSIDE}"
	check_error $? "Unable to change directory to ${OUTSIDE}."
	ABS_OUTSIDE="`pwd`"

	cd "${BASE}"
	check_error $? "Unable to change directory to ${BASE}."
	ABS_BASE="`pwd`"

	cd "${ORIGINAL}"
	check_error $? "Unable to change directory to ${ORIGINAL}."

	BASE_LEN="${#ABS_BASE}"
	OUTSIDE_TRIM="${ABS_OUTSIDE:0:${BASE_LEN}}"

	if [ "${OUTSIDE_TRIM}" == "${ABS_BASE}" ] ; then
		echo
		echo "CROSS_PREFIX cannot reside within the working directory."

		exit 5
	fi
}

unpack_tarball() {
	FILE="$1"
	DESC="$2"

	change_title "Unpacking ${DESC}"
	echo " >>> Unpacking ${DESC}"

	case "${FILE}" in
		*.gz)
			tar -xzf "${FILE}"
			;;
		*.xz)
			tar -xJf "${FILE}"
			;;
		*.bz2)
			tar -xjf "${FILE}"
			;;
		*)
			check_error 1 "Don't know how to unpack ${DESC}."
			;;
	esac
	check_error $? "Error unpacking ${DESC}."
}

patch_sources() {
	PATCH_FILE="$1"
	PATCH_STRIP="$2"
	DESC="$3"

	change_title "Patching ${DESC}"
	echo " >>> Patching ${DESC} with ${PATCH_FILE}"

	patch -t "-p${PATCH_STRIP}" <"$PATCH_FILE"
	check_error $? "Error patching ${DESC}."
}

prepare() {
	show_dependencies
	show_countdown 10

	mkdir -p "${BASEDIR}/downloads"
	cd "${BASEDIR}/downloads"
	check_error $? "Change directory failed."

	BINUTILS_SOURCE="ftp://ftp.gnu.org/gnu/binutils/"
	GCC_SOURCE="ftp://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/"
	GDB_SOURCE="ftp://ftp.gnu.org/gnu/gdb/"

	echo ">>> Downloading tarballs"

	download_fetch "${BINUTILS_SOURCE}" "${BINUTILS}" "9e8340c96626b469a603c15c9d843727"
	download_fetch "${GCC_SOURCE}" "${GCC}" "6bf56a2bca9dac9dbbf8e8d1036964a8"
	download_fetch "${GDB_SOURCE}" "${GDB}" "06c8f40521ed65fe36ebc2be29b56942"

	echo ">>> Unpacking tarballs"
	cd "${WORKDIR}"
	check_error $? "Change directory failed."

	unpack_tarball "${BASEDIR}/downloads/${BINUTILS}" "binutils"
	unpack_tarball "${BASEDIR}/downloads/${GCC}" "GCC"
	unpack_tarball "${BASEDIR}/downloads/${GDB}" "GDB"

	echo ">>> Applying patches"
	for p in $BINUTILS_PATCHES ; do
		patch_sources "${SRCDIR}/${p}" 0 "binutils"
	done
	for p in $GCC_PATCHES ; do
		patch_sources "${SRCDIR}/${p}" 0 "GCC"
	done
	for p in $GDB_PATCHES ; do
		patch_sources "${SRCDIR}/${p}" 0 "GDB"
	done

	echo ">>> Downloading GCC prerequisites"
	cd "gcc-${GCC_VERSION}"
	./contrib/download_prerequisites
	cd ..
}

set_target_from_platform() {
	case "$1" in
		"amd64")
			LINUX_TARGET="amd64-unknown-elf"
			HELENOS_TARGET="amd64-helenos"
			;;
		"arm32")
			LINUX_TARGET="arm-linux-gnueabi"
			HELENOS_TARGET="arm-helenos-gnueabi"
			;;
		"ia32")
			LINUX_TARGET="i686-pc-linux-gnu"
			HELENOS_TARGET="i686-pc-helenos"
			;;
		"ia64")
			LINUX_TARGET="ia64-pc-linux-gnu"
			HELENOS_TARGET="ia64-pc-helenos"
			;;
		"mips32")
			LINUX_TARGET="mipsel-linux-gnu"
			HELENOS_TARGET="mipsel-helenos"
			;;
		"mips32eb")
			LINUX_TARGET="mips-linux-gnu"
			HELENOS_TARGET="mips-helenos"
			;;
		"mips64")
			LINUX_TARGET="mips64el-linux-gnu"
			HELENOS_TARGET="mips64el-helenos"
			;;
		"ppc32")
			LINUX_TARGET="ppc-linux-gnu"
			HELENOS_TARGET="ppc-helenos"
			;;
		"ppc64")
			LINUX_TARGET="ppc64-linux-gnu"
			HELENOS_TARGET="ppc64-helenos"
			;;
		"riscv64")
			LINUX_TARGET="riscv64-unknown-linux-gnu"
			HELENOS_TARGET="riscv64-helenos"
			;;
		"sparc64")
			LINUX_TARGET="sparc64-linux-gnu"
			HELENOS_TARGET="sparc64-helenos"
			;;
		*)
			check_error 1 "No target known for $1."
			;;
	esac
}

build_target() {
	PLATFORM="$1"

	# This sets the *_TARGET variables
	set_target_from_platform "$PLATFORM"
	if $USE_HELENOS_TARGET ; then
		TARGET="$HELENOS_TARGET"
	else
		TARGET="$LINUX_TARGET"
	fi

	WORKDIR="${BASEDIR}/${TARGET}"
	INSTALL_DIR="${WORKDIR}/PKG"
	BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"
	GDBDIR="${WORKDIR}/gdb-${GDB_VERSION}"

	if [ -z "${CROSS_PREFIX}" ] ; then
		CROSS_PREFIX="/usr/local/cross"
	fi

	PREFIX="${CROSS_PREFIX}/${TARGET}"

	echo ">>> Removing previous content"
	cleanup_dir "${WORKDIR}"
	mkdir -p "${WORKDIR}"
	check_dirs "${PREFIX}" "${WORKDIR}"

	echo ">>> Processing binutils (${PLATFORM})"
	mkdir -p "${BINUTILSDIR}"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."

	change_title "binutils: configure (${PLATFORM})"
	CFLAGS=-Wno-error "${BASEDIR}/downloads/binutils-${BINUTILS_VERSION}/configure" \
		"--target=${TARGET}" \
		"--prefix=${PREFIX}" "--program-prefix=${TARGET}-" \
		--disable-nls --disable-werror --enable-gold \
		--enable-deterministic-archives
	check_error $? "Error configuring binutils."

	change_title "binutils: make (${PLATFORM})"
	make all
	check_error $? "Error compiling binutils."

	change_title "binutils: install (${PLATFORM})"
	make install "DESTDIR=${INSTALL_DIR}"
	check_error $? "Error installing binutils."


	echo ">>> Processing GCC (${PLATFORM})"
	mkdir -p "${GCCDIR}"
	cd "${GCCDIR}"
	check_error $? "Change directory failed."

	change_title "GCC: configure (${PLATFORM})"
	PATH="$PATH:${INSTALL_DIR}/${PREFIX}/bin" "${BASEDIR}/downloads/gcc-${GCC_VERSION}/configure" \
		"--target=${TARGET}" \
		"--prefix=${PREFIX}" "--program-prefix=${TARGET}-" \
		--with-gnu-as --with-gnu-ld --disable-nls --disable-threads \
		--enable-languages=c,objc,c++,obj-c++ \
		--disable-multilib --disable-libgcj --without-headers \
		--disable-shared --enable-lto --disable-werror
	check_error $? "Error configuring GCC."

	change_title "GCC: make (${PLATFORM})"
	PATH="${PATH}:${PREFIX}/bin:${INSTALL_DIR}/${PREFIX}/bin" make all-gcc
	check_error $? "Error compiling GCC."

	change_title "GCC: install (${PLATFORM})"
	PATH="${PATH}:${INSTALL_DIR}/${PREFIX}/bin" make install-gcc "DESTDIR=${INSTALL_DIR}"
	check_error $? "Error installing GCC."


	# No GDB support for RISC-V so far
	if [ "$PLATFORM" != "riscv64" ] ; then
		echo ">>> Processing GDB (${PLATFORM})"
		mkdir -p "${GDBDIR}"
		cd "${GDBDIR}"
		check_error $? "Change directory failed."

		change_title "GDB: configure (${PLATFORM})"
		PATH="$PATH:${INSTALL_DIR}/${PREFIX}/bin" "${BASEDIR}/downloads/gdb-${GDB_VERSION}/configure" \
			"--target=${TARGET}" \
			"--prefix=${PREFIX}" "--program-prefix=${TARGET}-" \
			--enable-werror=no --without-guile
		check_error $? "Error configuring GDB."

		change_title "GDB: make (${PLATFORM})"
		PATH="${PATH}:${PREFIX}/bin:${INSTALL_DIR}/${PREFIX}/bin" make all
		check_error $? "Error compiling GDB."

		change_title "GDB: make (${PLATFORM})"
		PATH="${PATH}:${INSTALL_DIR}/${PREFIX}/bin" make install "DESTDIR=${INSTALL_DIR}"
		check_error $? "Error installing GDB."
	fi

	# Symlink clang and lld to the install path.
	CLANG="`which clang 2> /dev/null || echo "/usr/bin/clang"`"
	LLD="`which ld.lld 2> /dev/null || echo "/usr/bin/ld.lld"`"

	ln -s $CLANG "${INSTALL_DIR}/${PREFIX}/bin/${TARGET}-clang"
	ln -s $LLD "${INSTALL_DIR}/${PREFIX}/bin/${TARGET}-ld.lld"

	if $REAL_INSTALL ; then
		echo ">>> Moving to the destination directory."
		cleanup_dir "${PREFIX}"
		echo mv "${INSTALL_DIR}/${PREFIX}" "${PREFIX}"
		mv "${INSTALL_DIR}/${PREFIX}" "${PREFIX}"
	fi

	cd "${BASEDIR}"
	check_error $? "Change directory failed."

	echo ">>> Cleaning up"
	cleanup_dir "${WORKDIR}"

	echo
	echo ">>> Cross-compiler for ${TARGET} installed."
}

while [ "$#" -gt 1 ] ; do
	case "$1" in
		--no-install)
			REAL_INSTALL=false
			shift
			;;
		--helenos-target)
			USE_HELENOS_TARGET=true
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

case "$1" in
	amd64|arm32|ia32|ia64|mips32|mips32eb|mips64|ppc32|ppc64|riscv64|sparc64)
		prepare
		build_target "$1"
		;;
	"all")
		prepare
		build_target "amd64"
		build_target "arm32"
		build_target "ia32"
		build_target "ia64"
		build_target "mips32"
		build_target "mips32eb"
		build_target "mips64"
		build_target "ppc32"
		build_target "ppc64"
		build_target "riscv64"
		build_target "sparc64"
		;;
	"essential")
		prepare
		build_target "amd64"
		build_target "arm32"
		build_target "ia32"
		build_target "ia64"
		build_target "mips32"
		build_target "mips32eb"
		build_target "ppc32"
		build_target "sparc64"
		;;
	"parallel")
		prepare
		build_target "amd64" &
		build_target "arm32" &
		build_target "ia32" &
		build_target "ia64" &
		build_target "mips32" &
		build_target "mips32eb" &
		build_target "mips64" &
		build_target "ppc32" &
		build_target "ppc64" &
		build_target "riscv64" &
		build_target "sparc64" &
		wait
		;;
	"2-way")
		prepare
		build_target "amd64" &
		build_target "arm32" &
		wait

		build_target "ia32" &
		build_target "ia64" &
		wait

		build_target "mips32" &
		build_target "mips32eb" &
		wait

		build_target "mips64" &
		build_target "ppc32" &
		wait

		build_target "riscv64" &
		build_target "ppc64" &
		wait

		build_target "sparc64" &
		wait
		;;
	*)
		show_usage
		;;
esac
