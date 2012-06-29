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

#if GCC_GMP_VERSION < GCC_GMP_VERSION_NUM(4,3,2)
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

BINUTILS_VERSION="2.22"
BINUTILS_RELEASE=""
GCC_VERSION="4.7.1"
GDB_VERSION="7.4"

BASEDIR="`pwd`"
BINUTILS="binutils-${BINUTILS_VERSION}${BINUTILS_RELEASE}.tar.bz2"
GCC="gcc-${GCC_VERSION}.tar.bz2"
GDB="gdb-${GDB_VERSION}.tar.bz2"

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

show_usage() {
	echo "Cross-compiler toolchain build script"
	echo
	echo "Syntax:"
	echo " $0 <platform>"
	echo
	echo "Possible target platforms are:"
	echo " amd64      AMD64 (x86-64, x64)"
	echo " arm32      ARM"
	echo " ia32       IA-32 (x86, i386)"
	echo " ia64       IA-64 (Itanium)"
	echo " mips32     MIPS little-endian 32b"
	echo " mips32eb   MIPS big-endian 32b"
	echo " mips64     MIPS little-endian 64b"
	echo " ppc32      32-bit PowerPC"
	echo " ppc64      64-bit PowerPC"
	echo " sparc64    SPARC V9"
	echo " all        build all targets"
	echo " parallel   same as 'all', but all in parallel"
	echo " 2-way      same as 'all', but 2-way parallel"
	echo
	echo "The toolchain will be installed to the directory specified by"
	echo "the CROSS_PREFIX environment variable. If the variable is not"
	echo "defined, /usr/local/cross will be used by default."
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
	echo " - GNU Multiple Precision Library (GMP)"
	echo " - GNU Make"
	echo " - GNU tar"
	echo " - GNU Coreutils"
	echo " - GNU Sharutils"
	echo " - MPFR"
	echo " - MPC"
	echo " - Parma Polyhedra Library (PPL)"
	echo " - ClooG-PPL"
	echo " - native C compiler, assembler and linker"
	echo " - native C library with headers"
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

unpack_tarball() {
	FILE="$1"
	DESC="$2"
	
	change_title "Unpacking ${DESC}"
	echo " >>> Unpacking ${DESC}"
	
	tar -xjf "${FILE}"
	check_error $? "Error unpacking ${DESC}."
}

prepare() {
	show_dependencies
	check_dependecies
	show_countdown 10
	
	BINUTILS_SOURCE="ftp://ftp.gnu.org/gnu/binutils/"
	GCC_SOURCE="ftp://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/"
	GDB_SOURCE="ftp://ftp.gnu.org/gnu/gdb/"
	
	download_fetch "${BINUTILS_SOURCE}" "${BINUTILS}" "ee0f10756c84979622b992a4a61ea3f5"
	download_fetch "${GCC_SOURCE}" "${GCC}" "933e6f15f51c031060af64a9e14149ff"
	download_fetch "${GDB_SOURCE}" "${GDB}" "95a9a8305fed4d30a30a6dc28ff9d060"
}

build_target() {
	PLATFORM="$1"
	TARGET="$2"
	
	WORKDIR="${BASEDIR}/${PLATFORM}"
	BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"
	OBJDIR="${WORKDIR}/gcc-obj"
	GDBDIR="${WORKDIR}/gdb-${GDB_VERSION}"
	
	if [ -z "${CROSS_PREFIX}" ] ; then
		CROSS_PREFIX="/usr/local/cross"
	fi
	
	PREFIX="${CROSS_PREFIX}/${PLATFORM}"
	
	echo ">>> Downloading tarballs"
	source_check "${BASEDIR}/${BINUTILS}"
	source_check "${BASEDIR}/${GCC}"
	source_check "${BASEDIR}/${GDB}"
	
	echo ">>> Removing previous content"
	cleanup_dir "${PREFIX}"
	cleanup_dir "${WORKDIR}"
	
	create_dir "${PREFIX}" "destination directory"
	create_dir "${OBJDIR}" "GCC object directory"
	
	echo ">>> Unpacking tarballs"
	cd "${WORKDIR}"
	check_error $? "Change directory failed."
	
	unpack_tarball "${BASEDIR}/${BINUTILS}" "binutils"
	unpack_tarball "${BASEDIR}/${GCC}" "GCC"
	unpack_tarball "${BASEDIR}/${GDB}" "GDB"
	
	echo ">>> Processing binutils (${PLATFORM})"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."
	
	change_title "binutils: configure (${PLATFORM})"
	CFLAGS=-Wno-error ./configure "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --disable-nls --disable-werror
	check_error $? "Error configuring binutils."
	
	change_title "binutils: make (${PLATFORM})"
	make all install
	check_error $? "Error compiling/installing binutils."
	
	echo ">>> Processing GCC (${PLATFORM})"
	cd "${OBJDIR}"
	check_error $? "Change directory failed."
	
	change_title "GCC: configure (${PLATFORM})"
	"${GCCDIR}/configure" "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --with-gnu-as --with-gnu-ld --disable-nls --disable-threads --enable-languages=c,objc,c++,obj-c++ --disable-multilib --disable-libgcj --without-headers --disable-shared --enable-lto --disable-werror
	check_error $? "Error configuring GCC."
	
	change_title "GCC: make (${PLATFORM})"
	PATH="${PATH}:${PREFIX}/bin" make all-gcc install-gcc
	check_error $? "Error compiling/installing GCC."
	
	echo ">>> Processing GDB (${PLATFORM})"
	cd "${GDBDIR}"
	check_error $? "Change directory failed."
	
	change_title "GDB: configure (${PLATFORM})"
	./configure "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-"
	check_error $? "Error configuring GDB."
	
	change_title "GDB: make (${PLATFORM})"
	make all install
	check_error $? "Error compiling/installing GDB."
	
	cd "${BASEDIR}"
	check_error $? "Change directory failed."
	
	echo ">>> Cleaning up"
	cleanup_dir "${WORKDIR}"
	
	echo
	echo ">>> Cross-compiler for ${TARGET} installed."
}

if [ "$#" -lt "1" ]; then
	show_usage
fi

case "$1" in
	"amd64")
		prepare
		build_target "amd64" "amd64-linux-gnu"
		;;
	"arm32")
		prepare
		build_target "arm32" "arm-linux-gnueabi"
		;;
	"ia32")
		prepare
		build_target "ia32" "i686-pc-linux-gnu"
		;;
	"ia64")
		prepare
		build_target "ia64" "ia64-pc-linux-gnu"
		;;
	"mips32")
		prepare
		build_target "mips32" "mipsel-linux-gnu"
		;;
	"mips32eb")
		prepare
		build_target "mips32eb" "mips-linux-gnu"
		;;
	"mips64")
		prepare
		build_target "mips64" "mips64el-linux-gnu"
		;;
	"ppc32")
		prepare
		build_target "ppc32" "ppc-linux-gnu"
		;;
	"ppc64")
		prepare
		build_target "ppc64" "ppc64-linux-gnu"
		;;
	"sparc64")
		prepare
		build_target "sparc64" "sparc64-linux-gnu"
		;;
	"all")
		prepare
		build_target "amd64" "amd64-linux-gnu"
		build_target "arm32" "arm-linux-gnueabi"
		build_target "ia32" "i686-pc-linux-gnu"
		build_target "ia64" "ia64-pc-linux-gnu"
		build_target "mips32" "mipsel-linux-gnu"
		build_target "mips32eb" "mips-linux-gnu"
		build_target "mips64" "mips64el-linux-gnu"
		build_target "ppc32" "ppc-linux-gnu"
		build_target "ppc64" "ppc64-linux-gnu"
		build_target "sparc64" "sparc64-linux-gnu"
		;;
	"parallel")
		prepare
		build_target "amd64" "amd64-linux-gnu" &
		build_target "arm32" "arm-linux-gnueabi" &
		build_target "ia32" "i686-pc-linux-gnu" &
		build_target "ia64" "ia64-pc-linux-gnu" &
		build_target "mips32" "mipsel-linux-gnu" &
		build_target "mips32eb" "mips-linux-gnu" &
		build_target "mips64" "mips64el-linux-gnu" &
		build_target "ppc32" "ppc-linux-gnu" &
		build_target "ppc64" "ppc64-linux-gnu" &
		build_target "sparc64" "sparc64-linux-gnu" &
		wait
		;;
	"2-way")
		prepare
		build_target "amd64" "amd64-linux-gnu" &
		build_target "arm32" "arm-linux-gnueabi" &
		wait
		
		build_target "ia32" "i686-pc-linux-gnu" &
		build_target "ia64" "ia64-pc-linux-gnu" &
		wait
		
		build_target "mips32" "mipsel-linux-gnu" &
		build_target "mips32eb" "mips-linux-gnu" &
		wait
		
		build_target "mips64" "mips64el-linux-gnu" &
		build_target "ppc32" "ppc-linux-gnu" &
		wait
		
		build_target "ppc64" "ppc64-linux-gnu" &
		build_target "sparc64" "sparc64-linux-gnu" &
		wait
		;;
	*)
		show_usage
		;;
esac
