#!/bin/bash

#
# Copyright (c) 2016 Jakub Jermar
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

REPO="http://repo.gem5.org/gem5"
OPENSPARC_TARBALL="OpenSPARCT1_Arch.1.5.tar.bz2"
OPENSPARC_URL="http://download.oracle.com/technetwork/systems/opensparc/${OPENSPARC_TARBALL}"

ARCHIVE_PREFIX="./S10image"
BINARIES="1up-hv.bin 1up-md.bin nvram1 openboot.bin q.bin reset.bin"
RENAMES="openboot.bin q.bin reset.bin"

echo "==== Cloning gem5 repository ===="

if [ ! -d gem5 ];
then
    hg clone ${REPO}
else
    echo "===== Directory gem5 already exists, skipping. ====="
fi

echo "==== Building gem5.fast (be patient, this may take a while) ===="

if [ ! -e gem5/build/SPARC/gem5.fast ];
then
    (cd gem5; scons build/SPARC/gem5.fast --ignore-style -j 4)
else
    echo "===== SPARC emulator binary already exists, skipping. ====="
fi

echo "==== Building m5term ===="

if [ ! -e gem5/util/term/m5term ];
then
    (cd gem5/util/term; make)
else
    echo "===== m5term binary already exists, skipping. ======"
fi

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
	tar --strip-components=2 -xjf ../OpenSPARCT1_Arch.1.5.tar.bz2 ${BINLIST}
    fi

    for r in ${RENAMES};
    do
	NNAME=`basename $r .bin`_new.bin
	if [ ! -f ${NNAME} ];
	then
	    ln -s $r ${NNAME};
	else
	    echo "===== $r seems to be already linked, skipping. ====="
	fi
    done
)

echo "==== Copying configs ===="

cp -r gem5/configs .

echo "==== Applying patches to the example configuration ===="

patch -p 1 <disk-image.patch
