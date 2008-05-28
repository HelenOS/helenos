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
import struct

def usage(prname):
	"Print usage syntax"
	print prname + " <PATH> <IMAGE>"

def main():
	if (len(sys.argv) < 3):
		usage(sys.argv[0])
		return
	
	path = os.path.abspath(sys.argv[1]);
	if (not os.path.isdir(path)):
		print "<PATH> must be a directory"
		return
	
	outf = file(sys.argv[2], "w");
	
	for root, dirs, files in os.walk(path):
		relpath = root[len(path):len(root)]
		for name in files:
			canon = os.path.join(relpath, name)
			outf.write(struct.pack("<BL" + ("%d" % len(canon)) + "s", 1, len(canon), canon))
			
			fn = os.path.join(root, name)
			size = os.path.getsize(fn)
			rd = 0;
			outf.write(struct.pack("<L", size))
			
			inf = file(fn, "r")
			while (rd < size):
				data = inf.read(4096);
				outf.write(data)
				rd += len(data)
			inf.close()
		
		for name in dirs:
			canon = os.path.join(relpath, name)
			outf.write(struct.pack("<BL" + ("%d" % len(canon)) + "s", 2, len(canon), canon))
	
	outf.close()

if __name__ == '__main__':
	main()
