#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2008 Martin Decky
# SPDX-FileCopyrightText: 2011 Martin Sucha
#
# SPDX-License-Identifier: BSD-3-Clause
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
