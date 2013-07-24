#!/bin/sh

if [ -z "${QEMU_BINARY}" ] ; then
	QEMU_BINARY="`which --tty-only qemu 2> /dev/null`"
fi

if [ -z "${QEMU_BINARY}" ] ; then
	QEMU_BINARY="`which --tty-only qemu-system-i386 2> /dev/null`"
fi

if [ -z "${QEMU_BINARY}" ] ; then
	echo "QEMU binary not found"
fi

DISK_IMG=hdisk.img

# Create a disk image if it does not exist
if [ ! -f "$DISK_IMG" ]; then
	tools/mkfat.py 1048576 uspace/dist/data "$DISK_IMG"
fi

"${QEMU_BINARY}" "$@" -m 32 -hda "$DISK_IMG" -cdrom image.iso -boot d
