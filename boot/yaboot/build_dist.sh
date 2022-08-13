#!/bin/sh
#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

BOOT_OUTPUT="$1"
SRCDIR="$2"
DESTDIR="$3"

rm -rf ${DESTDIR}

mkdir -p ${DESTDIR}/boot
cp -t ${DESTDIR}/boot \
	${SRCDIR}/ofboot.b \
	${SRCDIR}/yaboot \
	${SRCDIR}/yaboot.conf
cp ${BOOT_OUTPUT} ${DESTDIR}/boot/image.boot

mkdir -p ${DESTDIR}/ppc
cp -t ${DESTDIR}/ppc \
	${SRCDIR}/bootinfo.txt
