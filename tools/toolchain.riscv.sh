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

GMP_MAIN=<<EOF
#define GCC_GMP_VERSION_NUM(a, b, c) \
	(((a) << 16L) | ((b) << 8) | (c))

#define GCC_GMP_VERSION \
	GCC_GMP_VERSION_NUM(__GNU_MP_VERSION, __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL)

#if GCC_GMP_VERSION < GCC_GMP_VERSION_NUM(4, 3, 2)
	choke me
#endif
EOF

MPFR_MAIN=<<EOF
#if MPFR_VERSION < MPFR_VERSION_NUM(2, 4, 2)
	choke me
#endif
EOF

MPC_MAIN=<<EOF
#if MPC_VERSION < MPC_VERSION_NUM(0, 8, 1)
	choke me
#endif
EOF

ISL_MAIN=<<EOF
isl_ctx_get_max_operations (isl_ctx_alloc ());
EOF

BINUTILS_VERSION="2.25.1"
BINUTILS_RELEASE=""
GCC_VERSION="5.3.0"

BASEDIR="`pwd`"
SRCDIR="$(readlink -f $(dirname "$0"))"
BINUTILS="riscv-binutils-${BINUTILS_VERSION}${BINUTILS_RELEASE}.tar.bz2"
GCC="riscv-gcc-${GCC_VERSION}.tar.bz2"

REAL_INSTALL=true
USE_HELENOS_TARGET=false
INSTALL_DIR="${BASEDIR}/PKG"

#
# Check if the library described in the argument
# exists and has acceptable version.
#
check_dependency() {
	DEPENDENCY="$1"
	HEADER="$2"
	BODY="$3"
	
	FNAME="/tmp/conftest-$$"
	
	echo "#include ${HEADER}" > "${FNAME}.c"
	echo >> "${FNAME}.c"
	echo "int main()" >> "${FNAME}.c"
	echo "{" >> "${FNAME}.c"
	echo "${BODY}" >> "${FNAME}.c"
	echo "	return 0;" >> "${FNAME}.c"
	echo "}" >> "${FNAME}.c"
	
	cc -c -o "${FNAME}.o" "${FNAME}.c" 2> "${FNAME}.log"
	RC="$?"
	
	if [ "$RC" -ne "0" ] ; then
		echo " ${DEPENDENCY} not found, too old or compiler error."
		echo " Please recheck manually the source file \"${FNAME}.c\"."
		echo " The compilation of the toolchain is probably going to fail,"
		echo " you have been warned."
		echo
		echo " ===== Compiler output ====="
		cat "${FNAME}.log"
		echo " ==========================="
		echo
	else
		echo " ${DEPENDENCY} found"
		rm -f "${FNAME}.log" "${FNAME}.o" "${FNAME}.c"
	fi
}

check_dependecies() {
	echo ">>> Basic dependency check"
	check_dependency "GMP" "<gmp.h>" "${GMP_MAIN}"
	check_dependency "MPFR" "<mpfr.h>" "${MPFR_MAIN}"
	check_dependency "MPC" "<mpc.h>" "${MPC_MAIN}"
	check_dependency "isl" "<isl/ctx.h>" "${ISL_MAIN}"
	echo
}

check_error() {
	if [ "$1" -ne "0" ]; then
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
	echo " - GNU Multiple Precision Library (GMP)"
	echo " - MPFR"
	echo " - MPC"
	echo " - integer point manipulation library (isl)"
	echo " - native C and C++ compiler, assembler and linker"
	echo " - native C and C++ standard library with headers"
	echo
}

download_fetch() {
	SOURCE="$1"
	FILE="$2"
	CHECKSUM="$3"
	
	if [ ! -f "${FILE}" ]; then
		change_title "Downloading ${FILE}"
		wget -c "${SOURCE}${FILE}"
		check_error $? "Error downloading ${FILE}."
	fi
	
	check_md5 "${FILE}" "${CHECKSUM}"
}

source_check() {
	FILE="$1"
	
	if [ ! -f "${FILE}" ]; then
		echo
		echo "File ${FILE} not found."
		
		exit 4
	fi
}

cleanup_dir() {
	DIR="$1"
	
	if [ -d "${DIR}" ]; then
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

prepare() {
	show_dependencies
	check_dependecies
	show_countdown 10
	
	BINUTILS_SOURCE="http://gist.decky.cz/"
	GCC_SOURCE="http://gist.decky.cz/"
	
	download_fetch "${BINUTILS_SOURCE}" "${BINUTILS}" "62a8b269139e9625a23dc3174809edfa"
	download_fetch "${GCC_SOURCE}" "${GCC}" "aaf5fa585a3328509c23ef6fbfba78fb"
}

set_target_from_platform() {
	case "$1" in
		"riscv64")
			LINUX_TARGET="riscv64-unknown-linux-gnu"
			HELENOS_TARGET="riscv64-helenos"
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
	if $USE_HELENOS_TARGET; then
		TARGET="$HELENOS_TARGET"
	else
		TARGET="$LINUX_TARGET"
	fi
	
	WORKDIR="${BASEDIR}/${PLATFORM}"
	BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"
	OBJDIR="${WORKDIR}/gcc-obj"
	
	if [ -z "${CROSS_PREFIX}" ] ; then
		CROSS_PREFIX="/usr/local/cross"
	fi
	if [ -z "${CROSS_HELENOS_PREFIX}" ] ; then
		CROSS_HELENOS_PREFIX="/usr/local/cross-helenos"
	fi
	
	if $USE_HELENOS_TARGET; then
		PREFIX="${CROSS_HELENOS_PREFIX}/${PLATFORM}"
	else
		PREFIX="${CROSS_PREFIX}/${PLATFORM}"
	fi
	
	echo ">>> Downloading tarballs"
	source_check "${BASEDIR}/${BINUTILS}"
	source_check "${BASEDIR}/${GCC}"
	
	echo ">>> Removing previous content"
	$REAL_INSTALL && cleanup_dir "${PREFIX}"
	cleanup_dir "${WORKDIR}"
	
	$REAL_INSTALL && create_dir "${PREFIX}" "destination directory"
	create_dir "${OBJDIR}" "GCC object directory"
	
	check_dirs "${PREFIX}" "${WORKDIR}"
	
	echo ">>> Unpacking tarballs"
	cd "${WORKDIR}"
	check_error $? "Change directory failed."
	
	unpack_tarball "${BASEDIR}/${BINUTILS}" "binutils"
	unpack_tarball "${BASEDIR}/${GCC}" "GCC"
	
	echo ">>> Processing binutils (${PLATFORM})"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."
	
	change_title "binutils: configure (${PLATFORM})"
	CFLAGS=-Wno-error ./configure \
		"--target=${TARGET}" \
		"--prefix=${PREFIX}" "--program-prefix=${TARGET}-" \
		--disable-nls --disable-werror
	check_error $? "Error configuring binutils."
	
	change_title "binutils: make (${PLATFORM})"
	make all
	check_error $? "Error compiling binutils."
	
	change_title "binutils: install (${PLATFORM})"
	if $REAL_INSTALL; then
		make install
	else
		make install "DESTDIR=${INSTALL_DIR}"
	fi
	check_error $? "Error installing binutils."
	
	
	echo ">>> Processing GCC (${PLATFORM})"
	cd "${OBJDIR}"
	check_error $? "Change directory failed."
	
	change_title "GCC: configure (${PLATFORM})"
	PATH="$PATH:${INSTALL_DIR}/${PREFIX}/bin" "${GCCDIR}/configure" \
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
	if $REAL_INSTALL; then
		PATH="${PATH}:${PREFIX}/bin" make install-gcc
	else
		PATH="${PATH}:${INSTALL_DIR}/${PREFIX}/bin" make install-gcc "DESTDIR=${INSTALL_DIR}"
	fi
	check_error $? "Error installing GCC."
	
	
	cd "${BASEDIR}"
	check_error $? "Change directory failed."
	
	echo ">>> Cleaning up"
	cleanup_dir "${WORKDIR}"
	
	echo
	echo ">>> Cross-compiler for ${TARGET} installed."
}

while [ "$#" -gt 1 ]; do
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

prepare
build_target riscv64
