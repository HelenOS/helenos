#!/bin/sh
#
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
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
	uspace/lib/libcpp.a \
	uspace/lib/libdisplay.a \
	uspace/lib/libgfx.a \
	uspace/lib/libgfxfont.a \
	uspace/lib/libgfximage.a \
	uspace/lib/libhound.a \
	uspace/lib/libipcgfx.a \
	uspace/lib/libmath.a \
	uspace/lib/libmemgfx.a \
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

mkdir -p "$EXPORT_DIR/lib"
cp -t "$EXPORT_DIR/lib" $EXPORT_LIBS
rm -rf "$EXPORT_DIR/include"
cp -R dist/include "$EXPORT_DIR/include"
cp -t "$EXPORT_DIR" $EXPORT_CONFIGS
