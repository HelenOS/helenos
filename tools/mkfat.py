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
import array
from imgutil import *

def subtree_size(root, cluster_size, dirent_size):
	"Recursive directory walk and calculate size"
	
	size = 0
	files = 2
	
	for item in listdir_items(root):
		if item.is_file:
			size += align_up(item.size, cluster_size)
			files += 1
		elif item.is_dir:
			size += subtree_size(item.path, cluster_size, dirent_size)
			files += 1
	
	return size + align_up(files * dirent_size, cluster_size)

def root_entries(root):
	"Return number of root directory entries"
	
	return len(os.listdir(root))

def write_file(item, outf, cluster_size, data_start, fat, reserved_clusters):
	"Store the contents of a file"
	
	prev = -1
	first = 0
	
	for data in chunks(item, cluster_size):
		empty_cluster = fat.index(0)
		fat[empty_cluster] = 0xffff
		
		if (prev != -1):
			fat[prev] = empty_cluster
		else:
			first = empty_cluster
		
		prev = empty_cluster
		
		outf.seek(data_start + (empty_cluster - reserved_clusters) * cluster_size)
		outf.write(data)
	
	return first, item.size

def write_directory(directory, outf, cluster_size, data_start, fat, reserved_clusters, dirent_size, empty_cluster):
	"Store the contents of a directory"
	
	length = len(directory)
	size = length * dirent_size
	prev = -1
	first = 0
	
	i = 0
	rd = 0;
	while (rd < size):
		if (prev != -1):
			empty_cluster = fat.index(0)
			fat[empty_cluster] = 0xffff
			fat[prev] = empty_cluster
		else:
			first = empty_cluster
		
		prev = empty_cluster
		
		data = bytes()
		data_len = 0
		while ((i < length) and (data_len < cluster_size)):
			if (i == 0):
				directory[i].cluster = empty_cluster
			
			data += directory[i].pack()
			data_len += dirent_size
			i += 1
		
		outf.seek(data_start + (empty_cluster - reserved_clusters) * cluster_size)
		outf.write(data)
		rd += len(data)
	
	return first, size

DIR_ENTRY = """little:
	char name[8]               /* file name */
	char ext[3]                /* file extension */
	uint8_t attr               /* file attributes */
	uint8_t lcase              /* file name case (NT extension) */
	uint8_t ctime_fine         /* create time (fine resolution) */
	uint16_t ctime             /* create time */
	uint16_t cdate             /* create date */
	uint16_t adate             /* access date */
	padding[2]                 /* EA-index */
	uint16_t mtime             /* modification time */
	uint16_t mdate             /* modification date */
	uint16_t cluster           /* first cluster */
	uint32_t size              /* file size */
"""

DOT_DIR_ENTRY = """little:
	uint8_t signature          /* 0x2e signature */
	char name[7]               /* empty */
	char ext[3]                /* empty */
	uint8_t attr               /* file attributes */
	padding[1]                 /* reserved for NT */
	uint8_t ctime_fine         /* create time (fine resolution) */
	uint16_t ctime             /* create time */
	uint16_t cdate             /* create date */
	uint16_t adate             /* access date */
	padding[2]                 /* EA-index */
	uint16_t mtime             /* modification time */
	uint16_t mdate             /* modification date */
	uint16_t cluster           /* first cluster */
	uint32_t size              /* file size */
"""

DOTDOT_DIR_ENTRY = """little:
	uint8_t signature[2]       /* 0x2e signature */
	char name[6]               /* empty */
	char ext[3]                /* empty */
	uint8_t attr               /* file attributes */
	padding[1]                 /* reserved for NT */
	uint8_t ctime_fine         /* create time (fine resolution) */
	uint16_t ctime             /* create time */
	uint16_t cdate             /* create date */
	uint16_t adate             /* access date */
	padding[2]                 /* EA-index */
	uint16_t mtime             /* modification time */
	uint16_t mdate             /* modification date */
	uint16_t cluster           /* first cluster */
	uint32_t size              /* file size */
"""

LFN_ENTRY = """little:
	uint8_t pos
	uint16_t name1[5]
	uint8_t attr
	uint8_t type
	uint8_t csum
	uint16_t name2[6]
	uint16_t fc
	uint16_t name3[2]
"""

# Global variable to hold the file names in 8.3 format. Needed to 
# keep track of "number" when creating a short fname from a LFN.
name83_list = []

def name83(fname):
	"Create a 8.3 name for the given fname"

	# FIXME: filter illegal characters
	parts = fname.split('.')
	
	name = ''
	ext = ''
	lfn = False

	if len(fname) > 11 :
		lfn = True

	if len(parts) > 0:
		name = parts[0]
		if len(name) > 8 :
			lfn = True

	if len(parts) > 1 :
		ext = parts[-1]
		if len(ext) > 3 :
			lfn = True

	if len(parts) > 2 :
		lfn = True

	if (lfn == False) :
		return (name.ljust(8)[0:8], ext.ljust(3)[0:3], False)

	# For filenames with multiple extensions, we treat the last one
	# as the actual extension. The rest of the filename is stripped
	# of dots and concatenated to form the short name
	for _name in parts[1:-1]:
		name = name + _name		

	global name83_list
	for number in range(1, 10000) :
		number_str = '~' + str(number)

		if len(name) + len(number_str) > 8 :
			name = name[0:8 - len(number_str)]

		name = name + number_str;

		if (name + ext) not in name83_list :
			break
			
	name83_list.append(name + ext)	

	return (name.ljust(8)[0:8], ext.ljust(3)[0:3], True)

def get_utf16(name, l) :
	"Create a int array out of a string which we can store in uint16_t arrays"

	bs = [0xFFFF for i in range(l)]

	for i in range(len(name)) :
		bs[i] = ord(name[i])
	
	if (len(name) < l) :
		bs[len(name)] = 0;
	
	return bs

def create_lfn_entry((name, index)) :
	entry = xstruct.create(LFN_ENTRY)

	entry.name1 = get_utf16(name[0:5], 5)
	entry.name2 = get_utf16(name[5:11], 6)
	entry.name3 = get_utf16(name[11:13], 2)
	entry.pos = index

	entry.attr = 0xF
	entry.fc = 0
	entry.type = 0

	return entry

def create_dirent(name, directory, cluster, size):
	
	dir_entry = xstruct.create(DIR_ENTRY)
	
	dir_entry.name, dir_entry.ext, lfn = name83(name)

	dir_entry.name = dir_entry.name.upper().encode('ascii')
	dir_entry.ext = dir_entry.ext.upper().encode('ascii')

	if (directory):
		dir_entry.attr = 0x30
	else:
		dir_entry.attr = 0x20
	
	dir_entry.lcase = 0x18
	dir_entry.ctime_fine = 0 # FIXME
	dir_entry.ctime = 0 # FIXME
	dir_entry.cdate = 0 # FIXME
	dir_entry.adate = 0 # FIXME
	dir_entry.mtime = 0 # FIXME
	dir_entry.mdate = 0 # FIXME
	dir_entry.cluster = cluster
	
	if (directory):
		dir_entry.size = 0
	else:
		dir_entry.size = size
	

	if not lfn:
		return [dir_entry]

	n = len(name) / 13 + 1
	names = [(name[i * 13: (i + 1) * 13 + 1], i + 1) for i in range(n)]

	entries = sorted(map (create_lfn_entry, names), reverse = True, key = lambda e : e.pos)
	entries[0].pos |= 0x40

	fname11 = dir_entry.name + dir_entry.ext

	csum = 0
	for i in range(0, 11) :
		csum = ((csum & 1) << 7) + (csum  >> 1) + ord(fname11[i])
		csum = csum & 0xFF
	
	for e in entries :
		e.csum = csum;
	
	entries.append(dir_entry)

	return entries

def create_dot_dirent(empty_cluster):
	dir_entry = xstruct.create(DOT_DIR_ENTRY)
	
	dir_entry.signature = 0x2e
	dir_entry.name = b'       '
	dir_entry.ext = b'   '
	dir_entry.attr = 0x10
	
	dir_entry.ctime_fine = 0 # FIXME
	dir_entry.ctime = 0 # FIXME
	dir_entry.cdate = 0 # FIXME
	dir_entry.adate = 0 # FIXME
	dir_entry.mtime = 0 # FIXME
	dir_entry.mdate = 0 # FIXME
	dir_entry.cluster = empty_cluster
	dir_entry.size = 0
	
	return dir_entry

def create_dotdot_dirent(parent_cluster):
	dir_entry = xstruct.create(DOTDOT_DIR_ENTRY)
	
	dir_entry.signature = [0x2e, 0x2e]
	dir_entry.name = b'      '
	dir_entry.ext = b'   '
	dir_entry.attr = 0x10
	
	dir_entry.ctime_fine = 0 # FIXME
	dir_entry.ctime = 0 # FIXME
	dir_entry.cdate = 0 # FIXME
	dir_entry.adate = 0 # FIXME
	dir_entry.mtime = 0 # FIXME
	dir_entry.mdate = 0 # FIXME
	dir_entry.cluster = parent_cluster
	dir_entry.size = 0
	
	return dir_entry

def recursion(head, root, outf, cluster_size, root_start, data_start, fat, reserved_clusters, dirent_size, parent_cluster):
	"Recursive directory walk"
	
	directory = []
	
	if (not head):
		# Directory cluster preallocation
		empty_cluster = fat.index(0)
		fat[empty_cluster] = 0xffff
		
		directory.append(create_dot_dirent(empty_cluster))
		directory.append(create_dotdot_dirent(parent_cluster))
	else:
		empty_cluster = 0
	
	for item in listdir_items(root):		
		if item.is_file:
			rv = write_file(item, outf, cluster_size, data_start, fat, reserved_clusters)
			directory.extend(create_dirent(item.name, False, rv[0], rv[1]))
		elif item.is_dir:
			rv = recursion(False, item.path, outf, cluster_size, root_start, data_start, fat, reserved_clusters, dirent_size, empty_cluster)
			directory.extend(create_dirent(item.name, True, rv[0], rv[1]))
	
	if (head):
		outf.seek(root_start)
		for dir_entry in directory:
			outf.write(dir_entry.pack())
	else:
		return write_directory(directory, outf, cluster_size, data_start, fat, reserved_clusters, dirent_size, empty_cluster)

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

EMPTY_SECTOR = """little:
	padding[512]               /* empty sector data */
"""

FAT_ENTRY = """little:
	uint16_t next              /* FAT16 entry */
"""

def usage(prname):
	"Print usage syntax"
	print(prname + " <EXTRA_BYTES> <PATH> <IMAGE>")

def main():
	if (len(sys.argv) < 4):
		usage(sys.argv[0])
		return
	
	if (not sys.argv[1].isdigit()):
		print("<EXTRA_BYTES> must be a number")
		return
	
	extra_bytes = int(sys.argv[1])
	
	path = os.path.abspath(sys.argv[2])
	if (not os.path.isdir(path)):
		print("<PATH> must be a directory")
		return
	
	fat16_clusters = 4096
	
	sector_size = 512
	cluster_size = 4096
	dirent_size = 32
	fatent_size = 2
	fat_count = 2
	reserved_clusters = 2
	
	# Make sure the filesystem is large enough for FAT16
	size = subtree_size(path, cluster_size, dirent_size) + reserved_clusters * cluster_size + extra_bytes
	while (size // cluster_size < fat16_clusters):
		if (cluster_size > sector_size):
			cluster_size = cluster_size // 2
			size = subtree_size(path, cluster_size, dirent_size) + reserved_clusters * cluster_size + extra_bytes
		else:
			size = fat16_clusters * cluster_size + reserved_clusters * cluster_size
	
	root_size = align_up(root_entries(path) * dirent_size, cluster_size)
	
	fat_size = align_up(align_up(size, cluster_size) // cluster_size * fatent_size, sector_size)
	
	sectors = (cluster_size + fat_count * fat_size + root_size + size) // sector_size
	root_start = cluster_size + fat_count * fat_size
	data_start = root_start + root_size
	
	outf = open(sys.argv[3], "wb")
	
	boot_sector = xstruct.create(BOOT_SECTOR)
	boot_sector.jmp = [0xEB, 0x3C, 0x90]
	boot_sector.oem = b'MSDOS5.0'
	boot_sector.sector = sector_size
	boot_sector.cluster = cluster_size // sector_size
	boot_sector.reserved = cluster_size // sector_size
	boot_sector.fats = fat_count
	boot_sector.rootdir = root_size // dirent_size
	if (sectors <= 65535):
		boot_sector.sectors = sectors
	else:
		boot_sector.sectors = 0
	boot_sector.descriptor = 0xF8
	boot_sector.fat_sectors = fat_size // sector_size
	boot_sector.track_sectors = 63
	boot_sector.heads = 6
	boot_sector.hidden = 0
	if (sectors > 65535):
		boot_sector.sectors_big = sectors
	else:
		boot_sector.sectors_big = 0
	
	boot_sector.drive = 0x80
	boot_sector.extboot_signature = 0x29
	boot_sector.serial = random.randint(0, 0x7fffffff)
	boot_sector.label = b'HELENOS'
	boot_sector.fstype = b'FAT16   '
	boot_sector.boot_signature = [0x55, 0xAA]
	
	outf.write(boot_sector.pack())
	
	empty_sector = xstruct.create(EMPTY_SECTOR)
	
	# Reserved sectors
	for i in range(1, cluster_size // sector_size):
		outf.write(empty_sector.pack())
	
	# FAT tables
	for i in range(0, fat_count):
		for j in range(0, fat_size // sector_size):
			outf.write(empty_sector.pack())
	
	# Root directory
	for i in range(0, root_size // sector_size):
		outf.write(empty_sector.pack())
	
	# Data
	for i in range(0, size // sector_size):
		outf.write(empty_sector.pack())
	
	fat = array.array('L', [0] * (fat_size // fatent_size))
	fat[0] = 0xfff8
	fat[1] = 0xffff
	
	recursion(True, path, outf, cluster_size, root_start, data_start, fat, reserved_clusters, dirent_size, 0)
	
	# Store FAT
	fat_entry = xstruct.create(FAT_ENTRY)
	for i in range(0, fat_count):
		outf.seek(cluster_size + i * fat_size)
		for j in range(0, fat_size // fatent_size):
			fat_entry.next = fat[j]
			outf.write(fat_entry.pack())
	
	outf.close()
	
if __name__ == '__main__':
	main()
