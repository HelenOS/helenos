#!/bin/sh

#
# Copyright (c) 2019 Jiří Zárevúcky
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

if [ "$#" -ne 1 ]; then
	echo "Must define export directory."
	exit 1
fi

EXPORT_DIR="$1"

# Only (re)build files we actually want to export.

EXPORT_LIBS=" \
	uspace/lib/c/libstartfiles.a \
	uspace/lib/libclui.a \
	uspace/lib/libc.a \
	uspace/lib/libcongfx.a \
	uspace/lib/libconsole.a \
	uspace/lib/libcpp.a \
	uspace/lib/libdisplay.a \
	uspace/lib/libgfx.a \
	uspace/lib/libgfxfont.a \
	uspace/lib/libgfximage.a \
	uspace/lib/libinput.a \
	uspace/lib/libhound.a \
	uspace/lib/libipcgfx.a \
	uspace/lib/libmath.a \
	uspace/lib/libmemgfx.a \
	uspace/lib/liboutput.a \
	uspace/lib/libpcm.a \
	uspace/lib/libpixconv.a \
	uspace/lib/libposix.a \
	uspace/lib/libriff.a \
	uspace/lib/libui.a \
"

EXPORT_CONFIGS=" \
	meson/part/exports/config.mk \
	meson/part/exports/config.sh \
"

ninja $EXPORT_LIBS $EXPORT_CONFIGS
ninja devel-headers

#
# Meson may produce thin archives. These cannot be simply copied to another
# location. Copy them using ar instead, converting them to regular,
# non-thin archives in the process.
#
mkdir -p "$EXPORT_DIR/lib"
for lpath in $EXPORT_LIBS; do
	dest="$EXPORT_DIR/lib/$(basename $lpath)"
	ar -t $lpath | xargs ar crs $dest
done

rm -rf "$EXPORT_DIR/include"
cp -R dist/include "$EXPORT_DIR/include"
cp -t "$EXPORT_DIR" $EXPORT_CONFIGS
