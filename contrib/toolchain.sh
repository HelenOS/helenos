#!/bin/bash

# Cross-compiler toolchain build script
#  by Martin Decky <martin@decky.cz>
#
#  GPL'ed, copyleft
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
	echo " amd64      AMD64 (x86_64, x64)"
	echo " arm32      ARM32"
	echo " ia32       IA-32 (x86, i386)"
	echo " ia64       IA-64 (Itanium)"
	echo " mips32     MIPS32 little-endian"
	echo " mips32eb   MIPS32 big-endian"
	echo " ppc32      PowerPC 32"
	echo " ppc64      PowerPC 64"
	echo " sparc64    UltraSPARC 64"
	echo " all        build all targets"
	echo
	
	exit 3
}

download_check() {
	SOURCE="$1"
	FILE="$2"
	CHECKSUM="$3"
	
	if [ ! -f "${FILE}" ]; then
		wget -c "${SOURCE}${FILE}"
		check_error $? "Error downloading ${FILE}."
	fi
	
	check_md5 "${FILE}" "${CHECKSUM}"
}

cleanup_dir() {
	DIR="$1"
	
	if [ -d "${DIR}" ]; then
		echo " >>> Removing ${DIR}"
		rm -fr "${DIR}"
	fi
}

create_dir() {
	DIR="$1"
	DESC="$2"
	
	echo ">>> Creating ${DESC}"
	
	mkdir -p "${DIR}"
	test -d "${DIR}"
	check_error $? "Unable to create ${DIR}."
}

unpack_tarball() {
	FILE="$1"
	DESC="$2"
	
	echo " >>> ${DESC}"
	
	tar -xjf "${FILE}"
	check_error $? "Error unpacking ${DESC}."
}

build_target() {
	PLATFORM="$1"
	TARGET="$2"
	
	BINUTILS_VERSION="2.19.1"
	GCC_VERSION="4.4.1"
	
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
	download_check "${BINUTILS_SOURCE}" "${BINUTILS}" "09a8c5821a2dfdbb20665bc0bd680791"
	download_check "${GCC_SOURCE}" "${GCC_CORE}" "d19693308aa6b2052e14c071111df59f"
	download_check "${GCC_SOURCE}" "${GCC_OBJC}" "f7b2a606394036e81433b2f4c3251cba"
	download_check "${GCC_SOURCE}" "${GCC_CPP}" "d449047b5761348ceec23739f5553e0b"
	
	echo ">>> Removing previous content"
	cleanup_dir "${PREFIX}"
	cleanup_dir "${OBJDIR}"
	cleanup_dir "${BINUTILSDIR}"
	cleanup_dir "${GCCDIR}"
	
	create_dir "${PREFIX}" "destionation directory"
	create_dir "${OBJDIR}" "GCC object directory"
	
	echo ">>> Unpacking tarballs"
	unpack_tarball "${BINUTILS}" "binutils"
	unpack_tarball "${GCC_CORE}" "GCC Core"
	unpack_tarball "${GCC_OBJC}" "Objective C"
	unpack_tarball "${GCC_CPP}" "C++"
	
	echo ">>> Compiling and installing binutils"
	cd "${BINUTILSDIR}"
	check_error $? "Change directory failed."
	./configure "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" "--disable-nls"
	check_error $? "Error configuring binutils."
	make all install
	check_error $? "Error compiling/installing binutils."
	
	echo ">>> Compiling and installing GCC"
	cd "${OBJDIR}"
	check_error $? "Change directory failed."
	"${GCCDIR}/configure" "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --with-gnu-as --with-gnu-ld --disable-nls --disable-threads --enable-languages=c,objc,c++,obj-c++ --disable-multilib --disable-libgcj --without-headers --disable-shared
	check_error $? "Error configuring GCC."
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
