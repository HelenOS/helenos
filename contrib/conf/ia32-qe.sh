#!/bin/sh

DISK_IMG=hdisk.img

# Create a disk image if it does not exist
if [ ! -f "$DISK_IMG" ]; then
	tools/mkfat.py 1048576 uspace/dist/data "$DISK_IMG"
fi

qemu $@ -m 32 -hda "$DISK_IMG" -cdrom image.iso -boot d
