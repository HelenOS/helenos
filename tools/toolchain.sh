#! /bin/sh

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
	echo " mips32     MIPS little-endian"
	echo " mips32eb   MIPS big-endian"
	echo " ppc32      32-bit PowerPC"
	echo " ppc64      64-bit PowerPC"
	echo " sparc64    SPARC V9"
	echo " all        build all targets"
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
	
	show_countdown 10
}

download_check() {
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

patch_binutils() {
	PLATFORM="$1"
	
	if [ "${PLATFORM}" == "arm32" ] ; then
		patch -p1 <<EOF
diff -Naur binutils-2.20.orig/gas/config/tc-arm.c binutils-2.20/gas/config/tc-arm.c
--- binutils-2.20.orig/gas/config/tc-arm.c	2009-08-30 00:10:59.000000000 +0200
+++ binutils-2.20/gas/config/tc-arm.c	2009-11-02 14:25:11.000000000 +0100
@@ -2485,8 +2485,9 @@
       know (frag->tc_frag_data.first_map == NULL);
       frag->tc_frag_data.first_map = symbolP;
     }
-  if (frag->tc_frag_data.last_map != NULL)
+  if (frag->tc_frag_data.last_map != NULL) {
     know (S_GET_VALUE (frag->tc_frag_data.last_map) < S_GET_VALUE (symbolP));
+  }
   frag->tc_frag_data.last_map = symbolP;
 }
EOF
		check_error $? "Error patching binutils"
	fi
}

build_target() {
	PLATFORM="$1"
	TARGET="$2"
	
	BINUTILS_VERSION="2.20"
	GCC_VERSION="4.5.1"
	
	BINUTILS="binutils-${BINUTILS_VERSION}.tar.bz2"
	GCC_CORE="gcc-core-${GCC_VERSION}.tar.bz2"
	GCC_OBJC="gcc-objc-${GCC_VERSION}.tar.bz2"
	GCC_CPP="gcc-g++-${GCC_VERSION}.tar.bz2"
	
	BINUTILS_SOURCE="ftp://ftp.gnu.org/gnu/binutils/"
	GCC_SOURCE="ftp://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/"
	
	WORKDIR="`pwd`"
	BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"
	GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"
	OBJDIR="${WORKDIR}/gcc-obj"
	
	if [ -z "${CROSS_PREFIX}" ] ; then
		CROSS_PREFIX="/usr/local"
	fi
	
	PREFIX="${CROSS_PREFIX}/${PLATFORM}"
	
	echo ">>> Downloading tarballs"
	download_check "${BINUTILS_SOURCE}" "${BINUTILS}" "ee2d3e996e9a2d669808713360fa96f8"
	download_check "${GCC_SOURCE}" "${GCC_CORE}" "dc8959e31b01a65ce10d269614815054"
	download_check "${GCC_SOURCE}" "${GCC_OBJC}" "3c11b7037896e967eddf8178af2ddd98"
	download_check "${GCC_SOURCE}" "${GCC_CPP}" "b294953ff0bb2f20c7acb2bf005d832a"
	
	echo ">>> Removing previous content"
	cleanup_dir "${PREFIX}"
	cleanup_dir "${OBJDIR}"
	cleanup_dir "${BINUTILSDIR}"
	cleanup_dir "${GCCDIR}"
	
	create_dir "${PREFIX}" "destination directory"
	create_dir "${OBJDIR}" "GCC object directory"
	
	echo ">>> Unpacking tarballs"
	unpack_tarball "${BINUTILS}" "binutils"
	unpack_tarball "${GCC_CORE}" "GCC Core"
	unpack_tarball "${GCC_OBJC}" "Objective C"
	unpack_tarball "${GCC_CPP}" "C++"
	
	echo ">>> Processing binutils (${PLATFORM})"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."
	patch_binutils "${PLATFORM}"
	
	change_title "binutils: configure (${PLATFORM})"
	./configure "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --disable-nls
	check_error $? "Error configuring binutils."
	
	change_title "binutils: make (${PLATFORM})"
	make all install
	check_error $? "Error compiling/installing binutils."
	
	echo ">>> Processing GCC (${PLATFORM})"
	cd "${OBJDIR}"
	check_error $? "Change directory failed."
	
	change_title "GCC: configure (${PLATFORM})"
	"${GCCDIR}/configure" "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --with-gnu-as --with-gnu-ld --disable-nls --disable-threads --enable-languages=c,objc,c++,obj-c++ --disable-multilib --disable-libgcj --without-headers --disable-shared --enable-lto
	check_error $? "Error configuring GCC."
	
	change_title "GCC: make (${PLATFORM})"
	PATH="${PATH}:${PREFIX}/bin" make all-gcc install-gcc
	check_error $? "Error compiling/installing GCC."
	
	cd "${WORKDIR}"
	check_error $? "Change directory failed."
	
	echo ">>> Cleaning up"
	cleanup_dir "${OBJDIR}"
	cleanup_dir "${BINUTILSDIR}"
	cleanup_dir "${GCCDIR}"
	
	echo
	echo ">>> Cross-compiler for ${TARGET} installed."
}

if [ "$#" -lt "1" ]; then
	show_usage
fi

show_dependencies

case "$1" in
	"amd64")
		build_target "amd64" "amd64-linux-gnu"
		;;
	"arm32")
		build_target "arm32" "arm-linux-gnu"
		;;
	"ia32")
		build_target "ia32" "i686-pc-linux-gnu"
		;;
	"ia64")
		build_target "ia64" "ia64-pc-linux-gnu"
		;;
	"ia64")
		build_target "ia64" "ia64-pc-linux-gnu"
		;;
	"mips32")
		build_target "mips32" "mipsel-linux-gnu"
		;;
	"mips32eb")
		build_target "mips32eb" "mips-linux-gnu"
		;;
	"ppc32")
		build_target "ppc32" "ppc-linux-gnu"
		;;
	"ppc64")
		build_target "ppc64" "ppc64-linux-gnu"
		;;
	"sparc64")
		build_target "sparc64" "sparc64-linux-gnu"
		;;
	"all")
		build_target "amd64" "amd64-linux-gnu"
		build_target "arm32" "arm-linux-gnu"
		build_target "ia32" "i686-pc-linux-gnu"
		build_target "ia64" "ia64-pc-linux-gnu"
		build_target "ia64" "ia64-pc-linux-gnu"
		build_target "mips32" "mipsel-linux-gnu"
		build_target "mips32eb" "mips-linux-gnu"
		build_target "ppc32" "ppc-linux-gnu"
		build_target "ppc64" "ppc64-linux-gnu"
		build_target "sparc64" "sparc64-linux-gnu"
		;;
	*)
		show_usage
		;;
esac
