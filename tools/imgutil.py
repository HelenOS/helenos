#!/usr/bin/env python
#
# Copyright (c) 2008 Martin Decky
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
Utilities for filesystem image creators
"""

import os
import stat

exclude_names = set(['.svn', '.bzr', '.git'])

def align_up(size, alignment):
	"Return size aligned up to alignment"

	if (size % alignment == 0):
		return size

	return ((size // alignment) + 1) * alignment

def count_up(size, alignment):
	"Return units necessary to fit the size"

	if (size % alignment == 0):
		return (size // alignment)

	return ((size // alignment) + 1)

def num_of_trailing_bin_zeros(num):
	"Return number of trailing zeros in binary representation"

	i = 0
	if (num == 0): raise ValueError()
	while num & 1 == 0:
		i += 1
		num = num >> 1
	return i

def get_bit(number, n):
	"Return True if n-th least-significant bit is set in the given number"

	return bool((number >> n) & 1)

def set_bit(number, n):
	"Return the number with n-th least-significant bit set"

	return number | (1 << n)

class ItemToPack:
	"Stores information about one directory item to be added to the image"

	def __init__(self, parent, name):
		self.parent = parent
		self.name = name
		self.path = os.path.join(parent, name)
		self.stat = os.stat(self.path)
		self.is_dir = stat.S_ISDIR(self.stat.st_mode)
		self.is_file = stat.S_ISREG(self.stat.st_mode)
		self.size = self.stat.st_size

def listdir_items(path):
	"Return a list of items to be packed inside a fs image"

	for name in os.listdir(path):
		if name in exclude_names:
			continue

		item = ItemToPack(path, name)

		if not (item.is_dir or item.is_file):
			continue

		yield item

def chunks(item, chunk_size):
	"Iterate contents of a file in chunks of a given size"

	inf = open(item.path, 'rb')
	rd = 0

	while (rd < item.size):
		data = bytes(inf.read(chunk_size))
		yield data
		rd += len(data)

	inf.close()
