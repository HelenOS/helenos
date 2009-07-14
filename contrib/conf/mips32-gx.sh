#!/bin/sh

DISK_IMG=hdisk.img

# Create a disk image if it does not exist
if [ ! -f "$DISK_IMG" ]; then
	tools/mkfat.py uspace/dist/data "$DISK_IMG"
fi

gxemul $@ -E testmips -C R4000 -X image.boot -M 32 -d d0:"$DISK_IMG"
