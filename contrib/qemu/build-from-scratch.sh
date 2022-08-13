#!/bin/bash
#
# SPDX-FileCopyrightText: 2014 Jakub Jermar
#
# SPDX-License-Identifier: BSD-3-Clause
#

VERSION=6.2.0
BASENAME=qemu-${VERSION}
BASENAME_MASTER=qemu-master
TARBALL=${BASENAME}.tar.bz2
SOURCEDIR=${BASENAME}
URL=https://download.qemu.org/${TARBALL}
REPO=git://git.qemu.org/qemu.git

OPENSPARC_TARBALL="OpenSPARCT1_Arch.1.5.tar.bz2"
OPENSPARC_URL="http://download.oracle.com/technetwork/systems/opensparc/${OPENSPARC_TARBALL}"

ARCHIVE_PREFIX="./S10image"
BINARIES="1up-hv.bin 1up-md.bin nvram1 openboot.bin q.bin reset.bin"

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

sudo install -d /usr/local/opensparc/image
sudo install -m 0444 binaries/* /usr/local/opensparc/image

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

	gpg --verify ${TARBALL}.sig ${TARBALL}
	if [ $? -ne 0 ]; then
		echo Unable to verify the signature
		exit
	fi

	tar xvfj ${TARBALL}
	cd ${SOURCEDIR}
fi

echo "==== Configuring QEMU ===="

./configure --target-list=i386-softmmu,x86_64-softmmu,arm-softmmu,aarch64-softmmu,ppc-softmmu,sparc64-softmmu,mips-softmmu,mipsel-softmmu --audio-drv-list=pa

echo "==== Building QEMU ===="

make -j 4

echo "==== Installing QEMU ===="

sudo make install

