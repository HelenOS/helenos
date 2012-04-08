#!/bin/sh

DISK_IMG=hdisk.img

# Create a disk image if it does not exist
if [ ! -f "$DISK_IMG" ]; then
	tools/mkfat.py 1048576 uspace/dist/data "$DISK_IMG"
fi

gxemul $@ -E oldtestmips -C R4000 -X image.boot -d d0:"$DISK_IMG"
