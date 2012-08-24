#!/bin/sh

DISK_IMG=hdisk.img

# Create a disk image if it does not exist
if [ ! -f "$DISK_IMG" ]; then
	tools/mkfat.py 1048576 uspace/dist/data "$DISK_IMG"
fi

if [ "$1" == "-E" ] && [ -n "$2" ]; then
	MACHINE="$2"
	shift 2
else
	MACHINE="oldtestmips"
fi

gxemul $@ -E "$MACHINE" -C R4000 -X image.boot -d d0:"$DISK_IMG"
