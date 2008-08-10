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
HORD encapsulator
"""

import sys
import os
import xstruct

HEADER = """little:
	char tag[4]            /* 'HORD' */
	uint8_t version        /* version */
	uint8_t encoding       /* encoding */
	uint32_t header_size   /* header size */
	uint64_t payload_size  /* payload size */
"""

HORD_LSB = 1

def align_up(size, alignment):
	"Align upwards to a given alignment"
	return (((size) + ((alignment) - 1)) & ~((alignment) - 1))

def usage(prname):
	"Print usage syntax"
	print prname + " <ALIGNMENT> <FS_IMAGE> <HORD_IMAGE>" 

def main():
	if (len(sys.argv) < 4):
		usage(sys.argv[0])
		return
	
	if (not sys.argv[1].isdigit()):
		print "<ALIGNMENT> must be a number"
		return
	
	align = int(sys.argv[1], 0)
	if (align <= 0):
		print "<ALIGNMENT> must be positive"
		return
	
	fs_image = os.path.abspath(sys.argv[2])
	if (not os.path.isfile(fs_image)):
		print "<FS_IMAGE> must be a file"
		return
	
	inf = file(fs_image, "rb")
	outf = file(sys.argv[3], "wb")
	
	header = xstruct.create(HEADER)
	
	header_size = header.size()
	payload_size = os.path.getsize(fs_image)
	
	header_size_aligned = align_up(header_size, align)
	payload_size_aligned = align_up(payload_size, align)
	
	header.tag = "HORD"
	header.version = 1
	header.encoding = HORD_LSB
	header.header_size = header_size_aligned
	header.payload_size = payload_size_aligned
	
	outf.write(header.pack())
	outf.write(xstruct.create("little: padding[%d]" % (header_size_aligned - header_size)).pack())
	
	outf.write(inf.read())
	
	padding = payload_size_aligned - payload_size
	if (padding > 0):
		outf.write(xstruct.create("little: padding[%d]" % padding).pack())
	
	inf.close()
	outf.close()

if __name__ == '__main__':
	main()
