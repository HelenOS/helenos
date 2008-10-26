#!/usr/bin/env python
#
# Copyright (c) 2008 Martin Decky
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
"""
FAT creator
"""

import sys
import os
import random
import xstruct

def align_up(size, alignment):
	"Return size aligned up to alignment"
	
	if (size % alignment == 0):
		return size
	
	return (((size / alignment) + 1) * alignment)

def subtree_size(root, cluster_size):
	"Recursive directory walk and calculate size"
	
	size = 0
	files = 0
	
	for name in os.listdir(root):
		canon = os.path.join(root, name)
		
		if (os.path.isfile(canon)):
			size += align_up(os.path.getsize(canon), cluster_size)
			files += 1
		
		if (os.path.isdir(canon)):
			size += subtree_size(canon, cluster_size)
			files += 1
	
	return size + align_up(files * 32, cluster_size)

def root_entries(root):
	"Return number of root directory entries"
	
	return len(os.listdir(root))

BOOT_SECTOR = """little:
	uint8_t jmp[3]             /* jump instruction */
	char oem[8]                /* OEM string */
	uint16_t sector            /* bytes per sector */
	uint8_t cluster            /* sectors per cluster */
	uint16_t reserved          /* reserved sectors */
	uint8_t fats               /* number of FATs */
	uint16_t rootdir           /* root directory entries */
	uint16_t sectors           /* total number of sectors */
	uint8_t descriptor         /* media descriptor */
	uint16_t fat_sectors       /* sectors per single FAT */
	uint16_t track_sectors     /* sectors per track */
	uint16_t heads             /* number of heads */
	uint32_t hidden            /* hidden sectors */
	uint32_t sectors_big       /* total number of sectors (if sectors == 0) */
	
	/* Extended BIOS Parameter Block */
	uint8_t drive              /* physical drive number */
	padding[1]                 /* reserved (current head) */
	uint8_t extboot_signature  /* extended boot signature */
	uint32_t serial            /* serial number */
	char label[11]             /* volume label */
	char fstype[8]             /* filesystem type */
	padding[448]               /* boot code */
	uint8_t boot_signature[2]  /* boot signature */
"""

def usage(prname):
	"Print usage syntax"
	print prname + " <PATH> <IMAGE>"

def main():
	if (len(sys.argv) < 3):
		usage(sys.argv[0])
		return
	
	path = os.path.abspath(sys.argv[1])
	if (not os.path.isdir(path)):
		print "<PATH> must be a directory"
		return
	
	sector_size = 512
	cluster_size = 4096
	
	root_size = align_up(root_entries(sys.argv[1]) * 32, sector_size)
	size = subtree_size(sys.argv[1], cluster_size)
	fat_size = align_up(size / cluster_size * 2, sector_size)
	
	sectors = (cluster_size + 2 * fat_size + root_size + size) / sector_size
	
	outf = file(sys.argv[2], "w")
	
	boot_sector = xstruct.create(BOOT_SECTOR)
	boot_sector.jmp = [0xEB, 0x3C, 0x90]
	boot_sector.oem = "MSDOS5.0"
	boot_sector.sector = sector_size
	boot_sector.cluster = cluster_size / sector_size
	boot_sector.reserved = cluster_size / sector_size
	boot_sector.fats = 2
	boot_sector.rootdir = root_size / 32
	boot_sector.sectors = (sectors if (sectors <= 65535) else 0)
	boot_sector.descriptor = 0xF8
	boot_sector.fat_sectors = fat_size / sector_size
	boot_sector.track_sectors = 63
	boot_sector.heads = 6
	boot_sector.hidden = 0
	boot_sector.sectors_big = (sectors if (sectors > 65535) else 0)
	
	boot_sector.drive = 0x80
	boot_sector.extboot_signature = 0x29
	boot_sector.serial = random.randint(0, 0xFFFFFFFF)
	boot_sector.label = "HELENOS"
	boot_sector.fstype = "FAT16   "
	boot_sector.boot_signature = [0x55, 0xAA]
	
	outf.write(boot_sector.pack())
	
	outf.close()
	
if __name__ == '__main__':
	main()
