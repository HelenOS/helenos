#!/bin/bash

# Cross-Compiler Toolchain for ${PLATFORM}
#  by Martin Decky <martin@decky.cz>
#
#  GPL'ed, copyleft
#


check_error() {
    if [ "$1" -ne "0" ]; then
	echo
	echo "Script failed: $2"
	exit
    fi
}

BINUTILS_VERSION="2.17"
GCC_VERSION="4.1.1"

BINUTILS="binutils-${BINUTILS_VERSION}.tar.gz"
GCC_CORE="gcc-core-${GCC_VERSION}.tar.bz2"
GCC_OBJC="gcc-objc-${GCC_VERSION}.tar.bz2"
GCC_CPP="gcc-g++-${GCC_VERSION}.tar.bz2"

BINUTILS_SOURCE="ftp://ftp.gnu.org/gnu/binutils/"
GCC_SOURCE="ftp://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/"

PLATFORM="sparc64"
WORKDIR=`pwd`
TARGET="${PLATFORM}-linux-gnu"
HOST="i686-pc-linux-gnu"
PREFIX="/usr/local/${PLATFORM}"
BINUTILSDIR="${WORKDIR}/binutils-${BINUTILS_VERSION}"
GCCDIR="${WORKDIR}/gcc-${GCC_VERSION}"
OBJDIR="${WORKDIR}/gcc-obj"

echo ">>> Downloading tarballs"

if [ ! -f "${BINUTILS}" ]; then
    wget -c "${BINUTILS_SOURCE}${BINUTILS}"
    check_error $? "Error downloading binutils."
fi
if [ ! -f "${GCC_CORE}" ]; then
    wget -c "${GCC_SOURCE}${GCC_CORE}"
    check_error $? "Error downloading GCC Core."
fi
if [ ! -f "${GCC_OBJC}" ]; then
    wget -c "${GCC_SOURCE}${GCC_OBJC}"
    check_error $? "Error downloading GCC Objective C."
fi
if [ ! -f "${GCC_CPP}" ]; then
    wget -c "${GCC_SOURCE}${GCC_CPP}"
    check_error $? "Error downloading GCC C++."
fi

echo ">>> Creating destionation directory"
if [ ! -d "${PREFIX}" ]; then
    mkdir -p "${PREFIX}" 
    test -d "${PREFIX}"
    check_error $? "Unable to create ${PREFIX}."
fi

echo ">>> Creating GCC work directory"
if [ ! -d "${OBJDIR}" ]; then
    mkdir -p "${OBJDIR}"
    test -d "${OBJDIR}"
    check_error $? "Unable to create ${OBJDIR}."
fi

echo ">>> Unpacking tarballs"
tar -xvzf "${BINUTILS}"
check_error $? "Error unpacking binutils."
tar -xvjf "${GCC_CORE}"
check_error $? "Error unpacking GCC Core."
tar -xvjf "${GCC_OBJC}"
check_error $? "Error unpacking GCC Objective C."
tar -xvjf "${GCC_CPP}"
check_error $? "Error unpacking GCC C++."

echo ">>> Compiling and installing binutils"
cd "${BINUTILSDIR}"
check_error $? "Change directory failed."
./configure "--host=${HOST}" "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" "--disable-nls"
check_error $? "Error configuring binutils."
make all install
check_error $? "Error compiling/installing binutils."

echo ">>> Compiling and installing GCC"
cd "${OBJDIR}"
check_error $? "Change directory failed."
"${GCCDIR}/configure" "--host=${HOST}" "--target=${TARGET}" "--prefix=${PREFIX}" "--program-prefix=${TARGET}-" --with-gnu-as --with-gnu-ld --disable-nls --disable-threads --enable-languages=c,objc,c++,obj-c++ --disable-multilib --disable-libgcj --without-headers --disable-shared
check_error $? "Error configuring GCC."
PATH="${PATH}:${PREFIX}/bin" make all-gcc install-gcc all-target-libobjc install-target-libobjc
check_error $? "Error compiling/installing GCC."

echo
echo ">>> Cross-compiler for ${TARGET} installed."
