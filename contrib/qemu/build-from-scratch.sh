#!/bin/bash

#
# Copyright (c) 2014 Jakub Jermar
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

VERSION=9.1.0
BASENAME=qemu-${VERSION}
BASENAME_MASTER=qemu-master
TARBALL=${BASENAME}.tar.bz2
SOURCEDIR=${BASENAME}
URL=https://download.qemu.org/${TARBALL}
REPO=git@github.com:qemu/qemu.git

OPENSPARC_TARBALL="OpenSPARCT1_Arch.1.5.tar.bz2"
OPENSPARC_URL="http://download.oracle.com/technetwork/systems/opensparc/${OPENSPARC_TARBALL}"

ARCHIVE_PREFIX="./S10image"
BINARIES="1up-hv.bin 1up-md.bin nvram1 openboot.bin q.bin reset.bin"
INSTALL_PREFIX="$HOME/.local"

echo "==== Downloading OpenSPARC archive ===="

if [ ! -f ${OPENSPARC_TARBALL} ]
then
    wget ${OPENSPARC_URL}
else
    echo "===== OpenSPARC archive already exists, skipping. ====="
fi

echo "==== Extracting OpenSPARC binaries ===="
(
    mkdir -p binaries;

    BINLIST=""
    for b in ${BINARIES};
    do
	if [ ! -f binaries/$b ];
	then
	    BINLIST+=${ARCHIVE_PREFIX}/$b" "
	else
	    echo "===== $b seems to be already extracted, skipping. ====="
	fi
    done

    cd binaries

    if [ "${BINLIST}x" != "x" ];
    then
	tar --strip-components=2 -xjf ../${OPENSPARC_TARBALL} ${BINLIST}
    fi
)

echo "==== Installing OpenSPARC binaries ===="

install -d "$INSTALL_PREFIX/opensparc/image"
install -m 0444 binaries/* "$INSTALL_PREFIX/opensparc/image"

echo "==== Obtaining QEMU sources ===="

if [ "$1" == "--master" ]; then
	git clone ${REPO} ${BASENAME_MASTER}
	cd ${BASENAME_MASTER}
else
	if [ ! -f ${TARBALL} ]; then
		wget ${URL}
	fi

	if [ ! -f ${TARBALL}.sig ]; then
		wget ${URL}.sig
	fi

	gpg --auto-key-retrieve --verify ${TARBALL}.sig ${TARBALL}
	if [ $? -ne 0 ]; then
		echo Unable to verify the signature
		exit
	fi

	echo "==== Decompressing QEMU sources ===="
	tar xfj ${TARBALL}
	cd ${SOURCEDIR}
fi

echo "==== Configuring QEMU ===="

./configure --target-list=i386-softmmu,x86_64-softmmu,arm-softmmu,aarch64-softmmu,ppc-softmmu,sparc64-softmmu,mips-softmmu,mipsel-softmmu --enable-gtk --enable-vte --enable-kvm --enable-curses --enable-opengl --enable-slirp --enable-pa --audio-drv-list=pa --prefix="$INSTALL_PREFIX" || exit 1

echo "==== Building QEMU ===="

make -j`nproc` || exit 1

echo "==== Installing QEMU ===="

make install
