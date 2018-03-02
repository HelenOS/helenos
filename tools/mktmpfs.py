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
TMPFS creator
"""

import sys
import os
import xstruct
from imgutil import listdir_items, chunks

HEADER = """little:
	char tag[5]  /* 'TMPFS' */
"""

DENTRY_NONE = """little:
	uint8_t kind        /* NONE */
	uint32_t fname_len  /* 0 */
"""

DENTRY_FILE = """little:
	uint8_t kind        /* FILE */
	uint32_t fname_len  /* filename length */
	char fname[%d]      /* filename */
	uint32_t flen       /* file length */
"""

DENTRY_DIRECTORY = """little:
	uint8_t kind        /* DIRECTORY */
	uint32_t fname_len  /* filename length */
	char fname[%d]      /* filename */
"""

TMPFS_NONE = 0
TMPFS_FILE = 1
TMPFS_DIRECTORY = 2

def usage(prname):
	"Print usage syntax"
	print(prname + " <PATH> <IMAGE>")

def recursion(root, outf):
	"Recursive directory walk"

	for item in listdir_items(root):
		if item.is_file:
			dentry = xstruct.create(DENTRY_FILE % len(item.name))
			dentry.kind = TMPFS_FILE
			dentry.fname_len = len(item.name)
			dentry.fname = item.name.encode('ascii')
			dentry.flen = item.size

			outf.write(dentry.pack())

			for data in chunks(item, 4096):
				outf.write(data)

		elif item.is_dir:
			dentry = xstruct.create(DENTRY_DIRECTORY % len(item.name))
			dentry.kind = TMPFS_DIRECTORY
			dentry.fname_len = len(item.name)
			dentry.fname = item.name.encode('ascii')

			outf.write(dentry.pack())

			recursion(item.path, outf)

			dentry = xstruct.create(DENTRY_NONE)
			dentry.kind = TMPFS_NONE
			dentry.fname_len = 0

			outf.write(dentry.pack())

def main():
	if (len(sys.argv) < 3):
		usage(sys.argv[0])
		return

	path = os.path.abspath(sys.argv[1])
	if (not os.path.isdir(path)):
		print("<PATH> must be a directory")
		return

	outf = open(sys.argv[2], "wb")

	header = xstruct.create(HEADER)
	header.tag = b"TMPFS"

	outf.write(header.pack())

	recursion(path, outf)

	dentry = xstruct.create(DENTRY_NONE)
	dentry.kind = TMPFS_NONE
	dentry.fname_len = 0

	outf.write(dentry.pack())

	outf.close()

if __name__ == '__main__':
	main()
