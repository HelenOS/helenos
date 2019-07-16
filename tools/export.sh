#!/bin/sh

if [ "$#" -ne 1 ]; then
	echo "Must define export directory."
	exit 1
fi

EXPORT_DIR="$1"

# Only (re)build files we actually want to export.

EXPORT_LIBS=" \
	uspace/lib/libmath.a \
	uspace/lib/libclui.a \
	uspace/lib/libgui.a \
	uspace/lib/libdraw.a \
	uspace/lib/libsoftrend.a \
	uspace/lib/libhound.a \
	uspace/lib/libpcm.a \
	uspace/lib/libcpp.a \
	uspace/lib/libc.a \
	uspace/lib/c/libstartfiles.a \
	uspace/lib/libposix.a \
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
