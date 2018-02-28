#!/usr/bin/env python
#
# Copyright (c) 2012 Martin Decky
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
HelenOS out-of-source-tree build utility
"""

import sys
import os
import stat
import errno
import subprocess

exclude_names = set(['.svn', '.bzr', '.git'])

def duplicate_tree(src_path, dest_path, current):
	"Duplicate source directory tree in the destination path"

	for name in os.listdir(os.path.join(src_path, current)):
		if name in exclude_names:
			next

		following = os.path.join(current, name)
		src = os.path.join(src_path, following)
		dest = os.path.join(dest_path, following)
		dest_parent = os.path.join(dest_path, current)
		dest_stat = os.stat(src)

		# Create shadow directories
		if stat.S_ISDIR(dest_stat.st_mode):
			if not os.path.exists(dest):
				os.mkdir(dest)

			if not os.path.isdir(dest):
				raise IOError(errno.ENOTDIR, "Destination path exists, but is not a directory", dest)

			duplicate_tree(src_path, dest_path, following)
		else:
			# Compute the relative path from destination to source
			relative = os.path.relpath(src, dest_parent)

			# Create symlink
			if not os.path.exists(dest):
				os.symlink(relative, dest)

def usage(prname):
	"Print usage syntax"
	print(prname + " <SRC_PATH> <DEST_PATH> [MAKE ARGS...]")

def main():
	if len(sys.argv) < 3:
		usage(sys.argv[0])
		return 1

	# Source tree path
	src_path = os.path.abspath(sys.argv[1])
	if not os.path.isdir(src_path):
		print("<SRC_PATH> must be a directory")
		return 2

	# Destination tree path
	dest_path = os.path.abspath(sys.argv[2])
	if not os.path.exists(dest_path):
		os.mkdir(dest_path)

	if not os.path.isdir(dest_path):
		print("<DEST_PATH> must be a directory")
		return 3

	# Duplicate source directory tree
	duplicate_tree(src_path, dest_path, "")

	# Run the build from the destination tree
	os.chdir(dest_path)
	args = ["make"]
	args.extend(sys.argv[3:])

	return subprocess.Popen(args).wait()

if __name__ == '__main__':
	sys.exit(main())
