#!/usr/bin/env python
#
# Copyright (c) 2010 Jiri Svoboda
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
Create legacy uImage (U-Boot image)
"""

from collections import deque
import os
import sys
import xstruct
import zlib

UIMAGE_HEADER = """big:
	uint32_t magic
	uint32_t header_crc
	uint32_t c_tstamp
	uint32_t data_size
	uint32_t load_addr
	uint32_t start_addr
	uint32_t data_crc
	uint8_t os
	uint8_t arch
	uint8_t img_type
	uint8_t compression
	char img_name[32]
"""

def main():
	args = deque(sys.argv)
	cmd_name = args.popleft()
	base_name = os.path.basename(cmd_name)
	image_name = 'Noname'
	load_addr = 0
	start_addr = 0
	os_type = 5 #Linux is the default

	while len(args) >= 2 and args[0][0] == '-':
		opt = args.popleft()[1:]
		optarg = args.popleft()

		if opt == 'name':
			image_name = optarg
		elif opt == 'laddr':
			load_addr = (int)(optarg, 0)
		elif opt == 'saddr':
			start_addr = (int)(optarg, 0)
		elif opt == 'ostype':
			os_type = (int)(optarg, 0)
		else:
			print(base_name + ": Unrecognized option.")
			print_syntax(cmd_name)
			return

	if len(args) < 2:
		print(base_name + ": Argument missing.")
		print_syntax(cmd_name)
		return

	inf_name = args[0]
	outf_name = args[1]

	try:
		mkuimage(inf_name, outf_name, image_name, load_addr, start_addr, os_type)
	except:
		os.remove(outf_name)
		raise

def mkuimage(inf_name, outf_name, image_name, load_addr, start_addr, os_type):
	inf = open(inf_name, 'rb')
	outf = open(outf_name, 'wb')

	header = xstruct.create(UIMAGE_HEADER)
	header_size = header.size()

	#
	# Write data
	#
	outf.seek(header_size, os.SEEK_SET)
	data = inf.read()
	data_size = inf.tell()
	data_crc = calc_crc32(data)
	data_tstamp = (int)(os.path.getmtime(inf_name))
	outf.write(data)
	data = ''

	#
	# Write header
	#
	outf.seek(0, os.SEEK_SET)

	header.magic = 0x27051956	# uImage magic
	header.header_crc = 0
	header.c_tstamp = data_tstamp
	header.data_size = data_size
	header.load_addr = load_addr	# Address where to load image
	header.start_addr = start_addr	# Address of entry point
	header.data_crc = data_crc
	header.os = os_type
	header.arch = 2			# ARM
	header.img_type = 2		# Kernel
	header.compression = 0		# None
	header.img_name = image_name.encode('ascii')

	header_crc = calc_crc32(header.pack())
	header.header_crc = header_crc

	outf.write(header.pack())
	outf.close()

## Compute CRC32 of binary string.
#
# Works around bug in zlib.crc32() which returns signed int32 result
# in Python < 3.0.
#
def calc_crc32(byteseq):
	signed_crc = zlib.crc32(byteseq, 0)
	if signed_crc < 0:
		return signed_crc + (1 << 32)
	else:
		return signed_crc

## Print command-line syntax.
#
def print_syntax(cmd):
	print("syntax: " + cmd + " [<options>] <raw_image> <uImage>")
	print()
	print("\traw_image\tInput image name (raw binary data)")
	print("\tuImage\t\tOutput uImage name (U-Boot image)")
	print()
	print("options:")
	print("\t-name <name>\tImage name (default: 'Noname')")
	print("\t-laddr <name>\tLoad address (default: 0x00000000)")
	print("\t-saddr <name>\tStart address (default: 0x00000000)")

if __name__ == '__main__':
	main()
