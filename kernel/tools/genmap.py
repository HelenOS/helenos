#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
Create binary symbol map out of linker map file
"""

import sys
import struct
import re

MAXSTRING = 63
symtabfmt = "<Q%ds" % (MAXSTRING + 1)

funcline = re.compile(r'([0-9a-f]+)\s+[lg]\s+.\s+\.text\s+([0-9a-f]+)\s+(.*)$')
bssline = re.compile(r'([0-9a-f]+)\s+[lg]\s+[a-zA-Z]\s+\.bss\s+([0-9a-f]+)\s+(.*)$')
dataline = re.compile(r'([0-9a-f]+)\s+[lg]\s+[a-zA-Z]\s+\.data\s+([0-9a-f]+)\s+(.*)$')
fileexp = re.compile(r'([^\s]+):\s+file format')
startfile = re.compile(r'\.(text|bss|data)\s+(0x[0-9a-f]+)\s+0x[0-9a-f]+\s+(.*)$')

def read_obdump(inp):
	"Parse input"

	funcs = {}
	data = {}
	bss = {}
	fname = ''

	for line in inp:
		line = line.strip()
		res = funcline.match(line)
		if (res):
			funcs.setdefault(fname, []).append((int(res.group(1), 16), res.group(3)))
			continue

		res = bssline.match(line)
		if (res):
			start = int(res.group(1), 16)
			end = int(res.group(2), 16)
			if (end):
				bss.setdefault(fname, []).append((start, res.group(3)))

		res = dataline.match(line)
		if (res):
			start = int(res.group(1), 16)
			end = int(res.group(2), 16)
			if (end):
				data.setdefault(fname, []).append((start, res.group(3)))

		res = fileexp.match(line)
		if (res):
			fname = res.group(1)
			continue

	return {'text' : funcs, 'bss' : bss, 'data' : data}

def generate(kmapf, obmapf, out):
	"Generate output file"

	obdump = read_obdump(obmapf)

	def key_sorter(x):
		return x[0]

	for line in kmapf:
		line = line.strip()
		res = startfile.match(line)

		if ((res) and (res.group(3) in obdump[res.group(1)])):
			offset = int(res.group(2), 16)
			fname = res.group(3)
			symbols = obdump[res.group(1)][fname]
			symbols.sort(key = key_sorter)
			for addr, symbol in symbols:
				value = fname + ':' + symbol
				value_bytes = value.encode('ascii')
				data = struct.pack(symtabfmt, addr + offset, value_bytes[:MAXSTRING])
				out.write(data)

	out.write(struct.pack(symtabfmt, 0, b''))

def main():
	if (len(sys.argv) != 4):
		print("Usage: %s <kernel.map> <nm dump> <output.bin>" % sys.argv[0])
		return 1

	kmapf = open(sys.argv[1], 'r')
	obmapf = open(sys.argv[2], 'r')
	out = open(sys.argv[3], 'wb')

	generate(kmapf, obmapf, out)

	kmapf.close()
	obmapf.close()
	out.close()

if __name__ == '__main__':
	sys.exit(main())
