#!/usr/bin/env python
#
# Copyright (c) 2011 Martin Sucha
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
EXT2 creator
"""

import sys
import os
import xstruct
import array
import time
import uuid
from imgutil import *

if sys.version >= '3':
	xrange = range

GDE_SIZE = 32

STRUCT_DIR_ENTRY_HEAD = """little:
	uint32_t inode       /* inode number */
	uint16_t skip        /* byte offset to the next record */
	uint8_t  name_length /* file attributes */
	uint8_t  inode_type  /* type of the referenced inode */
"""

STRUCT_SUPERBLOCK = """little:
	uint32_t total_inode_count    /* Total number of inodes */
	uint32_t total_block_count    /* Total number of blocks */
	uint32_t reserved_block_count /* Total number of reserved blocks */
	uint32_t free_block_count     /* Total number of free blocks */
	uint32_t free_inode_count     /* Total number of free inodes */
	uint32_t first_block          /* Block containing the superblock */
	uint32_t block_size_log2      /* log_2(block_size) */
	int32_t  fragment_size_log2   /* log_2(fragment size) */
	uint32_t blocks_per_group     /* Number of blocks in one block group */
	uint32_t fragments_per_group  /* Number of fragments per block group */
	uint32_t inodes_per_group     /* Number of inodes per block group */
	uint32_t mount_time           /* Time when the filesystem was last mounted */
	uint32_t write_time           /* Time when the filesystem was last written */
	uint16_t mount_count          /* Mount count since last full filesystem check */
	uint16_t max_mount_count      /* Number of mounts after which the fs must be checked */
	uint16_t magic                /* Magic value */
	uint16_t state                /* State (mounted/unmounted) */
	uint16_t error_behavior       /* What to do when errors are encountered */
	uint16_t rev_minor            /* Minor revision level */
	uint32_t last_check_time      /* Unix time of last fs check */
	uint32_t max_check_interval   /* Max unix time interval between checks */
	uint32_t os                   /* OS that created the filesystem */
	uint32_t rev_major            /* Major revision level */
	padding[4] /* default reserved uid and gid */

	/* Following is for ext2 revision 1 only */
	uint32_t first_inode
	uint16_t inode_size
	uint16_t block_group_number /* Block group where this SB is stored */
	uint32_t features_compatible
	uint32_t features_incompatible
	uint32_t features_read_only
	char     uuid[16]
	char     volume_name[16]
"""

STRUCT_BLOCK_GROUP_DESCRIPTOR = """little:
	uint32_t block_bitmap_block      /* Block ID for block bitmap */
	uint32_t inode_bitmap_block      /* Block ID for inode bitmap */
	uint32_t inode_table_first_block /* Block ID of first block of inode table */
	uint16_t free_block_count        /* Count of free blocks */
	uint16_t free_inode_count        /* Count of free inodes */
	uint16_t directory_inode_count   /* Number of inodes allocated to directories */
"""

STRUCT_INODE = """little:
	uint16_t mode
	uint16_t user_id
	uint32_t size
	uint32_t access_time
	uint32_t creation_time
	uint32_t modification_time
	uint32_t deletion_time
	uint16_t group_id
	uint16_t usage_count /* Hard link count, when 0 the inode is to be freed */
	uint32_t reserved_512_blocks /* Size of this inode in 512-byte blocks */
	uint32_t flags
	padding[4]
	uint32_t direct_blocks[12] /* Direct block ids stored in this inode */
	uint32_t indirect_blocks[3]
	uint32_t version
	uint32_t file_acl
	uint32_t size_high /* For regular files in version >= 1, dir_acl if dir */
	padding[6]
	uint16_t mode_high /* Hurd only */
	uint16_t user_id_high /* Linux/Hurd only */
	uint16_t group_id_high /* Linux/Hurd only */
"""

# The following is here to handle byte-order conversion in indirect block lookups
STRUCT_BLOCK_REFERENCE = """little:
	uint32_t block_id /* Referenced block ID */
"""

class Filesystem:
	def __init__(self, filename, block_groups, blocks_per_group, inodes_per_group, block_size, inode_size, reserved_inode_count):
		"Initialize the filesystem writer"

		outf = open(filename, "w+b")
		# Set the correct size of the image, so that we can read arbitrary position
		outf.truncate(block_size * blocks_per_group * block_groups)
		self.outf = outf
		self.uuid = uuid.uuid4()
		self.block_size = block_size
		self.inode_size = inode_size
		self.block_groups = block_groups
		self.blocks_per_group = blocks_per_group
		self.inodes_per_group = inodes_per_group
		self.reserved_inode_count = reserved_inode_count
		self.gdt_blocks = count_up(block_groups * GDE_SIZE, block_size)
		self.inode_table_blocks_per_group = (inodes_per_group * inode_size) // block_size
		self.inode_bitmap_blocks_per_group = count_up(inodes_per_group // 8, block_size)
		self.block_bitmap_blocks_per_group = count_up(blocks_per_group // 8, block_size)
		self.total_block_count = self.blocks_per_group * self.block_groups
		self.total_inode_count = self.inodes_per_group * self.block_groups
		self.inode_table_blocks = count_up(self.total_inode_count * inode_size, block_size)
		self.inodes = {}
		self.superblock_at_block = 2047 // block_size
		self.block_ids_per_block = self.block_size // 4
		self.indirect_limits = [12, None, None, None]
		self.indirect_blocks_per_level = [1, None, None, None]
		for i in range(1,4):
			self.indirect_blocks_per_level[i] = self.indirect_blocks_per_level[i-1] * self.block_ids_per_block
			self.indirect_limits[i] = self.indirect_limits[i-1] + self.indirect_blocks_per_level[i]
		self.block_allocator = Allocator(0, self.total_block_count)
		self.inode_allocator = Allocator(1, self.total_inode_count)
		self.init_gdt()
		# Set the callbacks after GDT has been initialized
		self.block_allocator.mark_cb = self.mark_block_cb
		self.inode_allocator.mark_cb = self.mark_inode_cb
		self.block_allocator.mark_used_all(range(self.superblock_at_block))
		self.inode_allocator.mark_used_all(range(1, self.reserved_inode_count + 1))
		self.root_inode = Inode(self, 2, Inode.TYPE_DIR)
		self.gdt[0].directory_inode_count += 1
		self.lost_plus_found = Inode(self, self.inode_allocator.alloc(directory=True), Inode.TYPE_DIR)
		lpf_dir = DirWriter(self.lost_plus_found)
		lpf_dir.add(self.lost_plus_found.as_dirent('.'))
		lpf_dir.add(self.root_inode.as_dirent('..'))
		lpf_dir.finish()

	def init_gdt(self):
		"Initialize block group descriptor table"

		self.superblock_positions = []
		self.gdt = []
		consumed_blocks_per_group = (1 + self.gdt_blocks +
			    self.inode_bitmap_blocks_per_group +
			    self.block_bitmap_blocks_per_group +
			    self.inode_table_blocks_per_group)
		initial_free_blocks = self.blocks_per_group - consumed_blocks_per_group
		for bg in range(self.block_groups):
			base = bg * self.blocks_per_group
			if (bg == 0):
				base = self.superblock_at_block
				self.superblock_positions.append(1024)
			else:
				self.superblock_positions.append(base * self.block_size)
			self.block_allocator.mark_used_all(range(base, base+consumed_blocks_per_group))
			pos = base + 1 + self.gdt_blocks
			gde = xstruct.create(STRUCT_BLOCK_GROUP_DESCRIPTOR)
			gde.block_bitmap_block = pos
			pos += self.block_bitmap_blocks_per_group
			gde.inode_bitmap_block = pos
			pos += self.inode_bitmap_blocks_per_group
			gde.inode_table_first_block = pos
			gde.free_block_count = initial_free_blocks
			gde.free_inode_count = self.inodes_per_group
			gde.directory_inode_count = 0
			self.gdt.append(gde)

	def mark_block_cb(self, block):
		"Called after a block has been allocated"

		self.gdt[block // self.blocks_per_group].free_block_count -= 1

	def mark_inode_cb(self, index, directory=False):
		"Called after an inode has been allocated"

		index -= 1
		gde = self.gdt[index // self.inodes_per_group]
		gde.free_inode_count -= 1
		if directory:
			gde.directory_inode_count += 1

	def seek_to_block(self, block, offset=0):
		"Seek to offset bytes after the start of the given block"

		if offset < 0 or offset > self.block_size:
			raise Exception("Invalid in-block offset")
		self.outf.seek(block * self.block_size + offset)

	def seek_to_inode(self, index):
		"Seek to the start of the inode structure for the inode number index"

		index -= 1
		if index < 0:
			raise Exception("Invalid inode number")
		gde = self.gdt[index // self.inodes_per_group]
		base_block = gde.inode_table_first_block
		offset = (index % self.inodes_per_group) * self.inode_size
		block = base_block + (offset // self.block_size)
		self.seek_to_block(block, offset % self.block_size)

	def subtree_add(self, inode, parent_inode, dirpath, is_root=False):
		"Recursively add files to the filesystem"

		dir_writer = DirWriter(inode)
		dir_writer.add(inode.as_dirent('.'))
		dir_writer.add(parent_inode.as_dirent('..'))

		if is_root:
			dir_writer.add(self.lost_plus_found.as_dirent('lost+found'))

		for item in listdir_items(dirpath):
			newidx = self.inode_allocator.alloc(directory = item.is_dir)
			if item.is_file:
				child_inode = Inode(self, newidx, Inode.TYPE_FILE)
				for data in chunks(item, self.block_size):
					child_inode.write(data)
			elif item.is_dir:
				child_inode = Inode(self, newidx, Inode.TYPE_DIR)
				self.subtree_add(child_inode, inode, item.path)

			dir_writer.add(child_inode.as_dirent(item.name))
			self.write_inode(child_inode)

		dir_writer.finish()

	def write_inode(self, inode):
		"Write inode information into the inode table"

		self.seek_to_inode(inode.index)
		self.outf.write(inode.pack())

	def write_gdt(self):
		"Write group descriptor table at the current file position"

		for gde in self.gdt:
			data = bytes(gde.pack())
			self.outf.write(data)
			self.outf.seek(GDE_SIZE-len(data), os.SEEK_CUR)

	def write_superblock(self, block_group):
		"Write superblock at the current position"

		sb = xstruct.create(STRUCT_SUPERBLOCK)
		sb.total_inode_count = self.total_inode_count
		sb.total_block_count = self.total_block_count
		sb.reserved_block_count = 0
		sb.free_block_count = self.block_allocator.free
		sb.free_inode_count = self.inode_allocator.free
		sb.first_block = self.superblock_at_block
		sb.block_size_log2 = num_of_trailing_bin_zeros(self.block_size) - 10
		sb.fragment_size_log2 = sb.block_size_log2
		sb.blocks_per_group = self.blocks_per_group
		sb.fragments_per_group = self.blocks_per_group
		sb.inodes_per_group = self.inodes_per_group
		curtime = int(time.time())
		sb.mount_time = curtime
		sb.write_time = curtime
		sb.mount_count = 0
		sb.max_mount_count = 21
		sb.magic = 0xEF53
		sb.state = 5 # clean
		sb.error_behavior = 1 # continue on errors
		sb.rev_minor = 0
		sb.last_check_time = curtime
		sb.max_check_interval = 15552000 # 6 months
		sb.os = 0 # Linux
		sb.rev_major = 1
		sb.first_inode = self.reserved_inode_count + 1
		sb.inode_size = self.inode_size
		sb.block_group_number = block_group
		sb.features_compatible = 0
		sb.features_incompatible = 2 # filetype
		sb.features_read_only = 0
		sb.uuid = self.uuid.bytes_le
		sb.volume_name = 'HelenOS rdimage\0'
		self.outf.write(bytes(sb.pack()))

	def write_all_metadata(self):
		"Write superblocks, block group tables, block and inode bitmaps"

		bbpg = self.blocks_per_group // 8
		bipg = self.inodes_per_group // 8
		def window(arr, index, size):
			return arr[index * size:(index + 1) * size]

		for bg_index in xrange(len(self.gdt)):
			sbpos = self.superblock_positions[bg_index]
			sbblock = (sbpos + 1023) // self.block_size
			gde = self.gdt[bg_index]

			self.outf.seek(sbpos)
			self.write_superblock(bg_index)

			self.seek_to_block(sbblock+1)
			self.write_gdt()

			self.seek_to_block(gde.block_bitmap_block)
			self.outf.write(window(self.block_allocator.bitmap, bg_index, bbpg))

			self.seek_to_block(gde.inode_bitmap_block)
			self.outf.write(window(self.inode_allocator.bitmap, bg_index, bipg))

	def close(self):
		"Write all remaining data to the filesystem and close the file"

		self.write_inode(self.root_inode)
		self.write_inode(self.lost_plus_found)
		self.write_all_metadata()
		self.outf.close()

class Allocator:
	def __init__(self, base, count):
		self.count = count
		self.base = base
		self.free = count
		self.nextidx = 0
		self.bitmap = array.array('B', [0] * (count // 8))
		self.mark_cb = None

	def __contains__(self, item):
		"Check if the item is already used"

		bitidx = item - self.base
		return get_bit(self.bitmap[bitidx // 8], bitidx % 8)

	def alloc(self, **options):
		"Allocate a new item"

		while self.nextidx < self.count and (self.base + self.nextidx) in self:
			self.nextidx += 1
		if self.nextidx == self.count:
			raise Exception("No free item available")
		item = self.base + self.nextidx
		self.nextidx += 1
		self.mark_used(item, **options)
		return item

	def mark_used(self, item, **options):
		"Mark the specified item as used"

		bitidx = item - self.base
		if item in self:
			raise Exception("Item already used: " + str(item))
		self.free -= 1
		index = bitidx // 8
		self.bitmap[index] = set_bit(self.bitmap[index], bitidx % 8)
		if self.mark_cb:
			self.mark_cb(item, **options)

	def mark_used_all(self, items, **options):
		"Mark all specified items as used"

		for item in items:
			self.mark_used(item, **options)

class Inode:
	TYPE_FILE = 1
	TYPE_DIR = 2
	TYPE2MODE = {TYPE_FILE: 8, TYPE_DIR: 4}

	def __init__(self, fs, index, typ):
		self.fs = fs
		self.direct = [None] * 12
		self.indirect = [None] * 3
		self.pos = 0
		self.size = 0
		self.blocks = 0
		self.index = index
		self.type = typ
		self.refcount = 0

	def as_dirent(self, name):
		"Return a DirEntry corresponding to this inode"
		self.refcount += 1
		return DirEntry(name, self.index, self.type)

	def new_block(self, data=True):
		"Get a new block index from allocator and count it here as belonging to the file"

		block = self.fs.block_allocator.alloc()
		self.blocks += 1
		return block

	def get_or_add_block(self, block):
		"Get or add a real block to the file"

		if block < 12:
			return self.get_or_add_block_direct(block)
		return self.get_or_add_block_indirect(block)

	def get_or_add_block_direct(self, block):
		"Get or add a real block to the file (direct blocks)"

		if self.direct[block] == None:
			self.direct[block] = self.new_block()
		return self.direct[block]

	def get_or_add_block_indirect(self, block):
		"Get or add a real block to the file (indirect blocks)"

		# Determine the indirection level for the desired block
		level = None
		for i in range(4):
			if block < self.fs.indirect_limits[i]:
				level = i
				break

		assert level != None

		# Compute offsets for the topmost level
		block_offset_in_level = block - self.fs.indirect_limits[level-1];
		if self.indirect[level-1] == None:
			self.indirect[level-1] = self.new_block(data = False)
		current_block = xstruct.create(STRUCT_BLOCK_REFERENCE)
		current_block.block_id = self.indirect[level-1]
		offset_in_block = block_offset_in_level // self.fs.indirect_blocks_per_level[level-1]

		# Navigate through other levels
		while level > 0:
			assert offset_in_block < self.fs.block_ids_per_block

			level -= 1

			self.fs.seek_to_block(current_block.block_id, offset_in_block*4)
			current_block.unpack(self.fs.outf.read(4))

			if current_block.block_id == 0:
				# The block does not exist, so alloc one and write it there
				self.fs.outf.seek(-4, os.SEEK_CUR)
				current_block.block_id = self.new_block(data=(level==0))
				self.fs.outf.write(current_block.pack())

			# If we are on the last level, break here as
			# there is no next level to visit
			if level == 0:
				break

			# Visit the next level
			block_offset_in_level %= self.fs.indirect_blocks_per_level[level];
			offset_in_block = block_offset_in_level // self.fs.indirect_blocks_per_level[level-1]

		return current_block.block_id

	def do_seek(self):
		"Perform a seek to the position indicated by self.pos"

		block = self.pos // self.fs.block_size
		real_block = self.get_or_add_block(block)
		offset = self.pos % self.fs.block_size
		self.fs.seek_to_block(real_block, offset)

	def write(self, data):
		"Write a piece of data (arbitrarily long) as the contents of the inode"

		data_pos = 0
		while data_pos < len(data):
			bytes_remaining_in_block = self.fs.block_size - (self.pos % self.fs.block_size)
			bytes_to_write = min(bytes_remaining_in_block, len(data)-data_pos)
			self.do_seek()
			self.fs.outf.write(data[data_pos:data_pos + bytes_to_write])
			self.pos += bytes_to_write
			data_pos += bytes_to_write
			self.size = max(self.pos, self.size)

	def align_size_to_block(self):
		"Align the size of the inode up to block size"

		self.size = align_up(self.size, self.fs.block_size)

	def align_pos(self, bytes):
		"Align the current position up to bytes boundary"

		self.pos = align_up(self.pos, bytes)

	def set_pos(self, pos):
		"Set the current position"

		self.pos = pos

	def pack(self):
		"Pack the inode structure and return the result"

		data = xstruct.create(STRUCT_INODE)
		data.mode = (Inode.TYPE2MODE[self.type] << 12)
		data.mode |= 0x1ff # ugo+rwx
		data.user_id = 0
		data.size = self.size & 0xFFFFFFFF
		data.group_id = 0
		curtime = int(time.time())
		data.access_time = curtime
		data.modification_time = curtime
		data.creation_time = curtime
		data.deletion_time = 0
		data.usage_count = self.refcount
		data.reserved_512_blocks = self.blocks * (self.fs.block_size // 512)
		data.flags = 0
		blockconv = lambda x: 0 if x == None else x
		data.direct_blocks = list(map(blockconv, self.direct))
		data.indirect_blocks = list(map(blockconv, self.indirect))
		data.version = 0
		data.file_acl = 0
		if self.type == Inode.TYPE_FILE:
			data.size_high = (self.size >> 32)
		else:
			# size_high represents dir_acl in this case
			data.size_high = 0
		data.mode_high = 0
		data.user_id_high = 0
		data.group_id_high = 0
		return data.pack()

class DirEntry:
	"Represents a linked list directory entry"

	def __init__(self, name, inode, typ):
		self.name = name.encode('UTF-8')
		self.inode = inode
		self.skip = None
		self.type = typ

	def size(self):
		"Return size of the entry in bytes"

		return align_up(8 + len(self.name)+1, 4)

	def write(self, inode):
		"Write the directory entry into the inode"

		head = xstruct.create(STRUCT_DIR_ENTRY_HEAD)
		head.inode = self.inode
		head.skip = self.skip
		head.name_length = len(self.name)
		head.inode_type = self.type
		inode.write(head.pack())
		inode.write(self.name+'\0'.encode())
		inode.align_pos(4)

class DirWriter:
	"Manages writing directory entries into an inode (alignment, etc.)"

	def __init__(self, inode):
		self.pos = 0
		self.inode = inode
		self.prev_entry = None
		self.prev_pos = None

	def prev_write(self):
		"Write a previously remembered entry"

		if self.prev_entry:
			self.prev_entry.skip = self.pos - self.prev_pos
			if self.inode:
				self.prev_entry.write(self.inode)
				self.inode.set_pos(self.pos)

	def add(self, entry):
		"Add a directory entry to the directory"

		size = entry.size()
		block_size = self.inode.fs.block_size
		if align_up(self.pos, block_size) < align_up(self.pos + size, block_size):
			self.pos = align_up(self.pos, block_size)
		self.prev_write()
		self.prev_entry = entry
		self.prev_pos = self.pos
		self.pos += size

	def finish(self):
		"Write the last entry and finish writing the directory contents"

		if not self.inode:
			return
		self.pos = align_up(self.pos, self.inode.fs.block_size)
		self.prev_write()
		self.inode.align_size_to_block()

def subtree_stats(root, block_size):
	"Recursively calculate statistics"

	blocks = 0
	inodes = 1
	dir_writer = DirWriter(None)

	for item in listdir_items(root):
		inodes += 1
		if item.is_file:
			blocks += count_up(item.size, block_size)
		elif item.is_dir:
			subtree_blocks, subtree_inodes = subtree_stats(item.path, block_size)
			blocks += subtree_blocks
			inodes += subtree_inodes

	dir_writer.finish()
	blocks += count_up(dir_writer.pos, block_size)
	return (blocks, inodes)

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

	block_size = 4096
	inode_size = 128
	reserved_inode_count = 10
	blocks_per_group = 1024
	inodes_per_group = 512

	blocks, inodes = subtree_stats(path, block_size)
	blocks += count_up(extra_bytes, block_size)
	inodes += reserved_inode_count

	inodes_per_group = align_up(inodes_per_group, 8)
	blocks_per_group = align_up(blocks_per_group, 8)

	inode_table_blocks_per_group = (inodes_per_group * inode_size) // block_size
	inode_bitmap_blocks_per_group = count_up(inodes_per_group // 8, block_size)
	block_bitmap_blocks_per_group = count_up(blocks_per_group // 8, block_size)
	free_blocks_per_group = blocks_per_group
	free_blocks_per_group -= inode_table_blocks_per_group
	free_blocks_per_group -= inode_bitmap_blocks_per_group
	free_blocks_per_group -= block_bitmap_blocks_per_group
	free_blocks_per_group -= 10 # one for SB and some reserve for GDT

	block_groups = max(count_up(inodes, inodes_per_group), count_up(blocks, free_blocks_per_group))

	fs = Filesystem(sys.argv[3], block_groups, blocks_per_group, inodes_per_group,
	                        block_size, inode_size, reserved_inode_count)

	fs.subtree_add(fs.root_inode, fs.root_inode, path, is_root=True)
	fs.close()

if __name__ == '__main__':
	main()
