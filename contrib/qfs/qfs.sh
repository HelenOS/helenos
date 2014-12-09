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

VERSION=2.2.0
BASENAME=qemu-${VERSION}
BASENAME_MASTER=qemu-master
TARBALL=${BASENAME}.tar.bz2
SOURCEDIR=${BASENAME}
URL=http://wiki.qemu-project.org/download/${TARBALL}
REPO=git://git.qemu.org/qemu.git
MD5="f7a5e2da22d057eb838a91da7aff43c8"

if [ "$1" == "--master" ]; then
	git clone ${REPO} ${BASENAME_MASTER}
	cd ${BASENAME_MASTER}
else
	if [ ! -f ${TARBALL} ]; then
		wget ${URL}
	fi
	
	if [ "`md5sum ${TARBALL} | cut -f 1 -d " "`" != ${MD5} ]; then
		echo Wrong MD5 checksum
		exit
	fi
	
	tar xvfj ${TARBALL}
	cd ${SOURCEDIR}
fi

./configure --target-list=i386-softmmu,x86_64-softmmu,arm-softmmu,ppc-softmmu,sparc-softmmu,sparc64-softmmu,mips-softmmu,mipsel-softmmu --audio-drv-list=pa
make -j 4
sudo make install
